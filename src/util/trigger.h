#ifndef GMAC_UTIL_TRIGGER_H_
#define GMAC_UTIL_TRIGGER_H_

#ifdef USE_CXX0X
#include <functional>
#include <type_traits>
#else
#include <tr1/functional>
#include <tr1/type_traits>
#endif

#include <algorithm>
#include <list>

#include "trace/logger.h"

#include "factory.h"

namespace __impl { namespace util {

#ifdef USE_CXX0X
using std::bind;
#else
using namespace std::tr1;
#endif

#define do_member(f,o,...) util::bind(std::mem_fn(&f), o, __VA_ARGS__)
#define do_func(f,...)     util::bind(&f, __VA_ARGS__)

template <typename T>
class GMAC_LOCAL functor_iface {
public:
    virtual void operator()(T &t) = 0;

    virtual ~functor_iface()
    {
    }
};

template <>
class GMAC_LOCAL functor_iface<void> {
public:
    virtual void operator()() = 0;

    virtual ~functor_iface()
    {
    }
};

template <typename F, typename T, typename R>
class GMAC_LOCAL functor :
    public functor_iface<T> {
private:
    F fun_;
    R ret_;

public:
    functor(F fun, R ret) :
        fun_(fun),
        ret_(ret)
    {
    }

    void
    operator()(T &t)
    {
        R ret = fun_(t);
        ASSERTION(ret == ret_);
    }
};

template <typename F, typename R>
class GMAC_LOCAL functor<F, void, R> :
    public functor_iface<void> {
private:
    F fun_;
    R ret_;

public:
    functor(F fun, R ret) :
        fun_(fun),
        ret_(ret)
    {
    }

    void
    operator()()
    {
        R ret = fun_();
        ASSERTION(ret == ret_);
    }
};

template <typename F, typename T>
class GMAC_LOCAL functor<F, T, void> :
    public functor_iface<T> {
private:
    F fun_;

public:
    functor(F fun) :
        fun_(fun)
    {
    }

    void
    operator()(T &t)
    {
        fun_(t);
    }
};

template <typename F>
class GMAC_LOCAL functor<F, void, void> :
    public functor_iface<void> {
private:
    F fun_;

public:
    functor(F fun) :
        fun_(fun)
    {
    }

    void
    operator()()
    {
        fun_();
    }
};

template <typename T = void>
class GMAC_LOCAL list_trigger {
    template <class T1> friend class on_construction;
    template <class T1> friend class on_destruction;

protected:
    typedef std::list<functor_iface<T> *> base_list_trigger;
    base_list_trigger triggers_;

    void
    exec_triggers(bool freeTriggers)
    {
        typename base_list_trigger::iterator it;
        for (it  = triggers_.begin();
             it != triggers_.end();
             it++) {
            (**it)();
        }

        if (freeTriggers) {
            remove_triggers();
        }
    }

    template <typename T2 = T>
    void
    exec_triggers(bool freeTriggers, T2 &arg)
    {
        typename base_list_trigger::iterator it;
        for (it  = triggers_.begin();
             it != triggers_.end();
             it++) {
             (**it)(arg);
        }

        if (freeTriggers) {
            remove_triggers();
        }
    }

    void remove_triggers()
    {
        typename base_list_trigger::iterator it;
        for (it  = triggers_.begin();
             it != triggers_.end();
             it++) {
            delete *it;
        }
        triggers_.clear();
    }

    
public:
    inline
    virtual ~list_trigger()
    {
        remove_triggers();
    }

    template <typename F, typename R>
    void add_trigger(F fun, R ret)
    {
        triggers_.push_back(new functor<F, T, R>(fun, ret));
    }

    template <typename F>
    void add_trigger(F fun)
    {
        triggers_.push_back(new functor<F, T, void>(fun));
    }

    template <typename F>
    bool remove_trigger(F fun)
    {
        // TODO: implement
        FATAL("%s: Not implemented", __func__);
#if 0
        auto it = std::find(triggers_.begin(), triggers_.end(), fun);
        if (it != triggers_.end()) {
            triggers_.remove(it);
            return true;
        } else {
            return false;
        }
#endif
        return true;
    }
};

// Event type tags
namespace event {
struct construct {};
struct destruct {};
}

template <typename T, typename Evt>
class observable_base;

template <typename T, typename Evt>
class GMAC_LOCAL observer_base {
    friend class observable_base<T, Evt>;
public:
protected:
    typedef observable_base<T, Evt> observing;

    virtual void event_handler(T &obj, Evt evt) = 0;
public:
    typedef void (*handler_type)(T &obj, Evt evt);
    typedef Evt event_type;
};

template <typename T, typename Evt>
class GMAC_LOCAL observer_class {
public:
};

template <typename T, typename Evt>
class GMAC_LOCAL observable_base {
    typedef std::list<observer_base<T, Evt> *> list_observer;
    static list_observer observers_;

    typedef std::list<typename observer_base<T, Evt>::handler_type> list_observer_class;
    static list_observer_class observersClass_;

public:
    typedef observer_base<T, Evt> observer_type;

public:
    typedef Evt event_type;

    static void
    update(T &obj)
    {
        typename list_observer::iterator it = observers_.begin();

        for (; it != observers_.end(); ++it) {
            (*it)->event_handler(obj, Evt());
        }
    }

    static void
    add_observer(observer_type &obs)
    {
        observers_.push_back(&obs);
    }

    static bool
    remove_observer(observer_type &obs)
    {
        typename list_observer::iterator it = std::find(observers_.begin(),
                                                        observers_.end(),
                                                        &obs);
        if (it != observers_.end()) {
            observers_.erase(it);
            return true;
        } else {
            return false;
        }
    }
};

template <typename T, typename Evt, bool Auto = true>
class GMAC_LOCAL observer :
    public observer_base<T, Evt> {
    typedef observer_base<T, Evt> parent;
protected:
    inline
    observer()
    {
        TRACE(LOCAL, "Adding observer");
        observable_base<T, Evt>::add_observer(*this);
    }

    inline
    virtual ~observer()
    {
        TRACE(LOCAL, "Removing observer");
        parent::observing::remove_observer(*this);
    }
};

template <typename T, typename Evt>
class GMAC_LOCAL observer<T, Evt, false> :
    public observer_base<T, Evt> {
};


template <typename T, typename Evt>
typename observable_base<T, Evt>::list_observer observable_base<T, Evt>::observers_;

template <typename T, typename Evt>
class GMAC_LOCAL observable :
    public observable_base<T, Evt> {
    typedef observable_base<T, Evt> parent;

public:
};

template <typename T>
class GMAC_LOCAL observable<T, event::construct> :
    public observable_base<T, event::construct> {

    typedef observable_base<T, event::construct> parent;
    typedef factory<T> parent_factory;

public:
    template <typename S, typename... Args>
    static S *
    create(Args &&... args)
    {
        static_assert(std::is_class<S>::value, "S must be a class");
        static_assert(std::is_base_of<T, S>::value, "S must be a subclass of T");

        S *ret = new S(args...);

        parent::update(*ret);

        return ret;
    }
};

template <typename T>
class GMAC_LOCAL observable<T, event::destruct> :
    public observable_base<T, event::destruct> {

    typedef observable_base<T, event::destruct> parent;

public:
    static void
    destroy(T &obj)
    {
        parent::update(obj);

        delete &obj;
    }
};
}}

#endif // GMAC_UTIL_TRIGGER_H_

/* vim:set backspace=2 tabstop=4 shiftwidth=4 textwidth=120 foldmethod=marker expandtab: */
