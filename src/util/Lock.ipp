#ifndef __UTIL_LOCK_IPP_
#define __UTIL_LOCK_IPP_

inline void
Lock::lock()
{
    enterLock(__name);
    MUTEX_LOCK(__mutex);
    exitLock();
}

inline void
Lock::unlock()
{
    MUTEX_UNLOCK(__mutex);
}

inline bool
Lock::tryLock()
{
   return MUTEX_TRYLOCK(__mutex) == 0;
}

inline void
RWLock::lockRead()
{
    enterLock(__name);
    LOCK_READ(__lock);
    exitLock();
}

inline void
RWLock::lockWrite()
{
    enterLock(__name);
    LOCK_WRITE(__lock);
    exitLock();
}

inline void
RWLock::unlock()
{
    LOCK_RELEASE(__lock);
}

inline bool
RWLock::tryRead()
{
   return LOCK_TRYREAD(__lock) == 0;
}

inline bool
RWLock::tryWrite()
{
   return LOCK_TRYWRITE(__lock) == 0;
}

#endif
