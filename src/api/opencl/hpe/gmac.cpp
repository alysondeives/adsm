#include <fstream>

#include "config/common.h"
#include "include/gmac/opencl.h"

#include "api/opencl/hpe/Accelerator.h"
#include "api/opencl/hpe/Mode.h"
#include "api/opencl/hpe/Kernel.h"
#include "memory/Manager.h"

#include "hpe/init.h"

using __impl::util::params::ParamAutoSync;

static inline __impl::opencl::hpe::Mode &getCurrentOpenCLMode()
{
	return dynamic_cast<__impl::opencl::hpe::Mode &>(__impl::core::hpe::getCurrentMode());
}

GMAC_API gmacError_t APICALL __oclKernelSetArg(ocl_kernel *kernel, const void *addr, size_t size, unsigned index)
{
    enterGmac();
    ((__impl::opencl::hpe::KernelLaunch *)kernel->launch_)->setArgument(addr, size, index);
    exitGmac();

    return gmacSuccess;
}

GMAC_API gmacError_t APICALL __oclKernelConfigure(ocl_kernel *kernel, size_t workDim, size_t *globalWorkOffset,
        size_t *globalWorkSize, size_t *localWorkSize)
{
    enterGmac();
    ((__impl::opencl::hpe::KernelLaunch *)kernel->launch_)->setConfiguration(cl_int(workDim),
            globalWorkOffset, globalWorkSize, localWorkSize);
    exitGmac();
    return gmacSuccess;
}

gmacError_t gmacLaunch(__impl::core::hpe::KernelLaunch &);
gmacError_t gmacThreadSynchronize(__impl::core::hpe::KernelLaunch &);
GMAC_API gmacError_t APICALL __oclKernelLaunch(ocl_kernel *kernel)
{
    enterGmac();
    gmacError_t ret = gmacLaunch(*(__impl::opencl::hpe::KernelLaunch *)kernel->launch_);
    if(ret == gmacSuccess) {
#if defined(SEPARATE_COMMAND_QUEUES)
        ret = gmacThreadSynchronize(*(__impl::opencl::hpe::KernelLaunch *)kernel->launch_);
#else
        ret = __impl::memory::getManager().acquireObjects(getCurrentOpenCLMode());
#endif
    }
    exitGmac();
    return ret;
}


#if 0
GMAC_API gmacError_t APICALL __oclKernelWait(ocl_kernel *kernel)
{
    enterGmac();
    gmacError_t ret = gmacThreadSynchronize(*(__impl::opencl::hpe::KernelLaunch *)kernel->launch_);
    exitGmac();
    return ret;
}
#endif


GMAC_API gmacError_t APICALL __oclPrepareCLCode(const char *code, const char *flags)
{
    enterGmac();
    gmacError_t ret = __impl::opencl::hpe::Accelerator::prepareCLCode(code, flags);
    exitGmac();

    return ret;
}

GMAC_API gmacError_t APICALL __oclPrepareCLCodeFromFile(const char *path, const char *flags)
{
    std::ifstream in(path, std::ios_base::in);
    if (!in.good()) return gmacErrorInvalidValue;
    in.seekg (0, std::ios::end);
    std::streampos length = in.tellg();
    in.seekg (0, std::ios::beg);
	if (length == std::streampos(0)) return gmacSuccess;
    // Allocate memory for the code
    char *buffer = new char[int(length)+1];
    // Read data as a block
    in.read(buffer,length);
    buffer[length] = '\0';
    in.close();
    gmacError_t ret = __oclPrepareCLCode(buffer, flags);
    in.close();
    delete [] buffer;

    return ret;
}


gmacError_t APICALL __oclPrepareCLBinary(const unsigned char *binary, size_t size, const char *flags)
{
    enterGmac();
    gmacError_t ret = __impl::opencl::hpe::Accelerator::prepareCLBinary(binary, size, flags);
    exitGmac();

    return ret;
}

gmacError_t APICALL __oclKernelGet(gmac_kernel_id_t id, ocl_kernel *kernel)
{
    enterGmac();
    __impl::core::hpe::Mode &mode = getCurrentOpenCLMode();
    __impl::core::hpe::KernelLaunch *launch;
    gmacError_t ret = mode.launch(id, launch);
    if (ret == gmacSuccess) {
        kernel->id_ = id;
        kernel->launch_ = launch;
    }
    exitGmac();

    return gmacSuccess;
}

gmacError_t APICALL __oclKernelDestroy(ocl_kernel *kernel)
{
    enterGmac();
    if (kernel->launch_ != NULL) {
        delete ((__impl::opencl::hpe::KernelLaunch *) kernel->launch_);
    }
    exitGmac();

    return gmacSuccess;
}

/* vim:set backspace=2 tabstop=4 shiftwidth=4 textwidth=120 foldmethod=marker expandtab: */
