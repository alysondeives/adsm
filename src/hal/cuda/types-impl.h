#ifndef GMAC_HAL_CUDA_TYPES_IMPL_H_
#define GMAC_HAL_CUDA_TYPES_IMPL_H_

//#include "device.h"

namespace __impl { namespace hal { namespace cuda {

inline
aspace_t::aspace_t(CUcontext ctx, device &dev) :
    hal::detail::aspace_t<device, backend_traits>(ctx, dev)
{
}

inline
device &
aspace_t::get_device()
{
    return reinterpret_cast<device &>(hal::detail::aspace_t<device, backend_traits>::get_device());
}

inline
stream_t::stream_t(CUstream stream, aspace_t &aspace) :
    hal::detail::stream_t<device, backend_traits>(stream, aspace)
{
}

inline
aspace_t &
stream_t::get_address_space()
{
    return reinterpret_cast<aspace_t &>(hal::detail::stream_t<device, backend_traits>::get_address_space());
}

inline
void
_event_common_t::begin(stream_t &stream)
{
    timeBase_ = hal::get_timestamp();
    cuEventRecord(eventStart_, stream());
}

inline
void
_event_common_t::end(stream_t &stream)
{
    cuEventRecord(eventEnd_, stream());
}

inline
event_t::event_t(stream_t &stream, gmacError_t err) :
    hal::detail::event_t<device, backend_traits>(stream, err)
{
}

inline
stream_t &
event_t::get_stream()
{
    return reinterpret_cast<stream_t &>(hal::detail::event_t<device, backend_traits>::get_stream());
}

inline
async_event_t::async_event_t(stream_t &stream, gmacError_t err) :
    hal::detail::async_event_t<device, backend_traits>(stream, err)
{
}

inline
gmacError_t
async_event_t::sync()
{
    CUresult ret = cuEventSynchronize(eventEnd_);
    if (ret == CUDA_SUCCESS) {
        float mili;
        ret = cuEventElapsedTime(&mili, eventStart_, eventEnd_);
        if (ret == CUDA_SUCCESS) {
            timeQueued_ = timeSubmit_ = timeStart_ = timeBase_;
            timeEnd_ = timeQueued_ + time_t(mili * 1000.f);
        }
    }
    return error(ret);
}

inline
stream_t &
async_event_t::get_stream()
{
    return reinterpret_cast<stream_t &>(hal::detail::event_t<device, backend_traits>::get_stream());
}

}}}

#endif /* TYPES_IMPL_H */

/* vim:set backspace=2 tabstop=4 shiftwidth=4 textwidth=120 foldmethod=marker expandtab: */
