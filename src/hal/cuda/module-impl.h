#ifndef GMAC_API_CUDA_HPE_MODULE_IMPL_H_
#define GMAC_API_CUDA_HPE_MODULE_IMPL_H_

namespace __impl { namespace hal { namespace cuda {

inline bool
variable_descriptor::constant() const
{
    return constant_;
}

inline size_t
variable_t::size() const
{
    return size_;
}

inline CUdeviceptr
variable_t::devPtr() const
{
    return ptr_;
}

inline CUtexref
texture_t::texRef() const
{
    return texRef_;
}

inline
void
module_descriptor::add(kernel_descriptor & k)
{
    kernels_.push_back(k);
}

inline
void
module_descriptor::add(variable_descriptor & v)
{
    if (v.constant()) {
        constants_.push_back(v);
    } else {
        variables_.push_back(v);
    }
}

inline
void
module_descriptor::add(texture_descriptor &t)
{
    textures_.push_back(t);
}

#if 0
template <typename T>
void
code_repository::register_kernels(T &t) const
{
    map_kernel::const_iterator k;
    for (k = kernels_.begin(); k != kernels_.end(); k++) {
        t.register_kernel(k->first, *k->second);
    }
}
#endif

}}}

#endif
