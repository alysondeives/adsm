#ifndef GMAC_HAL_TYPES_STREAM_H_
#define GMAC_HAL_TYPES_STREAM_H_

namespace __impl { namespace hal {

namespace detail {

template <typename B, typename I>
class GMAC_LOCAL stream_t {
    typedef typename I::context context_parent_t;

protected:
    typename B::stream stream_;
    context_parent_t &context_;

    stream_t(typename B::stream stream, context_parent_t &context);

public:
    enum state {
        Empty,
        Running
    };

    context_parent_t &get_context();
    typename B::stream &operator()();
    const typename B::stream &operator()() const;

    virtual state query() = 0;
    virtual gmacError_t sync() = 0;
};

}

}}

#endif /* STREAM_H */

/* vim:set backspace=2 tabstop=4 shiftwidth=4 textwidth=120 foldmethod=marker expandtab: */
