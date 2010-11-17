#include "Accelerator.h"
#include "Mode.h"

#include "core/Process.h"

namespace gmac { namespace cuda {

#ifdef USE_MULTI_CONTEXT
gmac::util::Private<CUcontext> Accelerator::_Ctx;
#endif

void Switch::in()
{
    Mode::current().accelerator().pushContext();
}

void Switch::out()
{
    Mode::current().accelerator().popContext();
}

Accelerator::Accelerator(int n, CUdevice device) :
	gmac::core::Accelerator(n), device_(device),
    _ctx(NULL)
{
#if CUDA_VERSION > 3010
    size_t size = 0;
#else
    unsigned int size = 0;
#endif
    CUresult ret = cuDeviceTotalMem(&size, device_);
    CFATAL(ret == CUDA_SUCCESS, "Unable to initialize CUDA %d", ret);
    ret = cuDeviceComputeCapability(&_major, &_minor, device_);
    CFATAL(ret == CUDA_SUCCESS, "Unable to initialize CUDA %d", ret);
    memory_ = size;

#ifndef USE_MULTI_CONTEXT
    CUcontext tmp;
    unsigned int flags = 0;
#if CUDA_VERSION >= 2020
    if(_major >= 2 || (_major == 1 && _minor >= 1)) flags |= CU_CTX_MAP_HOST;
#else
    TRACE(LOCAL,"Host mapped memory not supported by the HW");
#endif

    int val;
#if CUDA_VERSION > 3000
    ret = cuDeviceGetAttribute(&val, CU_DEVICE_ATTRIBUTE_PCI_BUS_ID, n);
    CFATAL(ret == CUDA_SUCCESS, "Unable to get attribute %d", ret);
    busId_ = val;
    ret = cuDeviceGetAttribute(&val, CU_DEVICE_ATTRIBUTE_PCI_DEVICE_ID, n);
    CFATAL(ret == CUDA_SUCCESS, "Unable to get attribute %d", ret);
    busDevId_ = val;
#endif
    ret = cuDeviceGetAttribute(&val, CU_DEVICE_ATTRIBUTE_INTEGRATED, n);
    CFATAL(ret == CUDA_SUCCESS, "Unable to get attribute %d", ret);
    integrated_ = (val != 0);


    ret = cuCtxCreate(&_ctx, flags, device_);
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
    for(i = _modules.begin(); i != _modules.end(); i++) {
        delete *i;
    }
    _modules.clear();
    popContext();
    ASSERTION(cuCtxDestroy(_ctx) == CUDA_SUCCESS);
#endif
}

void Accelerator::init()
{
#ifdef USE_MULTI_CONTEXT
    gmac::util::Private<CUcontext>::init(_Ctx);
#endif
}

gmac::core::Mode *Accelerator::createMode(gmac::core::Process &proc)
{
    gmac::trace::EnterCurrentFunction();
	Mode *mode = new Mode(proc, *this);
    gmac::trace::ExitCurrentFunction();
	TRACE(LOCAL,"Creating Execution Mode %p to Accelerator", mode);
    return mode;
}

void Accelerator::registerMode(gmac::core::Mode &mode)
{
    Mode &_mode = static_cast<Mode &>(mode);
	TRACE(LOCAL,"Registering Execution Mode %p to Accelerator", &_mode);
    gmac::trace::EnterCurrentFunction();
	_queue.insert(&_mode);
    load_++;
    gmac::trace::ExitCurrentFunction();
}

void Accelerator::unregisterMode(gmac::core::Mode &mode)
{
    Mode &_mode = static_cast<Mode &>(mode);
	TRACE(LOCAL,"Unregistering Execution Mode %p", &_mode);
    gmac::trace::EnterCurrentFunction();
	std::set<Mode *>::iterator c = _queue.find(&_mode);
	ASSERTION(c != _queue.end());
	_queue.erase(c);
    load_--;
    gmac::trace::ExitCurrentFunction();
}


#ifdef USE_MULTI_CONTEXT
CUcontext
Accelerator::createCUcontext()
{
    gmac::trace::EnterCurrentFunction();
    CUcontext ctx, tmp;
    unsigned int flags = 0;
#if CUDA_VERSION >= 2020
    if(_major >= 2 || (_major == 1 && _minor >= 1)) flags |= CU_CTX_MAP_HOST;
#else
    TRACE(LOCAL,"Host mapped memory not supported by the HW");
#endif
    CUresult ret = cuCtxCreate(&ctx, flags, device_);
    if(ret != CUDA_SUCCESS)
        FATAL("Unable to create CUDA context %d", ret);
    ret = cuCtxPopCurrent(&tmp);
    ASSERTION(ret == CUDA_SUCCESS);
    gmac::trace::ExitCurrentFunction();
    return ctx;
}

void
Accelerator::destroyCUcontext(CUcontext ctx)
{
    gmac::trace::EnterCurrentFunction();
    CFATAL(cuCtxDestroy(ctx) == CUDA_SUCCESS, "Error destroying CUDA context");
    gmac::trace::ExitCurrentFunction();
}

#endif

#ifdef USE_MULTI_CONTEXT
ModuleVector Accelerator::createModules()
{
    gmac::trace::EnterCurrentFunction();
    pushContext();
    ModuleVector modules = ModuleDescriptor::createModules();
    popContext();
    gmac::trace::ExitCurrentFunction();
    return modules;
}

void
Accelerator::destroyModules(ModuleVector & modules)
{
    gmac::trace::EnterCurrentFunction();
    pushContext();
    ModuleVector::iterator i;
    for(i = modules.begin(); i != modules.end(); i++)
        delete *i;
    modules.clear();
    popContext();
    gmac::trace::ExitCurrentFunction();
}

#else
ModuleVector *Accelerator::createModules()
{
    gmac::trace::EnterCurrentFunction();
    if(_modules.empty()) {
        pushContext();
        _modules = ModuleDescriptor::createModules();
        popContext();
    }
    gmac::trace::ExitCurrentFunction();
    return &_modules;
}
#endif

gmacError_t Accelerator::malloc(void **addr, size_t size, unsigned align) 
{
    gmac::trace::EnterCurrentFunction();
    ASSERTION(addr != NULL);
    *addr = NULL;
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
        gmac::trace::ExitCurrentFunction();
        return error(ret);
    }
    CUdeviceptr gpuPtr = ptr;
    if(gpuPtr % align) {
        gpuPtr += align - (gpuPtr % align);
    }
    *addr = (void *)gpuPtr;
    _alignMap.lockWrite();
    _alignMap.insert(AlignmentMap::value_type(gpuPtr, ptr));
    _alignMap.unlock();
    TRACE(LOCAL,"Allocating device memory: %p (originally %p) - "FMT_SIZE" (originally "FMT_SIZE") bytes (alignment %u)", *addr, ptr, gpuSize, size, align);
    gmac::trace::ExitCurrentFunction();
    return error(ret);
}

gmacError_t Accelerator::free(void *addr)
{
    gmac::trace::EnterCurrentFunction();
    ASSERTION(addr != NULL);
    AlignmentMap::const_iterator i;
    CUdeviceptr gpuPtr = gpuAddr(addr);
    _alignMap.lockRead();
    i = _alignMap.find(gpuPtr);
    if (i == _alignMap.end()) {
        _alignMap.unlock();
        gmac::trace::ExitCurrentFunction();
        return gmacErrorInvalidValue;
    }
    _alignMap.unlock();
    pushContext();
    CUresult ret = cuMemFree(i->second);
    popContext();
    gmac::trace::ExitCurrentFunction();
    return error(ret);
}

gmacError_t Accelerator::memset(void *addr, int c, size_t size)
{
    gmac::trace::EnterCurrentFunction();
    CUresult ret = CUDA_SUCCESS;
    pushContext();
    if(size % 4 == 0) {
        int seed = c | (c << 8) | (c << 16) | (c << 24);
#if CUDA_VERSION >= 3020
		ret = cuMemsetD32(gpuAddr(addr), seed, size / 4);
#else
		ret = cuMemsetD32(gpuAddr(addr), seed, unsigned(size / 4));
#endif
    } else if(size % 2) {
		short s = (short) c & 0xffff;
        short seed = s | (s << 8);
#if CUDA_VERSION >= 3020
        ret = cuMemsetD16(gpuAddr(addr), seed, size / 2);
#else
		ret = cuMemsetD16(gpuAddr(addr), seed, unsigned(size / 2));
#endif
    } else {
#if CUDA_VERSION >= 3020
		ret = cuMemsetD8(gpuAddr(addr), (uint8_t)(c & 0xff), size);
#else
		ret = cuMemsetD8(gpuAddr(addr), (uint8_t)(c & 0xff), unsigned(size));
#endif
	}
    popContext();
    gmac::trace::ExitCurrentFunction();
    return error(ret);
}

gmacError_t Accelerator::sync()
{
    gmac::trace::EnterCurrentFunction();
    pushContext();
    CUresult ret = cuCtxSynchronize();
    popContext();
    gmac::trace::ExitCurrentFunction();
    return error(ret);
}

gmacError_t Accelerator::hostAlloc(void **addr, size_t size)
{
	gmac::trace::EnterCurrentFunction();
#if CUDA_VERSION >= 2020
    pushContext();
    CUresult ret = cuMemHostAlloc(addr, size, CU_MEMHOSTALLOC_PORTABLE | CU_MEMHOSTALLOC_DEVICEMAP);
    popContext();
#else
	CUresult ret = CUDA_ERROR_OUT_OF_MEMORY;
#endif
    gmac::trace::ExitCurrentFunction();
    return error(ret);
}

gmacError_t Accelerator::hostFree(void *addr)
{
    gmac::trace::EnterCurrentFunction();
#if CUDA_VERSION >= 2020
    pushContext();
    CUresult r = cuMemFreeHost(addr);
    popContext();
#else
	CUresult r = CUDA_ERROR_OUT_OF_MEMORY;
#endif
    gmac::trace::ExitCurrentFunction();
    return error(r);
}

void *Accelerator::hostMap(void *addr)
{
    gmac::trace::EnterCurrentFunction();
#if CUDA_VERSION >= 2020
    CUdeviceptr device;
    pushContext();
    CUresult ret = cuMemHostGetDevicePointer(&device, addr, 0);
    popContext();
#else
	CUresult ret = CUDA_ERROR_OUT_OF_MEMORY;
#endif
    if(ret != CUDA_SUCCESS) device = 0;
    gmac::trace::ExitCurrentFunction();
    return (void *)device;
}

void Accelerator::memInfo(size_t *free, size_t *total) const
{
    pushContext();
    size_t fakeFree;
    size_t fakeTotal;
    if (!free)  free  = &fakeFree;
    if (!total) total = &fakeTotal;

#if CUDA_VERSION > 3010
    CUresult ret = cuMemGetInfo(free, total);
#else
    CUresult ret = cuMemGetInfo((unsigned int *)free, (unsigned int *)total);
#endif
    CFATAL(ret == CUDA_SUCCESS, "Error getting memory info");
    popContext();
}

}}
