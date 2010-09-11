#ifndef __API_CUDA_MODE_IPP_H_
#define __API_CUDA_MODE_IPP_H_

#include "Context.h"

namespace gmac { namespace cuda {

inline
void Switch::in()
{
    dynamic_cast<Mode *>(Mode::current())->switchIn();
}

inline
void Switch::out()
{
    dynamic_cast<Mode *>(Mode::current())->switchOut();
}

inline
void Mode::switchIn()
{
    __mutex.lock();
#ifdef USE_MULTI_CONTEXT
    CUresult ret = cuCtxPushCurrent(__ctx);
    cfatal(ret != CUDA_SUCCESS, "Unable to switch to CUDA mode");
#else
    acc->switchIn();
#endif
}

inline
void Mode::switchOut()
{
#ifdef USE_MULTI_CONTEXT
    CUcontext tmp;
    CUresult ret = cuCtxPopCurrent(&tmp);
    cfatal(ret != CUDA_SUCCESS, "Unable to switch back from CUDA mode");
#else
    acc->switchOut();
#endif
    __mutex.unlock();
}

inline
gmacError_t Mode::bufferToDevice(void *dst, gmac::IOBuffer *buffer, size_t len, off_t off)
{
    switchIn();
    gmacError_t ret = _context->bufferToDevice(dst, dynamic_cast<IOBuffer *>(buffer), len, off);
    switchOut();
    return ret;
}

inline
gmacError_t Mode::deviceToBuffer(gmac::IOBuffer *buffer, const void * src, size_t len, off_t off)
{
    switchIn();
    gmacError_t ret = _context->deviceToBuffer(dynamic_cast<IOBuffer *>(buffer), src, len, off);
    switchOut();
    return ret;
}

inline
void Mode::call(dim3 Dg, dim3 Db, size_t shared, cudaStream_t tokens)
{
    switchIn();
    _context->call(Dg, Db, shared, tokens);
    switchOut();
}

inline
void Mode::argument(const void *arg, size_t size, off_t offset)
{
    switchIn();
    _context->argument(arg, size, offset);
    switchOut();
}

inline Mode *
Mode::current()
{
    Mode *mode = static_cast<Mode *>(gmac::Mode::key.get());
    if(mode == NULL) mode = static_cast<Mode *>(proc->create());
    gmac::util::Logger::ASSERTION(mode != NULL);
    return mode;
}

#ifdef USE_VM
inline CUdeviceptr
Mode::dirtyBitmapDevPtr() const
{
    return _bitmapDevPtr;
}

inline CUdeviceptr
Mode::dirtyBitmapShiftPageDevPtr() const
{
    return _bitmapShiftPageDevPtr;
}

#ifdef BITMAP_BIT
inline CUdeviceptr
Mode::dirtyBitmapShiftEntryDevPtr() const
{
    return _bitmapShiftEntryDevPtr;
}
#endif
#endif

}}

#endif
