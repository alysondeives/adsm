#ifndef GMAC_MEMORY_OBJECTMAP_IMPL_H_
#define GMAC_MEMORY_OBJECTMAP_IMPL_H_

#include "Object.h"

namespace __impl { namespace memory {

inline
gmacError_t ObjectMap::forEachObject(gmacError_t (Object::*f)(void) const) const
{
    const_iterator i;
    lockRead();
    for(i = begin(); i != end(); i++) {
        gmacError_t ret = (i->second->*f)();
        if(ret != gmacSuccess) {
            unlock();
            return ret;
        }
    }
    unlock();
    return gmacSuccess;
}

inline
gmacError_t ObjectMap::forEachObject(gmacError_t (Object::*f)(void))
{
    iterator i;
    lockRead();
    for(i = begin(); i != end(); i++) {
        gmacError_t ret = (i->second->*f)();
        if(ret != gmacSuccess) {
            unlock();
            return ret;
        }
    }
    unlock();
    return gmacSuccess;
}

template <typename P1>
gmacError_t ObjectMap::forEachObject(gmacError_t (Object::*f)(P1 &) const, P1 &p1) const
{
    const_iterator i;
    lockRead();
    for(i = begin(); i != end(); i++) {
        gmacError_t ret = (i->second->*f)(p1);
        if(ret != gmacSuccess) {
            unlock();
            return ret;
        }
    }
    unlock();
    return gmacSuccess;
}



template <typename P1>
gmacError_t ObjectMap::forEachObject(gmacError_t (Object::*f)(P1 &), P1 &p1)
{
    const_iterator i;
    lockRead();
    for(i = begin(); i != end(); i++) {
        gmacError_t ret = (i->second->*f)(p1);
        if(ret != gmacSuccess) {
            unlock();
            return ret;
        }
    }
    unlock();
    return gmacSuccess;
}

inline
gmacError_t ObjectMap::dumpObjects(std::string prefix, protocol::common::Statistic stat) const
{
#ifdef DEBUG
    lockRead();
    const_iterator i;
    for(i = begin(); i != end(); i++) {
        Object &obj = *(i->second);
        std::stringstream name;
        name << prefix << "#" << obj.getId() << "-" << obj.getDumps(stat) << "_" << protocol::common::StatisticName[stat];

        std::ofstream out(name.str().c_str(), std::ios_base::trunc);
        ASSERTION(out.good());
        obj.dump(out, stat);

        out.close();
    }
    unlock();
#endif
    return gmacSuccess;
}

inline
gmacError_t ObjectMap::dumpObject(std::string prefix, protocol::common::Statistic stat, hostptr_t ptr) const
{
#ifdef DEBUG
    Object *obj = get(ptr, 1);
    lockRead();
    ASSERTION(obj != NULL);
    std::stringstream name;
    name << prefix << "#" << obj->getId() << "-" << obj->getDumps(stat) << "_" << protocol::common::StatisticName[stat];

    std::ofstream out(name.str().c_str(), std::ios_base::trunc);
    ASSERTION(out.good());
    obj->dump(out, stat);

    out.close();
    unlock();
#endif
    return gmacSuccess;
}


}}

#endif /* OBJECTMAP_IMPL_H */

/* vim:set backspace=2 tabstop=4 shiftwidth=4 textwidth=120 foldmethod=marker expandtab: */
