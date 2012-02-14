#ifndef GMAC_HAL_CUDA_DEVICE_H_
#define GMAC_HAL_CUDA_DEVICE_H_

#include "hal/device.h"
#include "util/unique.h"

#include "device.h"
#include "types.h"

namespace __impl { namespace hal { namespace cuda {

class coherence_domain;

typedef hal::detail::aspace hal_aspace;
typedef hal::detail::device hal_device;

class platform;

class GMAC_LOCAL device :
    public hal_device,
    public util::unique<device>,
    public gmac::util::mutex<device> {

    friend class aspace;
    friend list_platform hal::get_platforms();

    typedef hal_device parent;
    typedef gmac::util::mutex<device> lock;

protected:
    CUdevice cudaDevice_;

    int major_;
    int minor_;

    GmacDeviceInfo info_;
    bool isInfoInitialized_;

public:
    device(CUdevice cudaDevice, platform &plat, coherence_domain &coherenceDomain);

    hal_aspace *create_aspace(const set_siblings &siblings, gmacError_t &err);
    gmacError_t destroy_aspace(hal_aspace &as);

    hal_stream *create_stream(hal_aspace &as);
    gmacError_t destroy_stream(hal_stream &stream);

    int get_major() const;
    int get_minor() const;

    size_t get_total_memory() const;
    size_t get_free_memory() const;

    bool has_direct_copy(const hal_device &dev) const;

    gmacError_t get_info(GmacDeviceInfo &info);
};

}}}

#endif /* GMAC_HAL_CUDA_DEVICE_H_ */

/* vim:set backspace=2 tabstop=4 shiftwidth=4 textwidth=120 foldmethod=marker expandtab: */
