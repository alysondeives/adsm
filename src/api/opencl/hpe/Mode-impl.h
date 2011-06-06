#ifndef GMAC_API_OPENCL_HPE_MODE_IMPL_H_
#define GMAC_API_OPENCL_HPE_MODE_IMPL_H_

#include "core/IOBuffer.h"

#include "api/opencl/hpe/Accelerator.h"
#include "api/opencl/hpe/Context.h"

namespace __impl { namespace opencl { namespace hpe {

inline
KernelList::KernelList() : gmac::util::Lock("KernelList")
{
}

inline
KernelList::~KernelList()
{
    Parent::const_iterator i;
    lock();
    for(i = Parent::begin(); i != Parent::end(); i++) delete *i;
    Parent::clear();
    unlock();
}

inline
void KernelList::insert(core::hpe::Kernel *kernel)
{
    lock();
    Parent::push_back(kernel);
    unlock();
}

inline
void Mode::switchIn()
{
}

inline
void Mode::switchOut()
{
}

#if 0

inline gmacError_t
Mode::wait(core::hpe::KernelLaunch &launch)
{
    switchIn();
    error_ = contextMap_.waitForCall(launch);
    switchOut();

    return error_;
}


inline gmacError_t
Mode::wait()
{
    switchIn();
    error_ = contextMap_.waitForCall();
    switchOut();

    return error_;
}
#endif

}}}

#endif
