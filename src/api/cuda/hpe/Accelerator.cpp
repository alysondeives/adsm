#include "Accelerator.h"
#include "Mode.h"

#include "core/Process.h"

namespace __impl { namespace cuda { namespace hpe {

#ifdef USE_MULTI_CONTEXT
util::Private<CUcontext> Accelerator::Ctx_;
#endif

void Switch::in(Mode &mode)
{
    mode.getAccelerator().pushContext();
}

void Switch::out(Mode &mode)
{
    mode.getAccelerator().popContext();
}

Accelerator::Accelerator(int n, CUdevice device) :
    gmac::core::hpe::Accelerator(n), device_(device)
#ifndef USE_MULTI_CONTEXT
#ifdef USE_VM
    , lastMode_(NULL)
#endif
    , ctx_(NULL)
#endif
{
#if CUDA_VERSION > 3010
    size_t size = 0;
#else
    unsigned int size = 0;
#endif
    CUresult ret = cuDeviceTotalMem(&size, device_);
    CFATAL(ret == CUDA_SUCCESS, "Unable to initialize CUDA device %d", ret);
    ret = cuDeviceComputeCapability(&major_, &minor_, device_);
    CFATAL(ret == CUDA_SUCCESS, "Unable to initialize CUDA device %d", ret);

    int val;

#if CUDA_VERSION > 3000
    ret = cuDeviceGetAttribute(&val, CU_DEVICE_ATTRIBUTE_PCI_BUS_ID, n);
    CFATAL(ret == CUDA_SUCCESS, "Unable to get attribute %d", ret);
    busId_ = val;
    ret = cuDeviceGetAttribute(&val, CU_DEVICE_ATTRIBUTE_PCI_DEVICE_ID, n);
    CFATAL(ret == CUDA_SUCCESS, "Unable to get attribute %d", ret);
    busAccId_ = val;
#endif
    ret = cuDeviceGetAttribute(&val, CU_DEVICE_ATTRIBUTE_INTEGRATED, n);
    CFATAL(ret == CUDA_SUCCESS, "Unable to get attribute %d", ret);
    integrated_ = (val != 0);

#ifndef USE_MULTI_CONTEXT
    CUcontext tmp;
    unsigned int flags = 0;
#if CUDA_VERSION >= 2020
    if(major_ >= 2 || (major_ == 1 && minor_ >= 1)) flags |= CU_CTX_MAP_HOST;
#else
    TRACE(LOCAL,"Host mapped memory not supported by the HW");
#endif

    ret = cuCtxCreate(&ctx_, flags, device_);
    CFATAL(ret == CUDA_SUCCESS, "Unable to create CUDA context %d", ret);
    ret = cuCtxPopCurrent(&tmp);
    CFATAL(ret == CUDA_SUCCESS, "Error setting up a new context %d", ret);
#else
#endif
}

Accelerator::~Accelerator()
{
#ifndef USE_MULTI_CONTEXT
    pushContext();
    ModuleVector::iterator i;
    for(i = modules_.begin(); i != modules_.end(); i++) {
        delete *i;
    }
    modules_.clear();
    popContext();
    CUresult ret = cuCtxDestroy(ctx_);
    ASSERTION(ret == CUDA_SUCCESS);
#endif
}

void Accelerator::init()
{
#ifdef USE_MULTI_CONTEXT
    util::Private<CUcontext>::init(Ctx_);
#endif
}

__impl::core::hpe::Mode *Accelerator::createMode(core::hpe::Process &proc)
{
    trace::EnterCurrentFunction();
    core::hpe::Mode *mode = ModeFactory::create(proc, *this);
    if (mode != NULL) {
        registerMode(*mode);
    }
    trace::ExitCurrentFunction();

    TRACE(LOCAL,"Creating Execution Mode %p to Accelerator", mode);
    return mode;
}

#ifdef USE_MULTI_CONTEXT
CUcontext
Accelerator::createCUcontext()
{
    trace::EnterCurrentFunction();
    CUcontext ctx, tmp;
    unsigned int flags = 0;
#if CUDA_VERSION >= 2020
    if(major_ >= 2 || (major_ == 1 && minor_ >= 1)) flags |= CU_CTX_MAP_HOST;
#else
    TRACE(LOCAL,"Host mapped memory not supported by the HW");
#endif
    CUresult ret = cuCtxCreate(&ctx, flags, device_);
    if(ret != CUDA_SUCCESS)
        FATAL("Unable to create CUDA context %d", ret);
    ret = cuCtxPopCurrent(&tmp);
    ASSERTION(ret == CUDA_SUCCESS);
    trace::ExitCurrentFunction();
    return ctx;
}

void
Accelerator::destroyCUcontext(CUcontext ctx)
{
    trace::EnterCurrentFunction();
    CFATAL(cuCtxDestroy(ctx) == CUDA_SUCCESS, "Error destroying CUDA context");
    trace::ExitCurrentFunction();
}

#endif

#ifdef USE_MULTI_CONTEXT
ModuleVector Accelerator::createModules()
{
    trace::EnterCurrentFunction();
    pushContext();
    ModuleVector modules = ModuleDescriptor::createModules();
    popContext();
    trace::ExitCurrentFunction();
    return modules;
}

void
Accelerator::destroyModules(ModuleVector & modules)
{
    trace::EnterCurrentFunction();
    pushContext();
    ModuleVector::iterator i;
    for(i = modules.begin(); i != modules.end(); i++)
        delete *i;
    modules.clear();
    popContext();
    trace::ExitCurrentFunction();
}

#else
ModuleVector *Accelerator::createModules()
{
    trace::EnterCurrentFunction();
    if(modules_.empty()) {
        pushContext();
        modules_ = ModuleDescriptor::createModules();
        popContext();
    }
    trace::ExitCurrentFunction();
    return &modules_;
}
#endif

const accptr_t &Accelerator::map(gmacError_t &err, hostptr_t src, size_t size, unsigned align) 
{
    trace::EnterCurrentFunction();
#if CUDA_VERSION >= 3020
    size_t gpuSize = size;
#else
    unsigned gpuSize = unsigned(size);
#endif
    if(align > 1) {
        gpuSize += align;
    }
    CUdeviceptr ptr = 0;
    pushContext();
    CUresult ret = cuMemAlloc(&ptr, gpuSize);
    popContext();
    if(ret != CUDA_SUCCESS) {
        trace::ExitCurrentFunction();
        err = error(ret);
        return nullaccptr;
    }

    CUdeviceptr gpuPtr = ptr;
    if(gpuPtr % align) {
        gpuPtr += align - (gpuPtr % align);
    }
    accptr_t dst(gpuPtr);
#ifndef USE_MULTI_CONTEXT
    dst.pasId_ = id_;
#endif

    const accptr_t &ref = allocations_.insert(src, dst, size);

    alignMap_.lockWrite();

    alignMap_.insert(AlignmentMap::value_type(gpuPtr, ptr));
    alignMap_.unlock();
    TRACE(LOCAL,"Allocating device memory: %p (originally %p) - "FMT_SIZE" (originally "FMT_SIZE") bytes (alignment %u)", (void *) dst, ptr, gpuSize, size, align);
    trace::ExitCurrentFunction();
    err = error(ret);
    return ref;
}

gmacError_t Accelerator::unmap(hostptr_t host, size_t size)
{
    trace::EnterCurrentFunction();
    ASSERTION(host != NULL);

    size_t s;

    const accptr_t &addr = allocations_.find(host, s);
    ASSERTION(addr != nullaccptr);
    ASSERTION(s == size);

    TRACE(LOCAL, "Releasing accelerator memory @ %p", addr.get());

    AlignmentMap::iterator i;
    alignMap_.lockWrite();
    i = alignMap_.find(addr);
    allocations_.erase(host, size);
    if (i == alignMap_.end()) {
        alignMap_.unlock();
        trace::ExitCurrentFunction();
        return gmacErrorInvalidValue;
    }
    CUdeviceptr device = i->second;
    alignMap_.erase(i);
    alignMap_.unlock();
    pushContext();
    trace::SetThreadState(trace::Wait);
    CUresult ret = cuMemFree(device);
    trace::SetThreadState(trace::Running);
    popContext();
    trace::ExitCurrentFunction();
    return error(ret);
}

gmacError_t Accelerator::memset(const accptr_t &addr, int c, size_t size)
{
    trace::EnterCurrentFunction();
    CUresult ret = CUDA_SUCCESS;
    pushContext();
    if(size % 4 == 0) {
        int seed = c | (c << 8) | (c << 16) | (c << 24);
#if CUDA_VERSION >= 3020
        ret = cuMemsetD32(addr, seed, size / 4);
#else
        ret = cuMemsetD32(addr, seed, unsigned(size / 4));
#endif
    } else if(size % 2) {
        short s = (short) c & 0xffff;
        short seed = s | (s << 8);
#if CUDA_VERSION >= 3020
        ret = cuMemsetD16(addr, seed, size / 2);
#else
        ret = cuMemsetD16(addr, seed, unsigned(size / 2));
#endif
    } else {
#if CUDA_VERSION >= 3020
        ret = cuMemsetD8(addr, (uint8_t)(c & 0xff), size);
#else
        ret = cuMemsetD8(addr, (uint8_t)(c & 0xff), unsigned(size));
#endif
    }
    popContext();
    trace::ExitCurrentFunction();
    return error(ret);
}

gmacError_t Accelerator::sync()
{
    trace::EnterCurrentFunction();
    pushContext();
    CUresult ret = cuCtxSynchronize();
    popContext();
    trace::ExitCurrentFunction();
    return error(ret);
}

gmacError_t Accelerator::hostAlloc(hostptr_t *addr, size_t size)
{
    trace::EnterCurrentFunction();
#if CUDA_VERSION >= 2020
    pushContext();
    CUresult ret = cuMemHostAlloc((void **) addr, size, CU_MEMHOSTALLOC_PORTABLE | CU_MEMHOSTALLOC_DEVICEMAP);
    popContext();
#else
	CUresult ret = CUDA_ERROR_OUT_OF_MEMORY;
#endif
    trace::ExitCurrentFunction();
    return error(ret);
}

gmacError_t Accelerator::hostFree(hostptr_t addr)
{
    trace::EnterCurrentFunction();
#if CUDA_VERSION >= 2020
    pushContext();
    CUresult r = cuMemFreeHost(addr);
    popContext();
#else
    CUresult r = CUDA_ERROR_OUT_OF_MEMORY;
#endif
    trace::ExitCurrentFunction();
    return error(r);
}

const accptr_t &Accelerator::hostMap(const hostptr_t addr)
{
    trace::EnterCurrentFunction();
#if CUDA_VERSION >= 2020
    CUdeviceptr device;
    pushContext();
    CUresult ret = cuMemHostGetDevicePointer(&device, addr, 0);
    if(ret == CUDA_SUCCESS) hostMem_.push_back(accptr_t(device));
    const accptr_t &ref = (ret == CUDA_SUCCESS) ? hostMem_.back() : nullaccptr;
    popContext();
#else
    const accptr_t &ref = nullaccptr;
    CUresult ret = CUDA_ERROR_OUT_OF_MEMORY;
#endif
    trace::ExitCurrentFunction();
    return ref;
}

gmacError_t Accelerator::copyToAccelerator(const accptr_t &acc, const hostptr_t host, size_t size, core::hpe::Mode &mode)
{
    trace::EnterCurrentFunction();
    TRACE(LOCAL,"Copy to accelerator: %p -> %p ("FMT_SIZE")", host, (void *) acc, size);
    trace::SetThreadState(trace::Wait);
    pushContext();
#if CUDA_VERSION >= 3020
    CUresult ret = cuMemcpyHtoD(acc, host, size);
#else
	CUresult ret = cuMemcpyHtoD(CUdeviceptr(acc), host, unsigned(size));
#endif
    popContext();
    trace::SetThreadState(trace::Running);
    // TODO: add communication
    trace::ExitCurrentFunction();
    return error(ret);
}

gmacError_t Accelerator::copyToAcceleratorAsync(const accptr_t &acc, IOBuffer &buffer, size_t bufferOff, size_t count, core::Mode &mode, CUstream stream)
{
    trace::EnterCurrentFunction();
    uint8_t *host = buffer.addr() + bufferOff;
    TRACE(LOCAL,"Async copy to accelerator: %p -> %p ("FMT_SIZE")", host, (void *) acc, count);
    pushContext();

    buffer.toAccelerator(dynamic_cast<cuda::Mode &>(mode), stream);
#if CUDA_VERSION >= 3020
    CUresult ret = cuMemcpyHtoDAsync(acc, host, count, stream);
#else
    CUresult ret = cuMemcpyHtoDAsync(CUdeviceptr(acc), host, unsigned(count), stream);
#endif
    buffer.started();
    popContext();
    trace::ExitCurrentFunction();
    return error(ret);
}

gmacError_t Accelerator::copyToHost(hostptr_t host, const accptr_t &acc, size_t size, core::hpe::Mode &mode)
{
    trace::EnterCurrentFunction();
    TRACE(LOCAL,"Copy to host: %p -> %p ("FMT_SIZE")", (void *) acc, host, size);
    trace::SetThreadState(trace::Wait);
    pushContext();
#if CUDA_VERSION >= 3020
    CUresult ret = cuMemcpyDtoH(host, acc, size);
#else
	CUresult ret = cuMemcpyDtoH(host, acc, unsigned(size));
#endif
    popContext();
    trace::SetThreadState(trace::Running);
    trace::ExitCurrentFunction();
    return error(ret);
}

gmacError_t Accelerator::copyToHostAsync(IOBuffer &buffer, size_t bufferOff, const accptr_t &acc, size_t count, core::Mode &mode, CUstream stream)
{
    trace::EnterCurrentFunction();
    uint8_t *host = buffer.addr() + bufferOff;
    TRACE(LOCAL,"Async copy to host: %p -> %p ("FMT_SIZE")", (void *) acc, host, count);
    pushContext();
    buffer.toHost(dynamic_cast<cuda::Mode &>(mode), stream);
#if CUDA_VERSION >= 3020
    CUresult ret = cuMemcpyDtoHAsync(host, acc, count, stream);
#else
	CUresult ret = cuMemcpyDtoHAsync(host, acc, unsigned(count), stream);
#endif
    buffer.started();
    popContext();
    trace::ExitCurrentFunction();
    return error(ret);
}

gmacError_t Accelerator::copyAccelerator(const accptr_t &dst, const accptr_t &src, size_t size)
{
    trace::EnterCurrentFunction();
    TRACE(LOCAL,"Copy accelerator-accelerator: %p -> %p ("FMT_SIZE")", (void *) src, (void *) dst, size);
    pushContext();
#if CUDA_VERSION >= 3020
    CUresult ret = cuMemcpyDtoD(dst, src, size);
#else
	CUresult ret = cuMemcpyDtoD(dst, src, unsigned(size));
#endif
    popContext();
    trace::ExitCurrentFunction();
    return error(ret);
}


void Accelerator::memInfo(size_t &free, size_t &total) const
{
    pushContext();

#if CUDA_VERSION > 3010
    CUresult ret = cuMemGetInfo(&free, &total);
#else
    CUresult ret = cuMemGetInfo((unsigned int *) &free, (unsigned int *) &total);
#endif
    CFATAL(ret == CUDA_SUCCESS, "Error getting memory info");
    popContext();
}

}}}
