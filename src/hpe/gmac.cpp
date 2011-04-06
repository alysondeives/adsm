#include <cstdlib>

#ifdef USE_CUDA
#include "include/gmac/cuda.h"
#else
#include "include/gmac/opencl.h"
#endif

#include "config/config.h"
#include "config/order.h"

#include "util/Logger.h"

#include "core/IOBuffer.h"

#include "core/hpe/Mode.h"
#include "core/hpe/Kernel.h"
#include "core/hpe/Process.h"

#include "memory/Manager.h"
#include "memory/Allocator.h"

#include "trace/Tracer.h"

#if defined(GMAC_DLL)
#include "init.h"
#endif

#if defined(__GNUC__)
#define RETURN_ADDRESS __builtin_return_address(0)
#elif defined(_MSC_VER)
extern "C" void * _ReturnAddress(void);
#pragma intrinsic(_ReturnAddress)
#define RETURN_ADDRESS _ReturnAddress()
static long getpagesize (void) {
    static long pagesize = 0;
    if(pagesize == 0) {
        SYSTEM_INFO systemInfo;
        GetSystemInfo(&systemInfo);
        pagesize = systemInfo.dwPageSize;
    }
    return pagesize;
}
#endif

using __impl::util::params::ParamBlockSize;
using __impl::util::params::ParamAutoSync;

#if 0
gmacError_t
gmacClear(gmacKernel_t k)
{
    gmacError_t ret = gmacSuccess;
    gmac::enterGmac();
    enterFunction(FuncGmacClear);
    gmac::Kernel *kernel = core::Mode::getCurrent()->kernel(k);
    if (kernel == NULL) ret = gmacErrorInvalidValue;
    else kernel->clear();
    exitFunction();
    gmac::exitGmac();
    return ret;
}

gmacError_t
gmacBind(void * obj, gmacKernel_t k)
{
    gmacError_t ret = gmacSuccess;
    gmac::enterGmac();
    enterFunction(FuncGmacBind);
    gmac::Kernel *kernel = core::Mode::getCurrent()->kernel(k);

    if (kernel == NULL) ret = gmacErrorInvalidValue;
    else ret = kernel->bind(obj);
    exitFunction();
    gmac::exitGmac();
    return ret;
}

gmacError_t
gmacUnbind(void * obj, gmacKernel_t k)
{
    gmacError_t ret = gmacSuccess;
    gmac::enterGmac();
    enterFunction(FuncGmacUnbind);
    gmac::Kernel  * kernel = core::Mode::getCurrent()->kernel(k);
    if (kernel == NULL) ret = gmacErrorInvalidValue;
    else ret = kernel->unbind(obj);
	exitFunction();
	gmac::exitGmac();
    return ret;
}
#endif

unsigned APICALL gmacGetNumberOfAccelerators()
{
    unsigned ret;
	gmac::enterGmac();
    gmac::trace::EnterCurrentFunction();
    __impl::core::hpe::Process &proc = __impl::core::Process::getInstance<gmac::core::hpe::Process>();
    ret = unsigned(proc.nAccelerators());
    gmac::trace::ExitCurrentFunction();
	gmac::exitGmac();
	return ret;
}

size_t APICALL gmacGetFreeMemory()
{
    gmacError_t ret = gmacSuccess;
    gmac::enterGmacExclusive();
    gmac::trace::EnterCurrentFunction();
    size_t free;
    size_t total;
    __impl::core::hpe::Mode &mode = gmac::core::hpe::Mode::getCurrent();
    mode.memInfo(free, total);
    gmac::trace::ExitCurrentFunction();
    gmac::exitGmac();
    return free;
}


gmacError_t APICALL gmacMigrate(unsigned acc)
{
	gmacError_t ret = gmacSuccess;
	gmac::enterGmacExclusive();
    gmac::trace::EnterCurrentFunction();
    __impl::core::hpe::Process &proc = __impl::core::Process::getInstance<gmac::core::hpe::Process>();
    if(gmac::core::hpe::Mode::hasCurrent()) {
        ret = proc.migrate(gmac::core::hpe::Mode::getCurrent(), acc);
    } else {
        if (proc.createMode(acc) == NULL) {
            ret = gmacErrorUnknown;
        } 
    }
    gmac::trace::ExitCurrentFunction();
	gmac::exitGmac();
	return ret;
}


gmacError_t APICALL gmacMemoryMap(void *cpuPtr, size_t count, GmacProtection prot)
{
#if 0
    gmacError_t ret = gmacSuccess;
    if (count == 0) {
        return ret;
    }
	gmac::enterGmac();
    gmac::trace::EnterCurrentFunction();
    gmac::memory::Manager &manager = gmac::memory::Manager::getInstance();
    // TODO Remove alignment constraints
    count = (int(count) < getpagesize())? getpagesize(): count;
    ret = manager.map(cpuPtr, count, prot);
    gmac::trace::ExitCurrentFunction();
	gmac::exitGmac();
    return ret;
#endif
    return gmacErrorFeatureNotSupported;
}


gmacError_t APICALL gmacMemoryUnmap(void *cpuPtr, size_t count)
{
#if 0
    gmacError_t ret = gmacSuccess;
    if (count == 0) {
        return ret;
    }
	gmac::enterGmac();
    gmac::trace::EnterCurrentFunction();
    gmac::memory::Manager &manager = gmac::memory::Manager::getInstance();
    // TODO Remove alignment constraints
    count = (int(count) < getpagesize())? getpagesize(): count;
    ret = manager.unmap(cpuPtr, count);
    gmac::trace::ExitCurrentFunction();
	gmac::exitGmac();
    return ret;
#endif
    return gmacErrorFeatureNotSupported;
}


gmacError_t APICALL gmacMalloc(void **cpuPtr, size_t count)
{
    gmacError_t ret = gmacSuccess;
    if (count == 0) {
        *cpuPtr = NULL;
        return ret;
    }
	gmac::enterGmac();
    gmac::trace::EnterCurrentFunction();
    __impl::memory::Allocator &allocator = __impl::memory::Allocator::getInstance();
    if(count < (ParamBlockSize / 2)) {
        *cpuPtr = allocator.alloc(gmac::core::hpe::Mode::getCurrent(), count, hostptr_t(RETURN_ADDRESS));
    }
    else {
    	gmac::memory::Manager &manager = gmac::memory::Manager::getInstance();
	    count = (int(count) < getpagesize())? getpagesize(): count;
	    ret = manager.alloc(gmac::core::hpe::Mode::getCurrent(), (hostptr_t *) cpuPtr, count);
    }
    gmac::trace::ExitCurrentFunction();
	gmac::exitGmac();
	return ret;
}

gmacError_t APICALL gmacGlobalMalloc(void **cpuPtr, size_t count, GmacGlobalMallocType hint)
{
    gmacError_t ret = gmacSuccess;
    if(count == 0) {
        *cpuPtr = NULL;
        return ret;
    }
    gmac::enterGmac();
    gmac::trace::EnterCurrentFunction();
	count = (count < (size_t)getpagesize()) ? (size_t)getpagesize(): count;
	ret = gmac::memory::Manager::getInstance().globalAlloc(gmac::core::hpe::Mode::getCurrent(), (hostptr_t *)cpuPtr, count, hint);
    gmac::trace::ExitCurrentFunction();
    gmac::exitGmac();
    return ret;
}

gmacError_t APICALL gmacFree(void *cpuPtr)
{
    gmacError_t ret = gmacSuccess;
	gmac::enterGmac();
    gmac::trace::EnterCurrentFunction();
    __impl::memory::Allocator &allocator = __impl::memory::Allocator::getInstance();
    if(allocator.free(gmac::core::hpe::Mode::getCurrent(), hostptr_t(cpuPtr)) == false) {
    	gmac::memory::Manager &manager = gmac::memory::Manager::getInstance();
        ret = manager.free(gmac::core::hpe::Mode::getCurrent(), hostptr_t(cpuPtr));
    }
    gmac::trace::ExitCurrentFunction();
	gmac::exitGmac();
	return ret;
}

__gmac_accptr_t APICALL gmacPtr(const void *ptr)
{
    accptr_t ret = accptr_t(0);
    gmac::enterGmac();
    ret = __impl::memory::Manager::getInstance().translate(gmac::core::hpe::Mode::getCurrent(), hostptr_t(ptr));
    gmac::exitGmac();
    TRACE(GLOBAL, "Translate %p to %p", ptr, ret.get());
    return ret.get();
}

gmacError_t APICALL gmacLaunch(gmacKernel_t k)
{
    gmac::enterGmac();
    __impl::core::hpe::Mode &mode = gmac::core::hpe::Mode::getCurrent();
    gmac::memory::Manager &manager = gmac::memory::Manager::getInstance();
    gmac::trace::EnterCurrentFunction();
    __impl::core::hpe::KernelLaunch &launch = mode.launch(k);

    gmacError_t ret = gmacSuccess;
    TRACE(GLOBAL, "Flush the memory used in the kernel");
    CFATAL(manager.releaseObjects(gmac::core::hpe::Mode::getCurrent()) == gmacSuccess, "Error releasing objects");

    TRACE(GLOBAL, "Kernel Launch");
    ret = mode.execute(launch);

    if (ParamAutoSync == true) {
        TRACE(GLOBAL, "Waiting for Kernel to complete");
        ret = manager.acquireObjects(gmac::core::hpe::Mode::getCurrent());
        CFATAL(ret == gmacSuccess, "Error waiting for kernel");
    }

    delete &launch;
    gmac::trace::ExitCurrentFunction();
    gmac::exitGmac();

    return ret;
}

gmacError_t APICALL gmacThreadSynchronize()
{
	gmac::enterGmac();
    gmac::trace::EnterCurrentFunction();

	gmacError_t ret = gmacSuccess;
    if (ParamAutoSync == false) {
        TRACE(GLOBAL, "Memory Sync");
        gmac::memory::Manager &manager = gmac::memory::Manager::getInstance();
        ret = manager.acquireObjects(gmac::core::hpe::Mode::getCurrent());
    }

    gmac::trace::ExitCurrentFunction();
	gmac::exitGmac();
	return ret;
}

gmacError_t APICALL gmacGetLastError()
{
	gmac::enterGmac();
	gmacError_t ret = gmac::core::hpe::Mode::getCurrent().error();
	gmac::exitGmac();
	return ret;
}

void * APICALL gmacMemset(void *s, int c, size_t size)
{
    gmac::enterGmac();
    void *ret = s;
    gmac::memory::Manager &manager = gmac::memory::Manager::getInstance();
    manager.memset(gmac::core::hpe::Mode::getCurrent(), hostptr_t(s), c, size);
	gmac::exitGmac();
    return ret;
}

void * APICALL gmacMemcpy(void *dst, const void *src, size_t size)
{
	gmac::enterGmac();
	void *ret = dst;

	// Locate memory regions (if any)
    __impl::core::Process &proc = __impl::core::Process::getInstance();
    __impl::core::Mode *dstMode = proc.owner(hostptr_t(dst), size);
    __impl::core::Mode *srcMode = proc.owner(hostptr_t(src), size);
    if (dstMode == NULL && srcMode == NULL) {
        gmac::exitGmac();
        return ::memcpy(dst, src, size);
    }
	gmac::memory::Manager &manager = gmac::memory::Manager::getInstance();
    manager.memcpy(gmac::core::hpe::Mode::getCurrent(), hostptr_t(dst), hostptr_t(src), size);

	gmac::exitGmac();
	return ret;
}

void APICALL gmacSend(THREAD_T id)
{
    gmac::enterGmac();
    __impl::core::hpe::Process &proc = __impl::core::Process::getInstance<gmac::core::hpe::Process>();
    proc.send((THREAD_T)id);
    gmac::exitGmac();
}

void APICALL gmacReceive()
{
    gmac::enterGmac();
    __impl::core::hpe::Process &proc = __impl::core::Process::getInstance<gmac::core::hpe::Process>();
    proc.receive();
    gmac::exitGmac();
}

void APICALL gmacSendReceive(THREAD_T id)
{
	gmac::enterGmac();
    __impl::core::hpe::Process &proc = __impl::core::Process::getInstance<gmac::core::hpe::Process>();
	proc.sendReceive((THREAD_T)id);
	gmac::exitGmac();
}

void APICALL gmacCopy(THREAD_T id)
{
    gmac::enterGmac();
    __impl::core::hpe::Process &proc = __impl::core::Process::getInstance<gmac::core::hpe::Process>();
    proc.copy((THREAD_T)id);
    gmac::exitGmac();
}