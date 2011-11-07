#ifndef GMAC_HAL_TYPES_ASPACE_H_
#define GMAC_HAL_TYPES_ASPACE_H_

#include <list>

#include "util/observer.h"

namespace __impl { namespace hal {

namespace detail {

template <typename D, typename B, typename I>
class GMAC_LOCAL buffer_t :
    public util::observer<typename I::event> {
public:
    enum type {
        ToHost,
        ToDevice,
        DeviceToDevice
    };
private:
    typename I::context &context_;
    type type_;
    size_t size_;

protected:
    buffer_t(type t, size_t size, typename I::context &context);

    type get_type() const;

public:
    virtual hostptr_t get_addr() = 0;
    virtual typename I::ptr get_device_addr() = 0;
    typename I::context &get_context();
    const typename I::context &get_context() const;
    size_t get_size() const;
};

template <typename D, typename B, typename I>
class GMAC_LOCAL list_event :
    protected std::list<typename I::event> {

    typedef std::list<typename I::event> Parent;
public:
    static list_event empty;

    void add_event(typename I::event event);
};

template <typename D, typename B, typename I>
list_event<D, B, I> list_event<D, B, I>::empty;

template <typename D, typename B, typename I>
class GMAC_LOCAL code_repository
{
public:
    virtual const typename I::kernel *get_kernel(gmac_kernel_id_t key) const = 0;
    virtual const typename I::kernel *get_kernel(const std::string &name) const = 0;
};

template <typename D, typename B, typename I>
class GMAC_LOCAL context_t {
protected:
    typename B::context context_;
    D &device_;

    context_t(typename B::context context, D &device);

public:
    D &get_device();
    const D &get_device() const;

    typename B::context &operator()();
    const typename B::context &operator()() const;

    virtual typename I::ptr alloc(size_t size, gmacError_t &err) = 0;
    virtual typename I::ptr alloc_host_pinned(size_t size, GmacProtection hint, gmacError_t &err) = 0;
    //virtual typename I::buffer *alloc_buffer(size_t count, GmacProtection hint, gmacError_t &err) = 0;
    virtual gmacError_t free(typename I::ptr acc) = 0;
    virtual gmacError_t free_host_pinned(typename I::ptr ptr) = 0;
    //virtual gmacError_t free_buffer(typename I::buffer &buffer) = 0;

    virtual typename I::event copy(typename I::ptr dst, typename I::ptr src, size_t count, typename I::stream &stream, list_event<D, B, I> &dependencies, gmacError_t &err) = 0;
    virtual typename I::event copy(typename I::ptr dst, typename I::ptr src, size_t count, typename I::stream &stream, typename I::event event, gmacError_t &err) = 0;
    virtual typename I::event copy(typename I::ptr dst, typename I::ptr src, size_t count, typename I::stream &stream, gmacError_t &err) = 0;

    virtual typename I::event copy(typename I::ptr dst, device_input &input, size_t count, typename I::stream &stream, list_event<D, B, I> &dependencies, gmacError_t &err) = 0;
    virtual typename I::event copy(typename I::ptr dst, device_input &input, size_t count, typename I::stream &stream, typename I::event event, gmacError_t &err) = 0;
    virtual typename I::event copy(typename I::ptr dst, device_input &input, size_t count, typename I::stream &stream, gmacError_t &err) = 0;

    virtual typename I::event copy(device_output &output, typename I::ptr src, size_t count, typename I::stream &stream, list_event<D, B, I> &dependencies, gmacError_t &err) = 0;
    virtual typename I::event copy(device_output &output, typename I::ptr src, size_t count, typename I::stream &stream, typename I::event event, gmacError_t &err) = 0;
    virtual typename I::event copy(device_output &output, typename I::ptr src, size_t count, typename I::stream &stream, gmacError_t &err) = 0;

    virtual typename I::event copy_async(typename I::ptr dst, const typename I::ptr src, size_t count, typename I::stream &stream, list_event<D, B, I> &dependencies, gmacError_t &err) = 0;
    virtual typename I::event copy_async(typename I::ptr dst, const typename I::ptr src, size_t count, typename I::stream &stream, typename I::event event, gmacError_t &err) = 0;
    virtual typename I::event copy_async(typename I::ptr dst, const typename I::ptr src, size_t count, typename I::stream &stream, gmacError_t &err) = 0;

    virtual typename I::event copy_async(typename I::ptr dst, device_input &input, size_t count, typename I::stream &stream, list_event<D, B, I> &dependencies, gmacError_t &err) = 0;
    virtual typename I::event copy_async(typename I::ptr dst, device_input &input, size_t count, typename I::stream &stream, typename I::event event, gmacError_t &err) = 0;
    virtual typename I::event copy_async(typename I::ptr dst, device_input &input, size_t count, typename I::stream &stream, gmacError_t &err) = 0;

    virtual typename I::event copy_async(device_output &output, typename I::ptr src, size_t count, typename I::stream &stream, list_event<D, B, I> &dependencies, gmacError_t &err) = 0;
    virtual typename I::event copy_async(device_output &output, typename I::ptr src, size_t count, typename I::stream &stream, typename I::event event, gmacError_t &err) = 0;
    virtual typename I::event copy_async(device_output &output, typename I::ptr src, size_t count, typename I::stream &stream, gmacError_t &err) = 0;

#if 0
    virtual typename I::event copy_async(typename I::ptr dst, typename I::buffer src, size_t off, size_t count, typename I::stream &stream, list_event<D, B, I> &dependencies, gmacError_t &err) = 0;
    virtual typename I::event copy_async(typename I::ptr dst, typename I::buffer src, size_t off, size_t count, typename I::stream &stream, typename I::event event, gmacError_t &err) = 0;
    virtual typename I::event copy_async(typename I::ptr dst, typename I::buffer src, size_t off, size_t count, typename I::stream &stream, gmacError_t &err) = 0;

    virtual typename I::event copy_async(typename I::buffer dst, size_t off, typename I::ptr src, size_t count, typename I::stream &stream, list_event<D, B, I> &dependencies, gmacError_t &err) = 0;
    virtual typename I::event copy_async(typename I::buffer dst, size_t off, typename I::ptr src, size_t count, typename I::stream &stream, typename I::event event, gmacError_t &err) = 0;
    virtual typename I::event copy_async(typename I::buffer dst, size_t off, typename I::ptr src, size_t count, typename I::stream &stream, gmacError_t &err) = 0;
#endif

    virtual typename I::event memset(typename I::ptr dst, int c, size_t count, typename I::stream &stream, list_event<D, B, I> &dependencies, gmacError_t &err) = 0;
    virtual typename I::event memset(typename I::ptr dst, int c, size_t count, typename I::stream &stream, typename I::event event, gmacError_t &err) = 0;
    virtual typename I::event memset(typename I::ptr dst, int c, size_t count, typename I::stream &stream, gmacError_t &err) = 0;

    virtual typename I::event memset_async(typename I::ptr dst, int c, size_t count, typename I::stream &stream, list_event<D, B, I> &dependencies, gmacError_t &err) = 0;
    virtual typename I::event memset_async(typename I::ptr dst, int c, size_t count, typename I::stream &stream, typename I::event event, gmacError_t &err) = 0;
    virtual typename I::event memset_async(typename I::ptr dst, int c, size_t count, typename I::stream &stream, gmacError_t &err) = 0;

    virtual const typename I::code_repository &get_code_repository() = 0;
};

}

}}

#endif /* ASPACE_H */

/* vim:set backspace=2 tabstop=4 shiftwidth=4 textwidth=120 foldmethod=marker expandtab: */
