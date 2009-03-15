#include <gmac.h>
#include <dlfcn.h>
#include <pthread.h>

#include <common/debug.h>

#ifdef PARAVER
#include <gmac/paraver.h>
#endif

typedef int (*pthread_create_t)(pthread_t *__restrict, __const pthread_attr_t *, void *(*)(void *), void *);

static pthread_create_t _pthread_create = NULL;

static void __attribute__((constructor)) gmacPthreadInit(void)
{
	TRACE("Thread Redirection");
	_pthread_create = (pthread_create_t)dlsym(RTLD_NEXT, "pthread_create");
}

typedef struct {
	void *(*__start_routine)(void *);
	void *__arg;
} gmac_thread_t;

static gmac_thread_t gthread;
static void *gmac_pthread(void *arg) 
{
	gmac_thread_t *gthread = (gmac_thread_t *)arg;
#ifdef PARAVER
	addThread();
	pushState(paraver::_Running_);
#endif
	gmacCreateManager();
	void *ret = gthread->__start_routine(gthread->__arg);
	gmacRemoveManager();
#ifdef PARAVER
	popState();
#endif
	return ret;
}

int pthread_create(pthread_t *__restrict __newthread,
		__const pthread_attr_t *__restrict __attr, 
		void *(*__start_routine)(void *),
		void *__restrict __arg) 
{
	TRACE("pthread_create");
	gthread.__start_routine = __start_routine;
	gthread.__arg = __arg;
	return _pthread_create(__newthread, __attr, gmac_pthread, (void *)&gthread);
}
