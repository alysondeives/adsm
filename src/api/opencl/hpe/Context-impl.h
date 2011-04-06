#ifndef GMAC_API_OPENCL_HPE_CONTEXT_IMPL_H_
#define GMAC_API_OPENCL_HPE_CONTEXT_IMPL_H_

#include "Kernel.h"

#include "api/opencl/hpe/Accelerator.h"

namespace __impl { namespace opencl { namespace hpe {

inline gmacError_t
Context::call(cl_uint workDim, size_t *globalWorkOffset, size_t *globalWorkSize, size_t *localWorkSize)
{
    ASSERTION(globalWorkSize != NULL);
    ASSERTION(localWorkSize != NULL);
    TRACE(LOCAL, "Creating new kernel call");
    call_ = KernelConfig(workDim, globalWorkOffset, globalWorkSize, localWorkSize, streamLaunch_);

    // TODO: perform some checking
    return gmacSuccess;
}

inline gmacError_t
Context::argument(const void *arg, size_t size, unsigned index)
{
    call_.setArgument(arg, size, index);
    // TODO: perform some checking
    return gmacSuccess;
}

inline const cl_command_queue
Context::eventStream() const
{
    return streamLaunch_;
}

inline Accelerator &
Context::accelerator()
{
    return dynamic_cast<Accelerator &>(acc_);
}

}}}

#endif