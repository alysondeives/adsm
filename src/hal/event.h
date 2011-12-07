#ifndef GMAC_HAL_TYPES_EVENT_H_
#define GMAC_HAL_TYPES_EVENT_H_

#include "util/delayed_exec.h"
#include "util/UniquePtr.h"

namespace __impl { namespace hal {

namespace detail {

template <typename I>
class GMAC_LOCAL _event_t :
    public util::delayed_exec {
    friend class I::context;
    friend class I::kernel;

    typedef typename I::context context_parent_t;
public:
    enum type {
        Transfer,
        Kernel
    };

    enum state {
        Queued,
        Submit,
        Start,
        End,
        None
    };

private:
    context_parent_t &context_;

protected:
    bool async_;
    bool synced_;
    type type_;
    state state_;
    gmacError_t err_;

    hal::time_t timeQueued_;
    hal::time_t timeSubmit_;
    hal::time_t timeStart_;
    hal::time_t timeEnd_;

    _event_t(bool async, type t, context_parent_t &context);

public:
    virtual ~_event_t();

    virtual gmacError_t sync();

    context_parent_t &get_context();

    type get_type() const;
    virtual state get_state() = 0;

    hal::time_t get_time_queued() const;
    hal::time_t get_time_submit() const;
    hal::time_t get_time_start() const;
    hal::time_t get_time_end() const;

    bool is_synced() const;
};

}

}}

#endif /* EVENT_H */

/* vim:set backspace=2 tabstop=4 shiftwidth=4 textwidth=120 foldmethod=marker expandtab: */