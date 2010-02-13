#include "RollingManager.h"
#include "os/Memory.h"

#include <kernel/Context.h>

#include <unistd.h>

#include <typeinfo>

namespace gmac { namespace memory { namespace manager {

RollingBuffer::RollingBuffer() :
    _lock(paraver::rollingBuffer),
    _max(0)
{}

void RollingManager::waitForWrite(void *addr, size_t size)
{
    writeMutex.lock();
    if(writeBuffer) {
        RollingBlock *r = regionRolling[Context::current()]->front();
        r->sync();
        munlock(writeBuffer, writeBufferSize);
    }
    writeBuffer = addr;
    writeBufferSize = size;
    writeMutex.unlock();
}


void RollingManager::writeBack()
{
    RollingBlock *r = regionRolling[Context::current()]->pop();
    waitForWrite(r->start(), r->size());
    mlock(writeBuffer, writeBufferSize);
    assert(r->copyToDevice() == gmacSuccess);
    r->readOnly();
}


PARAM_REGISTER(paramLineSize,
               size_t,
               1024,
               "GMAC_LINESIZE",
               PARAM_NONZERO);

PARAM_REGISTER(paramLruDelta,
               size_t,
               2,
               "GMAC_LRUDELTA",
               PARAM_NONZERO);

RollingManager::RollingManager() :
    Handler(),
    lineSize(0),
    lruDelta(0),
    writeMutex(paraver::writeMutex),
    writeBuffer(NULL),
    writeBufferSize(0)
{
    lineSize = paramLineSize;
    lruDelta = paramLruDelta;
    TRACE("Using %d as Line Size", lineSize);
    TRACE("Using %d as LRU Delta Size", lruDelta);
}

RollingManager::~RollingManager()
{
    std::map<Context *, RollingBuffer *>::iterator r;
    for (r = regionRolling.begin(); r != regionRolling.end(); r++) {
        delete r->second;
    }
}

void *RollingManager::alloc(void *addr, size_t count, int attr)
{
    void *cpuAddr;

    Context * ctx = Context::current();
    if (attr == GMAC_MALLOC_PINNED) {
        void *hAddr;
        if (ctx->halloc(&hAddr, count) != gmacSuccess) return NULL;
        cpuAddr = hostRemap(addr, hAddr, count);
    } else {
        cpuAddr = hostMap(addr, count, PROT_NONE);
    }
    insertVirtual(cpuAddr, addr, count);
    if (!regionRolling[ctx]) {
        regionRolling[ctx] = new RollingBuffer();
    }
    regionRolling[ctx]->inc(lruDelta);
    insert(new RollingRegion(*this, cpuAddr, count, pageTable().getPageSize()));
	TRACE("Alloc %p (%d bytes)", cpuAddr, count);
    return cpuAddr;
}


void RollingManager::release(void *addr)
{
    Region *reg = remove(addr);
    removeVirtual(reg->start(), reg->size());
    Context * ctx = Context::current();
    if(reg->owner() == ctx) {
        hostUnmap(addr, reg->size()); // Global mappings do not have a shadow copy in system memory
        TRACE("Deleting Region %p\n", addr);
        delete reg;
    }
#ifdef USE_GLOBAL_HOST
	// When using host-mapped memory, global regions do not
	// increase the rolling size
	if(proc->isShared(addr) == false)
        regionRolling[ctx]->dec(lruDelta);
#else
	regionRolling[ctx]->dec(lruDelta);
#endif
    TRACE("Released %p", addr);
}


void RollingManager::flush()
{
    TRACE("RollingManager Flush Starts");
    Context * ctx = Context::current();
    waitForWrite();
    while(regionRolling[ctx]->empty() == false) {
        RollingBlock *r = regionRolling[ctx]->pop();
        assert(r->copyToDevice() == gmacSuccess);
        r->readOnly();
        TRACE("Flush to Device %p", r->start());
    }

    TRACE("RollingManager Flush Ends");
}

void RollingManager::flush(const RegionSet & regions)
{
    // If no dependencies, a global flush is assumed
    if (regions.size() == 0) {
        flush();
        return;
    }

    TRACE("RollingManager Flush Starts");
    waitForWrite();
    RollingBuffer * buffer = regionRolling[Context::current()];
    size_t blocks = buffer->size();

    for(int i = 0; i < blocks; i++) {
        RollingBlock *r = regionRolling[Context::current()]->pop();
        // Check if we have to flush
        if (std::find(regions.begin(), regions.end(), &r->getParent()) == regions.end()) {
            buffer->push(r);
            continue;
        }

        assert(r->copyToDevice() == gmacSuccess);
        r->readOnly();
        TRACE("Flush to Device %p", r->start());
    }

    TRACE("RollingManager Flush Ends");
}

void RollingManager::invalidate()
{
    TRACE("RollingManager Invalidation Starts");
    Map::iterator i;
    Map * m = current();
    m->lock();
    for(i = m->begin(); i != m->end(); i++) {
        Region *r = i->second;
        assert(typeid(*r) == typeid(RollingRegion));
        dynamic_cast<RollingRegion *>(r)->invalidate();
    }
    m->unlock();
    //gmac::Context::current()->flush();
    gmac::Context::current()->invalidate();
    TRACE("RollingManager Invalidation Ends");
}

void RollingManager::invalidate(const RegionSet & regions)
{
    // If no dependencies, a global invalidation is assumed
    if (regions.size() == 0) {
        invalidate();
        return;
    }

    TRACE("RollingManager Invalidation Starts");
    RegionSet::const_iterator i;
    Map * m = current();
    m->lock();
    for(i = regions.begin(); i != regions.end(); i++) {
        Region *r = *i;
        assert(typeid(*r) == typeid(RollingRegion));
        dynamic_cast<RollingRegion *>(r)->invalidate();
    }
    m->unlock();
    //gmac::Context::current()->flush();
    gmac::Context::current()->invalidate();
    TRACE("RollingManager Invalidation Ends");
}

void RollingManager::invalidate(const void *addr, size_t size)
{
	RollingRegion *reg = current()->find<RollingRegion>(addr);
    assert(reg != NULL);
    assert(reg->end() >= (void *)((addr_t)addr + size));
    reg->invalidate(addr, size);
}

void RollingManager::flush(const void *addr, size_t size)
{
	RollingRegion *reg = current()->find<RollingRegion>(addr);
    assert(reg != NULL);
    assert(reg->end() >= (void *)((addr_t)addr + size));
    reg->flush(addr, size);
}

//
// Handler Interface
//

bool RollingManager::read(void *addr)
{
	RollingRegion *root = current()->find<RollingRegion>(addr);
    if(root == NULL) return false;
    Context * owner = root->owner();
    if (owner->status() == Context::RUNNING) owner->sync();

    ProtRegion *region = root->find(addr);
    assert(region != NULL);
    assert(region->present() == false);
    region->readWrite();
    if(current()->pageTable().dirty(addr)) {
        assert(region->copyToHost() == gmacSuccess);
        current()->pageTable().clear(addr);
    }
    region->readOnly();
    return true;
}


bool RollingManager::write(void *addr)
{
	RollingRegion *root = current()->find<RollingRegion>(addr);
    if (root == NULL) return false;
    Context * owner = root->owner();
    if (owner->status() == Context::RUNNING) owner->sync();

    ProtRegion *region = root->find(addr);
    assert(region != NULL);
    assert(region->dirty() == false);
    Context * ctx = Context::current();
    if (!regionRolling[ctx]) {
        regionRolling[ctx] = new RollingBuffer();
    }
    while(regionRolling[ctx]->overflows()) writeBack();
    region->readWrite();
    if(region->present() == false && current()->pageTable().dirty(addr)) {
        assert(region->copyToHost() == gmacSuccess);
        current()->pageTable().clear(addr);
    }
    regionRolling[ctx]->push(
        dynamic_cast<RollingBlock *>(region));
    return true;
}


}}}
