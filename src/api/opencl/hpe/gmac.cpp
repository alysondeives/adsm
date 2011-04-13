#include "config/common.h"
#include "include/gmac/opencl.h"

#include "api/opencl/hpe/Accelerator.h"
#include "api/opencl/hpe/Mode.h"
#include "api/opencl/hpe/Kernel.h"

#if defined(GMAC_DLL)
#include "hpe/init.h"
#endif

using __impl::opencl::hpe::Accelerator;
using __impl::opencl::hpe::Mode;

GMAC_API gmacError_t APICALL __oclKernelSetArg(OclKernel *kernel, const void *addr, size_t size, unsigned index)
{
    gmac::enterGmac();
    ((__impl::opencl:hpe::KernelLaunch *)kernel->launch_)->setArgument(addr, size, index);
    gmac::exitGmac();

    return gmacSuccess;
}

GMAC_API gmacError_t APICALL __oclKernelConfigure(OclKernel *kernel, size_t workDim, size_t *globalWorkOffset,
        size_t *globalWorkSize, size_t *localWorkSize)
{
    gmac::enterGmac();
    Mode &mode = Mode::getCurrent();
    ((__impl::opencl::hpe::KernelLaunch *)kernel->launch_)->setConfiguration(cl_int(workDim),
            globalWorkOffset, globalWorkSize, localWorkSize);
    gmac::exitGmac();
    return gmacSuccess;
}

gmacError_t gmacLaunch(__impl::core::hpe::KernelLaunch &);
GMAC_API gmacError_t APICALL __oclKernelLaunch(OclKernel *k)
{
    gmac::enterGmac();
    gmacError_t ret = gmacLaunch(*(__impl::opencl::hpe::KernelLaunch *)kernel->launch_);
    gmac::exitGmac();
    return ret;
}

gmacError_t gmacThreadSynchronize(__impl::core::hpe::KernelLaunch &);
GMAC_API gmacError_t APICALL __oclKernelWait(OclKernel *kernel)
{
    gmac::enterGmac();
    gmacError_t ret = gmacThreadSynchronize(*(__impl::opencl::KernelLaunch *)kernel->launch_);
    gmac::exitGmac();
    return ret;
}


GMAC_API gmacError_t APICALL __oclPrepareCLCode(const char *code, const char *flags)
{
    gmac::enterGmac();
    gmacError_t ret = Accelerator::prepareCLCode(code, flags);
    gmac::exitGmac();

    return ret;
}

gmacError_t APICALL __oclPrepareCLBinary(const unsigned char *binary, size_t size, const char *flags)
{
    gmac::enterGmac();
    gmacError_t ret = Accelerator::prepareCLBinary(binary, size, flags);
    gmac::exitGmac();

    return ret;
}

gmacError_t APICALL __oclKernelGet(gmac_kernel_id_t id, OclKernel *kernel)
{
    gmac::enterGmac();
    __impl::core::Mode &mode = gmac::core::Mode::getCurrent();
    gmac::memory::Manager &manager = gmac::memory::Manager::getInstance();
    __impl::core::KernelLaunch *launch;
    gmacError_t ret = mode.launch(id, launch);
    if (ret == gmacSuccess) {
        kernel->id_ = id;
        kernel->launch_ = launch;
    }
    gmac::exitGmac();

    return gmacSuccess;
}

gmacError_t APICALL __oclKernelDestroy(OclKernel *kernel)
{
    gmac::enterGmac();
    if (kernel->launch_ != NULL) {
        delete ((__impl::opencl::KernelLaunch *) kernel->launch_);
    }
    gmac::exitGmac();

    return gmacSuccess;
}

/* vim:set backspace=2 tabstop=4 shiftwidth=4 textwidth=120 foldmethod=marker expandtab: */
