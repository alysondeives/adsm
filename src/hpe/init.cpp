#include "core/hpe/Mode.h"
#include "core/hpe/Process.h"

#include "memory/Allocator.h"
#include "memory/Handler.h"
#include "memory/Manager.h"
#include "memory/allocator/Slab.h"

#include "util/Parameter.h"
#include "util/Logger.h"

#include "trace/Tracer.h"

#include "init.h"

static gmac::core::hpe::Process *Process_ = NULL;
static gmac::memory::Manager *Manager_ = NULL;
static __impl::memory::Allocator *Allocator_ = NULL;

extern void CUDA(gmac::core::hpe::Process &);
extern void OpenCL(gmac::core::hpe::Process &);

void initGmac(void)
{
    enterGmac();

    /* Call initialization of interpose libraries */
#if defined(POSIX)
    osInit();
    threadInit();
#endif
    stdcInit();

#ifdef USE_MPI
    mpiInit();
#endif

    TRACE(GLOBAL, "Using %s memory manager", __impl::util::params::ParamProtocol);
    TRACE(GLOBAL, "Using %s memory allocator", __impl::util::params::ParamAllocator);

    // Set the entry and exit points for Manager
    __impl::memory::Handler::setEntry(enterGmac);
    __impl::memory::Handler::setExit(exitGmac);

    // Process is a singleton class. The only allowed instance is Proc_
    TRACE(GLOBAL, "Initializing process");
    Process_ = new gmac::core::hpe::Process();

    TRACE(GLOBAL, "Initializing memory");
    Manager_ = new gmac::memory::Manager(*Process_);
    Allocator_ = new __impl::memory::allocator::Slab(*Manager_);

#if defined(USE_CUDA)
    TRACE(GLOBAL, "Initializing CUDA");
    CUDA(*Process_);
#endif
#if defined(USE_OPENCL)
    TRACE(GLOBAL, "Initializing OpenCL");
    OpenCL(*Process_);
#endif

    exitGmac();
}


namespace __impl {
    namespace core {
        namespace hpe {
            Mode &getCurrentMode() { return Process_->getCurrentMode(); }
            Process &getProcess() { return *Process_; } 
        }
        Mode &getMode(Mode &mode) { return Process_->getCurrentMode(); }
        Process &getProcess() { return *Process_; }
    }
    namespace memory {
        Manager &getManager() { return *Manager_; }
        Allocator &getAllocator() { return *Allocator_; }
    }
}


// We cannot call the destructor because CUDA might have
// been uninitialized
#if 0
DESTRUCTOR(fini);
static void fini(void)
{
	enterGmac();
    if(AtomicInc(gmacFini__) == 0) {
        Allocator_->destroy();
        Manager_->destroy();
        Process_->destroy();
        delete inGmacLock;
    }
	// TODO: Clean-up logger
}
#endif


#if defined(_WIN32)
#include <windows.h>

static void InitThread()
{
	gmac::trace::StartThread("CPU");
	enterGmac();
	getProcess().initThread();
    gmac::trace::SetThreadState(__impl::trace::Running);
	exitGmac();
}

static void FiniThread()
{
	enterGmac();
	gmac::trace::SetThreadState(gmac::trace::Idle);	
	// Modes and Contexts already destroyed in Process destructor
	getProcess().finiThread();
	exitGmac();
}

// DLL entry function (called on load, unload, ...)
BOOL APIENTRY DllMain(HANDLE /*hModule*/, DWORD dwReason, LPVOID /*lpReserved*/)
{
	switch(dwReason) {
		case DLL_PROCESS_ATTACH:
            break;
		case DLL_PROCESS_DETACH:
			// Really ugly hack -- Stupid windows do not allow calling DLLs from static
			// destructors, so we cannot release resources at termination time
			AtomicInc(gmacFini__);
			break;
		case DLL_THREAD_ATTACH:
			InitThread();
			break;
		case DLL_THREAD_DETACH:			
			FiniThread();
			break;
	};
    return TRUE;
}


#endif


/* vim:set backspace=2 tabstop=4 shiftwidth=4 textwidth=120 foldmethod=marker expandtab: */
