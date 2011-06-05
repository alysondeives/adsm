#ifndef GMAC_CORE_HPE_MODE_IMPL_H_
#define GMAC_CORE_HPE_MODE_IMPL_H_

#include "memory/Object.h"

#include "core/hpe/Accelerator.h"
#include "core/hpe/Process.h"
#include "core/hpe/Context.h"
#include "core/IOBuffer.h"

namespace __impl { namespace core { namespace hpe {

inline void ContextMap::add(THREAD_T id, Context *ctx)
{
    lockWrite();
    Parent::insert(Parent::value_type(id, ctx));
    unlock();
}

inline Context *ContextMap::find(THREAD_T id)
{
    lockRead();
    Parent::iterator i = Parent::find(id);
    Context *ret = NULL;
    if(i != end()) ret = i->second;
    unlock();
    return ret;
}

inline void ContextMap::remove(THREAD_T id)
{
    lockWrite();
    Parent::erase(id);
    unlock();
}

inline void ContextMap::clean()
{
    Parent::iterator i;
    lockWrite();
    for(i = begin(); i != end(); i++) delete i->second;
    Parent::clear();
    unlock();
}

inline gmacError_t ContextMap::prepareForCall()
{
    gmacError_t ret = gmacSuccess;
#if 0
    Parent::iterator i;
    lockWrite();
    for(i = begin(); i != end(); i++) {
        ret = i->second->prepareForCall();
        if(ret != gmacSuccess) break;
    }
    unlock();
#endif
    return ret;
}

inline gmacError_t ContextMap::waitForCall()
{
    gmacError_t ret = gmacSuccess;
#if 0
    Parent::iterator i;
    lockRead();
    for(i = begin(); i != end(); i++) {
        ret = i->second->waitForCall();
        if(ret != gmacSuccess) break;
    }
    unlock();
#endif
    return ret;
}

inline gmacError_t ContextMap::waitForCall(KernelLaunch &launch)
{
    Parent::iterator i;
    gmacError_t ret = gmacSuccess;
    lockRead();
    for(i = begin(); i != end(); i++) {
        ret = i->second->waitForCall(launch);
        if(ret != gmacSuccess) break;
    }
    unlock();
    return ret;
}

inline
memory::ObjectMap &Mode::getObjectMap()
{
    return map_;
}

inline
const memory::ObjectMap &Mode::getObjectMap() const
{
    return map_;
}


inline void Mode::cleanUpContexts()
{
    contextMap_.clean();
}

inline
void Mode::insertOrphan(memory::Object &obj)
{
    proc_.insertOrphan(obj);
}

inline
Accelerator &
Mode::getAccelerator() const
{
    return *acc_;
}

#ifdef USE_VM
inline memory::vm::Bitmap &
Mode::getDirtyBitmap()
{
    return bitmap_;
}

inline const memory::vm::Bitmap &
Mode::getDirtyBitmap() const
{
    return bitmap_;
}
#endif


inline Process &Mode::process()
{
    return proc_;
}

inline const Process &Mode::process() const
{
    return proc_;
}

inline void
Mode::memInfo(size_t &free, size_t &total)
{
    switchIn();
    acc_->memInfo(free, total);
    switchOut();
}

inline
gmacError_t
Mode::prepareForCall()
{
    switchIn();
    trace::SetThreadState(trace::Wait);
    gmacError_t ret = acc_->syncStream(streamToAccelerator_);
    trace::SetThreadState(trace::Idle);
    switchOut();
    return ret;
}

inline
gmacError_t Mode::bufferToAccelerator(accptr_t dst, IOBuffer &buffer, size_t len, size_t off)
{
    TRACE(LOCAL,"Copy %p to device %p ("FMT_SIZE" bytes)", buffer.addr(), dst.get(), len);
    trace::EnterCurrentFunction();
    switchIn();
    gmacError_t ret = acc_->copyToAcceleratorAsync(dst, buffer, off, len, *this, streamToAccelerator_);
    switchOut();
    trace::ExitCurrentFunction();
    return ret;
}

inline
gmacError_t Mode::acceleratorToBuffer(IOBuffer &buffer, const accptr_t src, size_t len, size_t off)
{
    TRACE(LOCAL,"Copy %p to host %p ("FMT_SIZE" bytes)", src.get(), buffer.addr(), len);
    trace::EnterCurrentFunction();
    switchIn();
    // Implement a function to remove these casts
    gmacError_t ret = acc_->copyToHostAsync(buffer, off, src, len, *this, streamToHost_);
    switchOut();
    trace::ExitCurrentFunction();
    return ret;
}

inline gmacError_t
Mode::wait(core::hpe::KernelLaunch &launch)
{
    switchIn();
    // TODO: use an event for this
    error_ = acc_->syncStream(streamLaunch_);
    switchOut();

    return error_;
}


inline gmacError_t
Mode::wait()
{
    switchIn();
    error_ = acc_->syncStream(streamLaunch_);
    switchOut();

    return error_;
}


}}}

#endif
