#ifdef USE_DBC

#include "memory/protocol/Lazy.h"

namespace __dbc { namespace memory { namespace protocol {

LazyBase::LazyBase(size_t limit) :
    __impl::memory::protocol::LazyBase(limit)
{
}

LazyBase::~LazyBase()
{
}

gmacError_t
LazyBase::signalRead(BlockImpl &_block, hostptr_t addr)
{
    LazyBlockImpl &block = dynamic_cast<LazyBlockImpl &>(_block);
    //REQUIRES(block.getState() == __impl::memory::protocol::lazy::Invalid);

    gmacError_t ret = Parent::signalRead(block, addr);

    return ret;
}

gmacError_t
LazyBase::signalWrite(BlockImpl &_block, hostptr_t addr)
{
    LazyBlockImpl &block = dynamic_cast<LazyBlockImpl &>(_block);
    gmacError_t ret = Parent::signalWrite(block, addr);

    ENSURES(block.getState() == __impl::memory::protocol::lazy::Dirty);

    return ret;
}

gmacError_t
LazyBase::acquire(BlockImpl &_block)
{
    LazyBlockImpl &block = dynamic_cast<LazyBlockImpl &>(_block);

    REQUIRES(block.getState() == __impl::memory::protocol::lazy::ReadOnly ||
             block.getState() == __impl::memory::protocol::lazy::Invalid);

    gmacError_t ret = Parent::acquire(block);

    ENSURES(block.getState() == __impl::memory::protocol::lazy::Invalid);

    return ret;
}

gmacError_t
LazyBase::release(BlockImpl &_block)
{
    LazyBlockImpl &block = dynamic_cast<LazyBlockImpl &>(_block);
    gmacError_t ret = Parent::release(block);

    ENSURES(block.getState() == __impl::memory::protocol::lazy::ReadOnly);

    return ret;
}

gmacError_t
LazyBase::releaseObjects()
{
    gmacError_t ret = Parent::releaseObjects();

    ENSURES(Parent::dbl_.size() == 0);

    return ret;
}

gmacError_t
LazyBase::toHost(BlockImpl &_block)
{
    LazyBlockImpl &block = dynamic_cast<LazyBlockImpl &>(_block);
    gmacError_t ret = Parent::toHost(block);

    ENSURES(block.getState() != __impl::memory::protocol::lazy::Invalid);

    return ret;
}

gmacError_t
LazyBase::copyToBuffer(BlockImpl &block, IOBufferImpl &buffer, size_t size,
    size_t bufferOffset, size_t blockOffset)
{
    REQUIRES(blockOffset  + size <= block.size());
    REQUIRES(bufferOffset + size <= buffer.size());

    gmacError_t ret = Parent::copyToBuffer(block, buffer, size, bufferOffset, blockOffset);

    return ret;
}

gmacError_t
LazyBase::copyFromBuffer(BlockImpl &block, IOBufferImpl &buffer, size_t size,
    size_t bufferOffset, size_t blockOffset)
{
    REQUIRES(blockOffset  + size <= block.size());
    REQUIRES(bufferOffset + size <= buffer.size());

    gmacError_t ret = Parent::copyFromBuffer(block, buffer, size, bufferOffset, blockOffset);

    return ret;
}

gmacError_t
LazyBase::memset(const BlockImpl &block, int v, size_t size, size_t blockOffset)
{
    REQUIRES(blockOffset + size <= block.size());

    gmacError_t ret = Parent::memset(block, v, size, blockOffset);

    return ret;
}

gmacError_t
LazyBase::flushDirty()
{
    gmacError_t ret = Parent::flushDirty();

    ENSURES(Parent::dbl_.size() == 0);

    return ret;
}

gmacError_t
LazyBase::copyBlockToBlock(Block &d, size_t dstOffset, Block &s, size_t srcOffset, size_t count)
{
    LazyBlockImpl &dst = dynamic_cast<LazyBlockImpl &>(d);
    LazyBlockImpl &src = dynamic_cast<LazyBlockImpl &>(s);

    REQUIRES(dstOffset + count <= dst.size());
    REQUIRES(srcOffset + count <= src.size());

    StateImpl dstState = dst.getState();
    StateImpl srcState = src.getState();

    gmacError_t ret = Parent::copyBlockToBlock(d, dstOffset, s, srcOffset, count);

    ENSURES(dst.getState() == dstState);
    ENSURES(src.getState() == srcState);

    return ret;
}

}}}

#endif // USE_DBC
