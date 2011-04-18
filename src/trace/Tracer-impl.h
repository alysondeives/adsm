#ifndef GMAC_TRACE_TRACER_IMPL_H_
#define GMAC_TRACE_TRACER_IMPL_H_

#include "util/Thread.h"

namespace __impl { namespace trace {

#if defined(USE_TRACE)
extern Tracer *tracer;

void InitApiTracer();
void FiniApiTracer();

inline
Tracer::Tracer() : base_(0)
{
	base_ = timeMark();
}

inline
Tracer::~Tracer()
{
}
#endif

#if defined(USE_TRACE)
inline int32_t GetThreadId()
{
    if(tid_.get() == NULL) {
        AtomicInc(threads_);
        tid_.set((int32_t *)threads_);
    }
    // This will break if we have more than 8192 CPU threads using GPUs
    return 8192 + static_cast<int32_t>((long_t)tid_.get() & 0xffffffff);
}
#endif


inline void StartThread(THREAD_T tid, const char *name)
{
#if defined(USE_TRACE)
	if(tracer != NULL) {		
		tracer->startThread(tid, name);
		tracer->setThreadState(tid, Init);
	}
#endif
}

inline void StartThread(const char *name)
{
#if defined(USE_TRACE)
	return StartThread(GetThreadId(), name);
#endif
}

inline void EndThread(THREAD_T tid)
{
#if defined(USE_TRACE)	
	if(tracer != NULL) tracer->endThread(tid);
#endif
}

inline void EndThread()
{
#if defined(USE_TRACE)
	return EndThread(GetThreadId());
#endif
}

inline void EnterFunction(THREAD_T tid, const char *name)
{
#if defined(USE_TRACE)
	if(tracer != NULL) tracer->enterFunction(tid, name);
#endif
}

inline void EnterFunction(const char *name)
{
#if defined(USE_TRACE)
	return EnterFunction(GetThreadId(), name);
#endif
}

inline void ExitFunction(THREAD_T tid, const char *name)
{
#if defined(USE_TRACE)
	if(tracer != NULL) tracer->exitFunction(tid, name);
#endif
}

inline void ExitFunction(const char *name)
{
#if defined(USE_TRACE)
	return ExitFunction(GetThreadId(),name);
#endif
}

inline void RequestLock(const char *name)
{
#if defined(USE_TRACE_LOCKS)
	if(tracer != NULL) tracer->requestLock(GetThreadId(), name);
#endif
}

inline void AcquireLockExclusive(const char *name)
{
#if defined(USE_TRACE_LOCKS)
	if(tracer != NULL) tracer->acquireLockExclusive(GetThreadId(), name);
#endif
}

inline void AcquireLockShared(const char *name)
{
#if defined(USE_TRACE_LOCKS)
	if(tracer != NULL) tracer->acquireLockShared(GetThreadId(), name);
#endif
}

inline void ExitLock(const char *name)
{
#if defined(USE_TRACE_LOCKS)
	if(tracer != NULL) tracer->exitLock(GetThreadId(), name);
#endif
}

inline void SetThreadState(THREAD_T tid, const State &state)
{
#if defined(USE_TRACE)
	if(tracer != NULL) tracer->setThreadState(tid, state);
#endif
}

inline void SetThreadState(const State &state)
{
#if defined(USE_TRACE)
	return SetThreadState(GetThreadId(), state);
#endif
}

inline void DataCommunication(THREAD_T src, THREAD_T dst, uint64_t delta, size_t size)
{
#if defined(USE_TRACE)
    if(tracer != NULL) tracer->dataCommunication(src, dst, delta, size);
#endif
}

inline void DataCommunication(THREAD_T tid, uint64_t delta, size_t size)
{
#if defined(USE_TRACE)
    return DataCommunication(GetThreadId(), tid, delta, size);
#endif
}

inline void TimeMark(uint64_t &mark)
{
#if defined(USE_TRACE)
    if(tracer != NULL) mark = tracer->timeMark();
    else mark = 0;
#endif
}

}}

#endif
