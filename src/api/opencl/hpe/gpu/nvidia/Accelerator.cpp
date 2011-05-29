#include "api/opencl/IOBuffer.h"

#include "api/opencl/hpe/gpu/nvidia/Accelerator.h"

namespace __impl { namespace opencl { namespace hpe { namespace gpu { namespace nvidia {

Accelerator::Accelerator(int n, cl_context context, cl_device_id device) :
    gmac::opencl::hpe::Accelerator(n, context, device)
{
}

Accelerator::~Accelerator()
{
}

gmacError_t Accelerator::copyToAcceleratorAsync(accptr_t acc, IOBuffer &buffer,
    size_t bufferOff, size_t count, core::hpe::Mode &mode, cl_command_queue stream)
{
    hostptr_t host = buffer.addr() + bufferOff;

    trace::EnterCurrentFunction();
    TRACE(LOCAL, "Async copy to accelerator: %p ("FMT_SIZE") @ %p", host, count, acc.get());

    cl_event start;
    cl_int ret;

    buffer.toAccelerator(dynamic_cast<opencl::Mode &>(mode));
    ret = clEnqueueWriteBuffer(stream, acc.get(), CL_FALSE,
            acc.offset(), count, host, 0, NULL, &start);
    CFATAL(ret == CL_SUCCESS, "Error copying to accelerator: %d", ret);
    buffer.started(start, count);

    trace::ExitCurrentFunction();
    return error(ret);
}

gmacError_t Accelerator::copyToHostAsync(IOBuffer &buffer, size_t bufferOff,
    const accptr_t acc, size_t count, core::hpe::Mode &mode, cl_command_queue stream)
{
    hostptr_t host = buffer.addr() + bufferOff;

    trace::EnterCurrentFunction();
    TRACE(LOCAL, "Async copy to host: %p ("FMT_SIZE") @ %p", host, count, acc.get());
    cl_event start;
    cl_int ret;

    buffer.toHost(reinterpret_cast<opencl::hpe::Mode &>(mode));
    ret = clEnqueueReadBuffer(stream, acc.get(), CL_FALSE,
            acc.offset(), count, host, 0, NULL, &start);
    CFATAL(ret == CL_SUCCESS, "Error copying to host: %d", ret);
    buffer.started(start, count);
#ifdef _MSC_VER
    ret = clFlush(stream);
    CFATAL(ret == CL_SUCCESS, "Error issuing read to accelerator: %d", ret);
#endif

    trace::ExitCurrentFunction();
    return error(ret);
}



}}}}}