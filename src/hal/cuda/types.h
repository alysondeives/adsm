#ifndef GMAC_HAL_CUDA_TYPES_H_
#define GMAC_HAL_CUDA_TYPES_H_

#include <cuda.h>

#include "hal/types-detail.h"

namespace __impl { namespace hal { namespace cuda {

class device;

typedef hal::detail::backend_traits<CUcontext, CUstream, CUevent> backend_traits;

gmacError_t error(CUresult err);

class aspace_t :
    public hal::detail::aspace_t<device, backend_traits> {
public:
    aspace_t(CUcontext ctx, device &device);

    device &get_device();
};

class stream_t :
    public hal::detail::stream_t<device, backend_traits> {
public:
    stream_t(CUstream stream, aspace_t &aspace);

    aspace_t &get_address_space();
    CUstream &operator()();
};

class _event_common_t {
protected:
    CUevent eventStart_;
    CUevent eventEnd_;

    hal::time_t timeBase_;

    // Not instantiable
    _event_common_t()
    {
    }

    void begin(stream_t &stream);
    void end(stream_t &stream);
};

class event_t :
    public hal::detail::event_t<device, backend_traits>,
    public _event_common_t {
    friend class device;
public:
    event_t(stream_t &stream, gmacError_t err = gmacSuccess);
    stream_t &get_stream();
};

class async_event_t :
    public hal::detail::async_event_t<device, backend_traits>,
    public _event_common_t {
    friend class device;
public:
    async_event_t(stream_t &stream, gmacError_t err = gmacSuccess);
    stream_t &get_stream();
    gmacError_t sync();
};


}}}

#include "types-impl.h"

#endif /* GMAC_HAL_CUDA_TYPES_H_ */

/* vim:set backspace=2 tabstop=4 shiftwidth=4 textwidth=120 foldmethod=marker expandtab: */
