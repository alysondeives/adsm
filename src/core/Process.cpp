#include "Accelerator.h"
#include "IOBuffer.h"
#include "Mode.h"
#include "Process.h"

#include "allocator/Buddy.h"

#include "gmac/init.h"
#include "memory/Manager.h"
#include "memory/Object.h"
#include "memory/DistributedObject.h"
#include "trace/Thread.h"


namespace gmac {

ModeMap::ModeMap() :
    RWLock("ModeMap")
{}

std::pair<ModeMap::iterator, bool>
ModeMap::insert(Mode *mode, Accelerator *acc)
{
    lockWrite();
    std::pair<iterator, bool> ret = Parent::insert(value_type(mode, acc));
    unlock();
    return ret;
}

size_t ModeMap::remove(Mode &mode)
{
    lockWrite();
    size_type ret = Parent::erase(&mode);
    unlock();
    return ret;
}

ContextMap::ContextMap() :
    RWLock("ContextMap")
{}

std::pair<ContextMap::iterator, bool>
ContextMap::insert(Context *context, Mode *mode)
{
    lockWrite();
    std::pair<iterator, bool> ret = Parent::insert(value_type(context, mode));
    unlock();
    return ret;
}

size_t ContextMap::remove(Context &context)
{
    lockWrite();
    size_type ret = Parent::erase(&context);
    unlock();
    return ret;
}


QueueMap::QueueMap() :
    util::RWLock("QueueMap")
{}

void QueueMap::cleanup()
{
    QueueMap::iterator q;
    lockWrite();
    for(q = Parent::begin(); q != Parent::end(); q++)
        delete q->second;
    clear();
    unlock();
}

std::pair<QueueMap::iterator, bool>
QueueMap::insert(THREAD_ID tid, ThreadQueue *q)
{
    lockWrite();
    std::pair<iterator, bool> ret =
        Parent::insert(value_type(tid, q));
    unlock();
    return ret;
}

QueueMap::iterator QueueMap::find(THREAD_ID id)
{
    lockRead();
    iterator q = Parent::find(id);
    unlock();
    return q;
}

QueueMap::iterator QueueMap::end()
{
    lockRead();
    iterator ret = Parent::end();
    unlock();
    return ret;
}


size_t Process::TotalMemory_ = 0;

Process::Process() :
    RWLock("Process"),
    shared_("SharedMemoryMap"),
    centralized_("CentralizedMemoryMap"),
    replicated_("ReplicatedMemoryMap"),
    current_(0),
    ioMemory_(NULL)
{
	// Create the private per-thread variables for the implicit thread
	Process::getInstance().initThread();
}

Process::~Process()
{
    trace("Cleaning process");

    if(ioMemory_ != NULL) delete ioMemory_;
    ioMemory_ = NULL;

    std::vector<Accelerator *>::iterator a;
    std::list<Mode *>::iterator c;
    ContextMap::const_iterator i;
    for(i = contexts_.begin(); i != contexts_.end(); i++) {
        delete i->first;
    }

    // TODO: Why is this lock necessary?
    modes_.lockWrite();
    ModeMap::const_iterator j;
    for(j = modes_.begin(); j != modes_.end(); j++) {
        j->first->release();
        delete j->first;
    }
    modes_.clear();
    modes_.unlock();

    for(a = accs_.begin(); a != accs_.end(); a++)
        delete *a;
    accs_.clear();
    // TODO: Free buddy allocator
    queues_.cleanup();
    memoryFini();
}

void
Process::initThread()
{
    ThreadQueue * q = new ThreadQueue();
    queues_.insert(SELF(), q);
    // Set the private per-thread variables
    Mode::initThread();
}

Mode *Process::createMode(int acc)
{
    lockWrite();
    unsigned usedAcc;

    if (acc != ACC_AUTO_BIND) {
        assertion(acc < int(accs_.size()));
        usedAcc = acc;
    }
    else {
        // Bind the new Context to the accelerator with less contexts
        // attached to it
        usedAcc = 0;
        for (unsigned i = 0; i < accs_.size(); i++) {
            if (accs_[i]->load() < accs_[usedAcc]->load()) {
                usedAcc = i;
            }
        }
    }

    trace("Creatintg Execution Mode on Acc#%d", usedAcc);

    // Initialize the global shared memory for the context
    Mode *mode = accs_[usedAcc]->createMode(*this);
    accs_[usedAcc]->registerMode(*mode);
    contexts_.insert(&Context::current(), mode);
    modes_.insert(mode, accs_[usedAcc]);

    trace("Adding %zd replicated memory objects", replicated_.size());
    memory::Map::iterator i;
    for(i = replicated_.begin(); i != replicated_.end(); i++) {
        memory::DistributedObject *obj =
                dynamic_cast<memory::DistributedObject *>(i->second);
        obj->addOwner(*mode);
    }
    unlock();

    mode->attach();
    lockWrite();
    if(ioMemory_ == NULL)
        ioMemory_ = new kernel::allocator::Buddy(accs_.size() * paramIOMemory);
    unlock();
    return mode;
}

#ifndef USE_MMAP
gmacError_t Process::globalMalloc(memory::DistributedObject &object, size_t size)
{
    gmacError_t ret;
    ModeMap::iterator i;
    lockRead();
    for(i = modes_.begin(); i != modes_.end(); i++) {
        if((ret = object.addOwner(*i->first)) != gmacSuccess) goto cleanup;
    }
    unlock();
    return gmacSuccess;
cleanup:
    ModeMap::iterator j;
    for(j = modes_.begin(); j != i; j++) {
        object.removeOwner(*j->first);
    }
    unlock();
    return gmacErrorMemoryAllocation;

}

gmacError_t Process::globalFree(memory::DistributedObject &object)
{
    gmacError_t ret = gmacSuccess;
    ModeMap::iterator i;
    lockRead();
    for(i = modes_.begin(); i != modes_.end(); i++) {
        gmacError_t tmp = object.removeOwner(*i->first);
        if(tmp != gmacSuccess) ret = tmp;
    }
    unlock();
    return gmacSuccess;
}
#endif

// This function owns the global lock
gmacError_t Process::migrate(Mode &mode, int acc)
{
    if (acc >= int(accs_.size())) return gmacErrorInvalidValue;
    gmacError_t ret = gmacSuccess;
    trace("Migrating execution mode");
#ifndef USE_MMAP
    if (int(mode.accId()) != acc) {
        // Create a new context in the requested accelerator
        //ret = _accs[acc]->bind(mode);
        Context &context = Context::current();
        ret = mode.moveTo(*accs_[acc]);

        if (ret == gmacSuccess) {
            contexts_.remove(context);
            contexts_.insert(&context, &mode);
            modes_[&mode] = accs_[acc];
        }
    }
#else
    Fatal("Migration not implemented when using mmap");
#endif
    trace("Context migrated");
    return ret;
}

// TODO when is this function called
void Process::remove(Mode &mode)
{
    trace("Adding %zd replicated memory objects", replicated_.size());
    memory::Map::iterator i;
    for(i = replicated_.begin(); i != replicated_.end(); i++) {
        memory::DistributedObject *obj =
                dynamic_cast<memory::DistributedObject *>(i->second);
        obj->removeOwner(mode);
    }
    lockWrite();
    modes_.remove(mode);
    unlock();
}

void Process::addAccelerator(Accelerator *acc)
{
    accs_.push_back(acc);
}

IOBuffer *Process::createIOBuffer(size_t size)
{
    assertion(ioMemory_ != NULL);
    void *addr = ioMemory_->get(size);
    if(addr == NULL) return NULL;
    return new IOBuffer(Mode::current(), addr, size);
}

void Process::destroyIOBuffer(IOBuffer *buffer)
{
	assertion(buffer->state() == IOBuffer::Idle, "Destroying buffer still in use");
    if(ioMemory_ == NULL) return;
    ioMemory_->put(buffer->addr(), buffer->size());
    delete buffer;
}


void *Process::translate(void *addr)
{
    Mode &mode = Mode::current();
    const memory::Object *object = mode.getObjectRead(addr);
    if(object == NULL) return NULL;
    void * ptr = object->device(addr);
    mode.putObject(*object);

    return ptr;
}

void Process::send(THREAD_ID id)
{
    Mode &mode = Mode::current();
    QueueMap::iterator q = queues_.find(id);
    assertion(q != queues_.end());
    q->second->queue->push(&mode);
    mode.inc();
    mode.detach();
}

void Process::receive()
{
    // Get current context and destroy (if necessary)
    Mode::current().detach();
    // Get a fresh context
    QueueMap::iterator q = queues_.find(SELF());
    assertion(q != queues_.end());
    q->second->queue->pop()->attach();
}

void Process::sendReceive(THREAD_ID id)
{
    Mode &mode = Mode::current();
    QueueMap::iterator q = queues_.find(id);
    assertion(q != queues_.end());
    q->second->queue->push(&mode);
    Mode::initThread();
    q = queues_.find(SELF());
    assertion(q != queues_.end());
    q->second->queue->pop()->attach();
}

void Process::copy(THREAD_ID id)
{
    Mode &mode = Mode::current();
    QueueMap::iterator q = queues_.find(id);
    assertion(q != queues_.end());
    mode.inc();
    q->second->queue->push(&mode);
}

Mode *Process::owner(const void *addr) const
{
    const memory::Object *object = shared_.getObjectRead(addr);
    if(object == NULL) return NULL;
    Mode & ret = object->owner();
    shared_.putObject(*object);
    return &ret;
}

}
