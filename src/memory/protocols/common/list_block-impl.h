#ifndef GMAC_MEMORY_PROTOCOL_BLOCKLIST_IMPL_H_
#define GMAC_MEMORY_PROTOCOL_BLOCKLIST_IMPL_H_

#include <algorithm>

#include "memory/vm/Model.h"

namespace __impl { namespace memory { namespace protocols { namespace common {

inline
list_block::list_block() :
    Lock("list_block")
{}

inline
list_block::~list_block()
{}

inline bool
list_block::empty() const
{
    lock();
    bool ret = Parent::empty();
    unlock();
    return ret;
}

inline size_t
list_block::size() const
{
    lock();
    size_t ret = Parent::size();
    unlock();
    return ret;
}

inline void
list_block::push(block_ptr block)
{
    lock();
    Parent::push_back(block);
    unlock();
}

inline list_block::locked_block
list_block::front()
{
    ASSERTION(Parent::empty() == false);
    lock();
    block_ptr b = Parent::front();
    locked_block ret(b);
    unlock();
    return ret;
}

inline void list_block::remove(block_ptr block)
{
    lock();
    Parent::remove(block);
    unlock();
    return;
}

}}}}

#endif