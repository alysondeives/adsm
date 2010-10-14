#ifndef GMAC_UTIL_POSIX_PRIVATE_IPP_
#define GMAC_UTIL_POSIX_PRIVATE_IPP_

namespace gmac { namespace util {

template <typename T>
inline
void Private<T>::init(Private &var)
{
    pthread_key_create(&var.key_, NULL);
}

#if 0
template <typename T>
inline
Private<T>::Private()
{
    pthread_key_create(&key_, NULL);
}
#endif

template <typename T>
inline
void Private<T>::set(const void *value)
{
    pthread_setspecific(key_, value);
}

template <typename T>
inline
T *Private<T>::get()
{
    return static_cast<T *>(pthread_getspecific(key_));
}

}}

#endif
