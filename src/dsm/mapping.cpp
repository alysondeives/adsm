#include "mapping.h"

namespace __impl { namespace dsm {

mapping::range_block
mapping::get_blocks_in_range(hal::ptr::offset_type offset, size_t count)
{
    ASSERTION(offset < size_);
    ASSERTION(offset + count <= size_);

    size_t tmp = 0;

    list_block::iterator first = blocks_.begin();
    
    while (true) {
        tmp += (*first)->get_size();
        if (tmp >= offset) break;
        first++;
    }

    list_block::iterator last = first;
    while (tmp < offset + count) {
        tmp += (*last)->get_size();
        // We increment before the check because we return an open range
        last++;
    }

    return range_block(first, last);
}

gmacError_t
mapping::dup2(mapping_ptr map1, hal::ptr::offset_type off1,
              mapping_ptr map2, hal::ptr::offset_type off2, size_t count)
{
    gmacError_t ret = gmacSuccess;

    return ret;
}

gmacError_t
mapping::dup(hal::ptr::offset_type off1, mapping_ptr map2,
             hal::ptr::offset_type off2, size_t count)
{
    gmacError_t ret = gmacSuccess;

    list_block::iterator it;
    hal::ptr::offset_type off = off2;

    // Move to the first block involved
    for (it = map2->blocks_.begin(); off < off2; off += (*it)->get_size());
    ASSERTION(off == off2, "map2 should contain a block starting @ address: %p", off2);

    // Duplicate all the blocks
    for (it = map2->blocks_.begin(); off < off2 + count; off += (*it)->get_size()) {
        ASSERTION(it != map2->blocks_.end(), "unexpected end of container");
        append(*it);
    }
    ASSERTION(off == off2 + count, "loop should end after a block boundary");

    return ret;
}

gmacError_t
mapping::split(hal::ptr::offset_type off, size_t count)
{
    return gmacSuccess;
}

gmacError_t
mapping::prepend(coherence::block_ptr b)
{
    gmacError_t ret = gmacSuccess;

    if (b->get_size() <= addr_.get_offset()) {
        blocks_.push_front(b);
        addr_ -= b->get_size();
        size_ += b->get_size();
    } else {
        ret = gmacErrorInvalidValue;
    }

    return ret;
}

gmacError_t
mapping::append(coherence::block_ptr b)
{
    gmacError_t ret = gmacSuccess;

    if (b->get_size() > 0) {
        blocks_.push_back(b);
        size_ += b->get_size();
    } else {
        ret = gmacErrorInvalidValue;
    }

    return ret;
}

gmacError_t
mapping::append(mapping_ptr map)
{
    if ((map->addr_.get_offset()  <  (addr_.get_offset() + size_)) ||
        (map->addr_.get_aspace() !=  (addr_.get_aspace()))) {
        return gmacErrorInvalidValue;
    }

    gmacError_t ret = gmacSuccess;

    // Add a new block between the mappings if needed
    if (map->get_bounds().start > get_bounds().end) {
        coherence::block_ptr b = factory_block::create(map->get_bounds().start - get_bounds().end);

        blocks_.push_back(b);

        size_ += b->get_size(); 
    }

    // Insert the rest of blocks into the map
    for (list_block::iterator it  = map->blocks_.begin();
                              it != map->blocks_.end();
                              it++) {
        blocks_.push_back(*it);

        size_ += (*it)->get_size(); 
    }
      
    return ret;
}

mapping::mapping(hal::ptr addr) :
    addr_(addr),
    size_(0)
{
}

mapping::mapping(const mapping &m) :
    addr_(m.addr_),
    size_(m.size_),
    blocks_(m.blocks_)
{
}

gmacError_t
mapping::acquire(size_t offset, size_t count, int flags)
{
    gmacError_t err = gmacSuccess;

    range_block range = get_blocks_in_range(offset, count);

    for (range_block::iterator i = range.begin(); i != range.end(); ++i) {
        err = (*i)->acquire(flags);
        if (err != gmacSuccess) break;
    }

    return err;
}

gmacError_t
mapping::release(size_t offset, size_t count)
{
    gmacError_t err = gmacSuccess;

    range_block range = get_blocks_in_range(offset, count);

    for (range_block::iterator i = range.begin(); i != range.end(); i++) {
        err = (*i)->release();
        if (err != gmacSuccess) break;
    }

    return err;
}

}}

/* vim:set backspace=2 tabstop=4 shiftwidth=4 textwidth=120 foldmethod=marker expandtab: */
