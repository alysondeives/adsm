#include "config/common.h"
#include "core/Process.h"
#include "core/Context.h"
#include "gmac/init.h"
#include "os/posix/loader.h"
#include "trace/Tracer.h"
#include "util/Lock.h"

#include "config/order.h"

#include <pthread.h>


SYM(int, __pthread_create, pthread_t *__restrict, __const pthread_attr_t *, void *(*)(void *), void *);

void threadInit(void)
{
	LOAD_SYM(__pthread_create, pthread_create);
}

static void __attribute__((destructor())) gmacPthreadFini(void)
{
}

struct gmac_thread_t {
	void *(*__start_routine)(void *);
	void *__arg;
};

using __impl::core::Process;

static void *gmac_pthread(void *arg)
{
    gmac::trace::StartThread("CPU");
	gmac::enterGmac();
	gmac_thread_t *gthread = (gmac_thread_t *)arg;
    Process &proc = Process::getInstance();
    proc.initThread();
    gmac::trace::SetThreadState(gmac::trace::Running);
	gmac::exitGmac();
	void *ret = gthread->__start_routine(gthread->__arg);
	gmac::enterGmac();
    // Modes and Contexts already destroyed in Process destructor
    proc.finiThread();
	free(gthread);
    gmac::trace::SetThreadState(gmac::trace::Idle);
	gmac::exitGmac();
	return ret;
}

int pthread_create(pthread_t *__restrict __newthread,
                   __const pthread_attr_t *__restrict __attr,
                   void *(*__start_routine)(void *),
                   void *__restrict __arg)
{
    if(gmac::inGmac() == 1)
        return __pthread_create(__newthread, __attr, __start_routine, __arg);
        
	int ret = 0;
	gmac::enterGmac();
    TRACE(GLOBAL, "New POSIX thread");
	gmac_thread_t *gthread = (gmac_thread_t *)malloc(sizeof(gmac_thread_t));
	gthread->__start_routine = __start_routine;
	gthread->__arg = __arg;
	ret = __pthread_create(__newthread, __attr, gmac_pthread, (void *)gthread);
	gmac::exitGmac();
	return ret;
}
