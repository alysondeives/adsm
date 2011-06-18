#ifndef GMAC_CORE_HPE_PROCESS_IMPL_H_
#define GMAC_CORE_HPE_PROCESS_IMPL_H_

#include "Map.h"

namespace __impl { namespace core { namespace hpe {

inline size_t
Process::nAccelerators() const
{
    return accs_.size();
}

inline Accelerator *
Process::getAccelerator(unsigned i)
{
    if (i >= accs_.size()) return NULL;

    return accs_[i];
}

inline memory::Protocol *
Process::protocol()
{
    return &protocol_;
}

inline memory::ObjectMap &
Process::shared()
{
    return shared_;
}

inline const memory::ObjectMap &
Process::shared() const
{
    return shared_;
}

inline memory::ObjectMap &
Process::global()
{
    return global_;
}

inline const memory::ObjectMap &
Process::global() const
{
    return global_;
}

inline memory::ObjectMap &
Process::orphans()
{
    return orphans_;
}

inline const memory::ObjectMap &
Process::orphans() const
{
    return orphans_;
}

inline void
Process::insertOrphan(memory::Object &obj)
{
    orphans_.insert(obj);
    shared_.remove(obj);
    // We decrease the count because it gets incremented when inserted
    obj.decRef();
}

}}}

#endif
