#ifndef GMAC_API_OPENCL_KERNEL_IMPL_H_
#define GMAC_API_OPENCL_KERNEL_IMPL_H_

#include "util/Logger.h"

#include "hpe/init.h"
#include "api/opencl/hpe/Mode.h"

#include "memory/Manager.h"

namespace __impl { namespace opencl { namespace hpe {

inline
Kernel::Kernel(const core::hpe::KernelDescriptor & k, cl_kernel kernel) :
    gmac::core::hpe::Kernel(k), f_(kernel)
{
    cl_int ret = clGetKernelInfo(kernel, CL_KERNEL_NUM_ARGS, sizeof(cl_int), &nArgs_, NULL);
    ASSERTION(ret == CL_SUCCESS);
}

inline
Kernel::~Kernel()
{
    cl_int ret = clReleaseKernel(f_);
    ASSERTION(ret == CL_SUCCESS);
}

inline
KernelLaunch *
Kernel::launch(Mode &mode, cl_command_queue stream)
{
    KernelLaunch * l = new KernelLaunch(mode, *this, stream);
    return l;
}

inline
unsigned
Kernel::getNArgs() const
{
    return nArgs_;
}

inline void
KernelLaunch::setConfiguration(cl_uint work_dim, size_t *globalWorkOffset,
        size_t *globalWorkSize, size_t *localWorkSize)
{
    if(workDim_ < work_dim) {
        if(globalWorkOffset_ != 0) {
            delete [] globalWorkOffset_;
            globalWorkOffset_ = NULL;
        }
        if(globalWorkSize_ != 0) {
            delete [] globalWorkSize_;
            globalWorkSize = NULL;
        }
        if(localWorkSize_ != 0) {
            delete [] localWorkSize_;
            localWorkSize_ = 0;
        }

        if(globalWorkOffset) {
            globalWorkOffset_ = new size_t[work_dim];
        }
        if(globalWorkSize) {
            globalWorkSize_ = new size_t[work_dim];
        }
        if(localWorkSize) {
            localWorkSize_ = new size_t[work_dim];
        }
    }

    workDim_ = work_dim;

    for(unsigned i = 0; i < workDim_; i++) {
        if(globalWorkOffset) globalWorkOffset_[i] = globalWorkOffset[i];
        if(globalWorkSize) globalWorkSize_[i] = globalWorkSize[i];
        if(localWorkSize) localWorkSize_[i] = localWorkSize[i];
    }

}

inline gmacError_t
KernelLaunch::setArgument(const void *arg, size_t size, unsigned index)
{
    TRACE(LOCAL, "Setting param %u @ %p ("FMT_SIZE")", index, arg, size);
    cl_int ret = clSetKernelArg(f_, index, size, arg);
    return Accelerator::error(ret);
}

inline
KernelLaunch::KernelLaunch(Mode &mode, const Kernel & k, cl_command_queue stream) :
#ifdef DEBUG
    core::hpe::KernelLaunch(dynamic_cast<core::hpe::Mode &>(mode), k.key()),
#else
    core::hpe::KernelLaunch(dynamic_cast<core::hpe::Mode &>(mode)),
#endif
    f_(k.f_),
    stream_(stream),
    workDim_(0),
    globalWorkOffset_(NULL),
    globalWorkSize_(NULL),
    localWorkSize_(NULL),
    trace_(mode.getAccelerator().getMajor(), mode.getAccelerator().getMinor())
{
    clRetainKernel(f_);
}

inline
KernelLaunch::~KernelLaunch()
{
    clReleaseKernel(f_);

    if (globalWorkOffset_) delete [] globalWorkOffset_;
    if (globalWorkSize_) delete [] globalWorkSize_;
    if (localWorkSize_) delete [] localWorkSize_;

    CacheSubBuffer::iterator itCacheMap;
    for (itCacheMap = cacheSubBuffer_.begin(); itCacheMap != cacheSubBuffer_.end(); itCacheMap++) {
        __impl::core::hpe::Mode *mode = itCacheMap->second.first;
        MapSubBuffer::iterator sb = itCacheMap->second.second;
        if (--sb->second.second == 0) {
            MapGlobalSubBuffer::iterator itG = mapSubBuffer_.findMode(*mode);
            MapSubBuffer &map = itG->second;
               
            int err = clReleaseMemObject(sb->second.first);
            ASSERTION(err == CL_SUCCESS);

            printf("Removing: %p\n", itCacheMap->first);
            map.erase(sb);
        } else {
            printf("Not Removing: %p\n", itCacheMap->first);
        }
    }
    cacheSubBuffer_.clear();
}

inline
cl_event
KernelLaunch::getCLEvent()
{
    return event_;
}

#if 0
inline
bool
KernelLaunch::hasSubBuffer(hostptr_t ptr) const
{
    return subBuffers_.find(ptr) != subBuffers_.end();
}
#endif

#if 0
inline
void
KernelLaunch::setSubBuffer(hostptr_t ptr, cl_mem subMem)
{
    ASSERTION(hasSubBuffer(ptr) == false);
    subBuffers_.insert(std::map<hostptr_t, cl_mem>::value_type(ptr, subMem));
}
#endif

}}}

#endif

/* vim:set backspace=2 tabstop=4 shiftwidth=4 textwidth=120 foldmethod=marker expandtab: */
