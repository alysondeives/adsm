#ifndef GMAC_HAL_CUDA_KERNEL_IMPL_H_
#define GMAC_HAL_CUDA_KERNEL_IMPL_H_

namespace __impl { namespace hal { namespace cuda {

inline
kernel_t::kernel_t(CUfunction func, const std::string &name) :
    Parent(func, name)
{
}

#if 0
inline 
kernel_t::launch &
kernel_t::launch_config(parent::config &conf, parent::arg_list &args, stream_t &stream)
{
    return *(new launch(*this, (kernel_t::config &) conf,
                               (kernel_t::arg_list &) args, stream));
}
#endif

inline
kernel_t::launch::launch(kernel_t &parent, config &conf, arg_list &args, stream_t &stream) :
    hal::detail::kernel_t<backend_traits, implementation_traits>::launch(parent, conf, args, stream)
{
}

inline
unsigned
kernel_t::arg_list::get_nargs() const
{
    return nArgs_;
}

inline
gmacError_t
kernel_t::arg_list::push_arg(const void *arg, size_t size)
{
    gmacError_t ret = gmacSuccess;

    params_[nArgs_++] = arg;

    return ret;
}

inline
event_ptr
kernel_t::launch::execute(list_event_detail &_dependencies, gmacError_t &err)
{
    event_ptr ret;
    list_event &dependencies = reinterpret_cast<list_event &>(_dependencies);
    dependencies.set_barrier(get_stream());

    if (err == gmacSuccess) {
        ret = execute(err);
    }

    return ret;
}

inline
event_ptr
kernel_t::launch::execute(event_ptr event, gmacError_t &err)
{
    list_event dependencies;
    dependencies.add_event(event);

    return execute(dependencies, err);
}

inline
event_ptr
kernel_t::launch::execute(gmacError_t &err)
{
    get_stream().get_context().set();

    CUresult res;

    dim3 dimsGlobal = get_config().get_dims_global();
    dim3 dimsGroup = get_config().get_dims_group();

    TRACE(LOCAL, "kernel launch on stream: %p", get_stream()());
    event_ptr ret(true, _event_t::Kernel, get_stream().get_context());

    ret->begin(get_stream());
    res = cuLaunchKernel(get_kernel()(), dimsGlobal.x,
                                         dimsGlobal.y,
                                         dimsGlobal.z,
                                         dimsGroup.x,
                                         dimsGroup.y,
                                         dimsGroup.z,
                                         get_config().memShared_,
                                         get_stream()(),
                                         (void **) get_arg_list().params_,
                                         NULL);
    ret->end();
    err = error(res);

    if (err != gmacSuccess) {
        ret.reset();
    }

    return ret;
}



inline
kernel_t::config::config(dim3 global, dim3 group, size_t shared, cudaStream_t tokens) :
    hal::detail::kernel_t<backend_traits, implementation_traits>::config(3),
    dimsGlobal_(global),
    dimsGroup_(group),
    memShared_(shared)
{
}

inline
const dim3 &
kernel_t::config::get_dims_global() const
{
    return dimsGlobal_;
}

inline
const dim3 &
kernel_t::config::get_dims_group() const
{
    return dimsGroup_;
}

}}}

#endif /* TYPES_IMPL_H */

/* vim:set backspace=2 tabstop=4 shiftwidth=4 textwidth=120 foldmethod=marker expandtab: */
