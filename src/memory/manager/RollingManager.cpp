#include "RollingManager.h"
#include "os/Memory.h"

#include <kernel/Context.h>

#include <unistd.h>

#include <typeinfo>

namespace gmac { namespace memory { namespace manager {

RollingBuffer::RollingBuffer() :
    RWLock(paraver::rollingBuffer),
    _max(0)
    {}

RollingMap::~RollingMap()
{
    RollingMap::iterator i;
    for(i = this->begin(); i != this->end(); i++)
        delete i->second;
}

void RollingManager::writeBack()
{
    // Get the buffer for the current thread
    RollingBlock *r = rollingMap.currentBuffer()->pop();
    r->lockWrite();
    flush(r);
    r->unlock();
}

    Region *    
RollingManager::newRegion(void * addr, size_t count, bool shared)
{
    rollingMap.currentBuffer()->inc(lruDelta);
    return new RollingRegion(*this, addr, count, shared, pageTable().getPageSize());
}

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
}

void RollingManager::free(void *addr)
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
        rollingMap.currentBuffer()->dec(lruDelta);
#else
    rollingMap.currentBuffer()->dec(lruDelta);
#endif
    TRACE("Released %p", addr);
}


void RollingManager::flush()
{
    TRACE("RollingManager Flush Starts");
    Context * ctx = Context::current();
#if 0
    RegionMap::iterator s;
    RegionMap &sharedMem = Map::shared();
    for(s = sharedMem.begin(); s != sharedMem.end(); s++) {
        RollingRegion *r = current()->find<RollingRegion>(s->second.start());
        r->lockWrite();
        r->transferDirty();
        r->unlock();
    }
#endif

    // We need to go through all regions from the context because
    // other threads might have regions owned by this context in
    // their flush buffer
    Map::iterator i;
    Map * m = current();
    m->lockRead();
    for(i = m->begin(); i != m->end(); i++) {
        RollingRegion *r = dynamic_cast<RollingRegion *>(i->second);
        r->lockWrite();
        r->flush();
        r->unlock();
    }
    m->unlock();

    RegionMap::iterator s;
    RegionMap &shared = Map::shared();
    shared.lockRead();
    for (s = shared.begin(); s != shared.end(); s++) {
        RollingRegion * r = dynamic_cast<RollingRegion *>(s->second);
        r->lockWrite();
        r->transferDirty();
        r->unlock();
    }
    shared.unlock();


    /** \todo Fix vm */
    // ctx->flush();
    ctx->invalidate();
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
    Context * ctx = Context::current();
#if 0
    SharedMap::iterator i;
    SharedMap &sharedMem = proc->sharedMem();
    sharedMem.lockRead();
    for(i = sharedMem.begin(); i != sharedMem.end(); i++) {
        RollingRegion * r = current()->find<RollingRegion>(i->second.start());
        r->lockWrite();
        r->transferDirty();
        r->unlock();
    }
    sharedMem.unlock();
#endif
    size_t blocks = rollingMap.currentBuffer()->size();

    for(int j = 0; j < blocks; j++) {
        RollingBlock *r = rollingMap.currentBuffer()->pop();
        r->lockWrite();
        // Check if we have to flush
        if(std::find(regions.begin(), regions.end(), &r->getParent()) == regions.end()) {
            rollingMap.currentBuffer()->push(r);
            r->unlock();
            continue;
        }
        flush(r);
        r->unlock();
        TRACE("Flush to Device %p", r->start());
    }

    /** \todo Fix vm */
    // ctx->flush();
    ctx->invalidate();
    TRACE("RollingManager Flush Ends");
}

void RollingManager::invalidate()
{
    TRACE("RollingManager Invalidation Starts");
    Map::iterator i;
    Map * m = current();
    m->lockRead();
    for(i = m->begin(); i != m->end(); i++) {
        RollingRegion *r = dynamic_cast<RollingRegion *>(i->second);
        r->lockWrite();
        r->invalidate();
        r->unlock();
    }
    m->unlock();

    Context * ctx = Context::current();
    RegionMap::iterator s;
    RegionMap &shared = Map::shared();
    shared.lockRead();
    for (s = shared.begin(); s != shared.end(); s++) {
        RollingRegion * r = dynamic_cast<RollingRegion *>(s->second);
        r->lockWrite();
        if(r->owner() == ctx) {
            r->invalidate();
        }
        r->unlock();
    }
    shared.unlock();

    //gmac::Context::current()->flush();
    ctx->invalidate();
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
    for(i = regions.begin(); i != regions.end(); i++) {
        RollingRegion *r = dynamic_cast<RollingRegion *>(*i);
        r->lockWrite();
        r->invalidate();
        r->unlock();
    }
    //gmac::Context::current()->flush();
    gmac::Context::current()->invalidate();
    TRACE("RollingManager Invalidation Ends");
}

void RollingManager::invalidate(const void *addr, size_t size)
{
    RollingRegion *reg = current()->find<RollingRegion>(addr);
    ASSERT(reg != NULL);
    reg->lockWrite();
    ASSERT(reg->end() >= (void *)((addr_t)addr + size));
    reg->invalidate(addr, size);
    reg->unlock();
}

void RollingManager::flush(const void *addr, size_t size)
{
    RollingRegion *reg = current()->find<RollingRegion>(addr);
    ASSERT(reg != NULL);
    reg->lockWrite();
    ASSERT(reg->end() >= (void *)((addr_t)addr + size));
    reg->flush(addr, size);
    reg->unlock();
}

//
// Handler Interface
//

bool RollingManager::read(void *addr)
{
    RollingRegion *root = current()->find<RollingRegion>(addr);
    if(root == NULL) return false;
    Context * owner = root->owner();
    ProtRegion *region = root->find(addr);
    ASSERT(region != NULL);
    region->lockWrite();
    if (region->present() == true) {
        region->unlock();
        return true;
    }
    region->readWrite();
    if(current()->pageTable().dirty(addr)) {
        gmacError_t ret = region->copyToHost();
        ASSERT(ret == gmacSuccess);
        current()->pageTable().clear(addr);
    }
    region->readOnly();
    region->unlock();
    return true;
}


bool RollingManager::write(void *addr)
{
    RollingRegion *root = current()->find<RollingRegion>(addr);
    if (root == NULL) return false;
    root->lockWrite();
    ProtRegion *region = root->find(addr);
    ASSERT(region != NULL);
    // Other thread fixed the fault?
    region->lockWrite();
    if(region->dirty() == true) {
        region->unlock();
        root->unlock();
        return true;
    }
    Context *owner = root->owner();

    Context *ctx = Context::current();

    while(rollingMap.currentBuffer()->overflows()) writeBack();
    region->readWrite();
    if(region->present() == false && current()->pageTable().dirty(addr)) {
        gmacError_t ret = region->copyToHost();
        ASSERT(ret == gmacSuccess);
        current()->pageTable().clear(addr);
    }
    region->unlock();
    root->unlock();
    rollingMap.currentBuffer()->push(dynamic_cast<RollingBlock *>(region));
    return true;
}

    void
RollingManager::remap(Context *ctx, Region *r, void *devPtr)
{
    RollingRegion *region = dynamic_cast<RollingRegion *>(r);
    ASSERT(region != NULL);
    region->lockWrite();
    insertVirtual(ctx, r->start(), devPtr, r->size());
    region->relate(ctx);
    region->transferNonDirty();
    region->unlock();
}

}}}
