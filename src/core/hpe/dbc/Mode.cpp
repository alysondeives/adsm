#ifdef USE_DBC

#include "core/hpe/Mode.h"

namespace __dbc { namespace core { namespace hpe {

void
Mode::cleanUpContexts()
{
    __impl::core::hpe::Mode::cleanUpContexts();
}

gmacError_t
Mode::cleanUp()
{
    gmacError_t ret;
    ret = __impl::core::hpe::Mode::cleanUp();
    return ret;
}

Mode::~Mode()
{
}

Mode::Mode(__impl::core::hpe::Process &proc, __impl::core::hpe::Accelerator &acc) :
    __impl::core::hpe::Mode(proc, acc)
{
}

void
Mode::detach()
{
    __impl::core::hpe::Mode::detach();
}

void
Mode::addObject(__impl::memory::Object &obj)
{
    // TODO Check that the object has not been previously added
    __impl::core::hpe::Mode::addObject(obj);
}

void
Mode::removeObject(__impl::memory::Object &obj)
{
    // TODO Check that the object has been previously added
    __impl::core::hpe::Mode::removeObject(obj);
}

__impl::memory::Object *
Mode::getObject(const hostptr_t addr, size_t size) const
{
    REQUIRES(addr != NULL);

    __impl::memory::Object *obj;
    obj = __impl::core::hpe::Mode::getObject(addr, size);
    return obj;
}

gmacError_t
Mode::map(accptr_t &dst, hostptr_t src, size_t size, unsigned align)
{
    REQUIRES(size > 0);

    gmacError_t ret;
    ret = __impl::core::hpe::Mode::map(dst, src, size, align);

    return ret;
}

gmacError_t
Mode::unmap(hostptr_t addr, size_t size)
{
    REQUIRES(addr != NULL);

    gmacError_t ret;
    ret = __impl::core::hpe::Mode::unmap(addr, size);

    return ret;
}

gmacError_t
Mode::copyToAccelerator(accptr_t acc, const hostptr_t host, size_t size)
{
    REQUIRES(acc != 0);
    REQUIRES(host != NULL);
    REQUIRES(size > 0);

    gmacError_t ret;
    ret = __impl::core::hpe::Mode::copyToAccelerator(acc, host, size);

    return ret;
}

gmacError_t
Mode::copyToHost(hostptr_t host, const accptr_t acc, size_t size)
{
    REQUIRES(host != NULL);
    REQUIRES(acc != 0);
    REQUIRES(size > 0);

    gmacError_t ret;
    ret = __impl::core::hpe::Mode::copyToHost(host, acc, size);

    return ret;
}

gmacError_t
Mode::copyAccelerator(accptr_t dst, const accptr_t src, size_t size)
{
    REQUIRES(dst != 0);
    REQUIRES(src != 0);
    REQUIRES(size > 0);

    gmacError_t ret;
    ret = __impl::core::hpe::Mode::copyAccelerator(dst, src, size);

    return ret;
}

gmacError_t
Mode::memset(accptr_t addr, int c, size_t size)
{
    REQUIRES(addr != 0);
    REQUIRES(size > 0);

    gmacError_t ret;
    ret = __impl::core::hpe::Mode::memset(addr, c, size);

    return ret;
}

gmacError_t 
Mode::moveTo(__impl::core::hpe::Accelerator &acc)
{
    REQUIRES(&acc != acc_);

    gmacError_t ret;
    ret = __impl::core::hpe::Mode::moveTo(acc);

    return ret;
}

gmacError_t
Mode::releaseObjects()
{
    REQUIRES(releasedObjects() == false);

    gmacError_t ret;
    ret = __impl::core::hpe::Mode::releaseObjects();

    ENSURES(releasedObjects() == true);
    return ret;
}

gmacError_t
Mode::acquireObjects()
{
    REQUIRES(releasedObjects() == true);

    gmacError_t ret;
    ret = __impl::core::hpe::Mode::acquireObjects();

    ENSURES(releasedObjects() == false);
    return ret;
}

}}}

#endif

/* vim:set backspace=2 tabstop=4 shiftwidth=4 textwidth=120 foldmethod=marker expandtab: */