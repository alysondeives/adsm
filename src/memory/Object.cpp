#include "config/config.h"
#include "core/IOBuffer.h"
#include "core/Mode.h"

#include "Object.h"
#include "memory/Memory.h"

namespace __impl { namespace memory {

#ifdef DEBUG
Atomic Object::Id_ = 0;
#endif

Object::BlockMap::const_iterator Object::firstBlock(size_t objectOffset, size_t &blockOffset) const
{
    BlockMap::const_iterator i = blocks_.begin();
    if(i == blocks_.end()) return i;
    while(objectOffset >= i->second->size()) {
        objectOffset -= i->second->size();
        i++;
        if(i == blocks_.end()) return i;
    }
    blockOffset = objectOffset;
    return i;
}

gmacError_t Object::coherenceOp(gmacError_t (Protocol::*f)(Block &))
{
    gmacError_t ret = gmacSuccess;
    BlockMap::const_iterator i;
    for(i = blocks_.begin(); i != blocks_.end(); i++) {
        ret = i->second->coherenceOp(f);
        if(ret != gmacSuccess) break;
    }
    return ret;
}

gmacError_t Object::memoryOp(Protocol::MemoryOp op,
                             core::IOBuffer &buffer, size_t size, size_t bufferOffset, size_t objectOffset)
{
    gmacError_t ret = gmacSuccess;
    size_t blockOffset = 0;
    BlockMap::const_iterator i = firstBlock(objectOffset, blockOffset);
    for(; i != blocks_.end() && size > 0; i++) {
        size_t blockSize = i->second->size() - blockOffset;
        blockSize = size < blockSize? size: blockSize;
        buffer.wait();
        ret = i->second->memoryOp(op, buffer, blockSize, bufferOffset, blockOffset);
        blockOffset = 0;
        bufferOffset += blockSize;
        size -= blockSize;
    }
    return ret;
}


gmacError_t Object::memset(size_t offset, int v, size_t size)
{
    gmacError_t ret = gmacSuccess;
    size_t blockOffset = 0;
    BlockMap::const_iterator i = firstBlock(offset, blockOffset);
    for(; i != blocks_.end() && size > 0; i++) {
        size_t blockSize = i->second->size() - blockOffset;
        blockSize = size < blockSize? size: blockSize;
        ret = i->second->memset(v, blockSize, blockOffset);
        blockOffset = 0;
        size -= blockSize;
    }
    return ret;
}

gmacError_t
Object::memcpyToObject(core::Mode &mode, size_t objOffset, const hostptr_t src, size_t size)
{
    trace::EnterCurrentFunction();
    gmacError_t ret = gmacSuccess;

    // We need to I/O buffers to double-buffer the copy
    core::IOBuffer *active;
    core::IOBuffer *passive;

    // Control variables
    size_t left = size;

    // Adjust the first copy to deal with a single block
    size_t copySize = size < blockEnd(objOffset)? size: blockEnd(objOffset);

    size_t bufSize = size < blockSize()? size: blockSize();
    active = &mode.createIOBuffer(bufSize);
    ASSERTION(bufSize >= copySize);

    if (copySize < size) {
        passive = &mode.createIOBuffer(bufSize);
    } else {
        passive = NULL;
    }

    // Copy the data to the first block
    ::memcpy(active->addr(), src, copySize);

    hostptr_t ptr = src;
    while(left > 0) {
        // We do not need for the active buffer to be full because ::memcpy() is
        // a synchronous call
        ret = copyFromBuffer(*active, copySize, 0, objOffset);
        ASSERTION(ret == gmacSuccess);
        //if (ret != gmacSuccess) return ret;
        ptr       += copySize;
        left      -= copySize;
        objOffset += copySize;
        if(left > 0) {
            // Start copying data from host memory to the passive I/O buffer
            copySize = (left < passive->size()) ? left : passive->size();
            ASSERTION(bufSize >= copySize);
            passive->wait(); // Avoid overwritten a buffer that is already in use
            ::memcpy(passive->addr(), ptr, copySize);
        }
        // Swap buffers
        core::IOBuffer *tmp = active;
        active = passive;
        passive = tmp;
    }
    // Clean up buffers after they are idle
    if (passive != NULL) {
        passive->wait();
        mode.destroyIOBuffer(*passive);
    }
    if (active  != NULL) {
        active->wait();
        mode.destroyIOBuffer(*active);
    }

    trace::ExitCurrentFunction();
    return ret;
}

gmacError_t
Object::memcpyObjectToObject(core::Mode &mode,
                              Object &dstObj, size_t dstOffset,
                              size_t srcOffset, size_t size)
{
    trace::EnterCurrentFunction();
    gmacError_t ret = gmacSuccess;

    hostptr_t dstPtr = dstObj.addr() + dstOffset;
    hostptr_t srcPtr = addr() + srcOffset;

    core::Mode &dstOwner = dstObj.owner(mode, dstPtr);
    core::Mode &srcOwner = owner(mode, srcPtr);

    if (__impl::util::params::ParamMemcpyAccToAcc &&
        dstObj.acceleratorAddr(dstOwner, dstPtr).pasId_ ==
        acceleratorAddr(srcOwner, srcPtr).pasId_) {
        TRACE(LOCAL, "Using fast path!: %p -> %p ("FMT_SIZE")",        addr() + srcOffset,
                                                                dstObj.addr() + dstOffset, size);
        accptr_t dstPtr = dstObj.acceleratorAddr(dstOwner, dstObj.addr() + dstOffset);
        accptr_t srcPtr = acceleratorAddr(srcOwner, addr() + srcOffset);

        lockWrite();
        dstObj.lockWrite();
        size_t dummyOffset = 0;
        BlockMap::const_iterator i = firstBlock(srcOffset, dummyOffset);
        TRACE(LOCAL, "FP: %p "FMT_SIZE, dstObj.addr() + dstOffset, size);
        BlockMap::const_iterator j = dstObj.firstBlock(dstOffset, dummyOffset);
        TRACE(LOCAL, "FP: %p vs %p "FMT_SIZE, j->second->addr(), dstObj.addr() + dstOffset, size);
        for (; j != dstObj.blocks_.end() && (j->second->addr() < dstObj.addr() + dstOffset + size); j++) {
            size_t copySize = size < dstObj.blockEnd(dstOffset)? size: dstObj.blockEnd(dstOffset);
            // Single copy from the source to fill the buffer
            if (copySize <= blockEnd(srcOffset)) {
                TRACE(LOCAL, "FP: Copying1: "FMT_SIZE" bytes", copySize);
                ret = i->second->copyOp(&Protocol::copyBlockToBlock, *j->second, dstOffset % blockSize(), srcOffset % blockSize(), copySize);
                ASSERTION(ret == gmacSuccess);
                i++;
            }
            else { // Two copies from the source to fill the buffer
                TRACE(LOCAL, "FP: Copying2: "FMT_SIZE" bytes", copySize);
                size_t firstCopySize = blockEnd(srcOffset);
                size_t secondCopySize = copySize - firstCopySize;

                ret = i->second->copyOp(&Protocol::copyBlockToBlock, *j->second,
                                      dstOffset % blockSize(),
                                      srcOffset % blockSize(),
                                      firstCopySize);
                ASSERTION(ret == gmacSuccess);
                i++;
                ret = i->second->copyOp(&Protocol::copyBlockToBlock, *j->second,
                                      (dstOffset + firstCopySize) % blockSize(),
                                      (srcOffset + firstCopySize) % blockSize(),
                                      secondCopySize);
                ASSERTION(ret == gmacSuccess);
            }
            dstOffset += copySize;
            srcOffset += copySize;
        }

        dstObj.unlock();
        unlock();

        TRACE(LOCAL, "Fast path finished!");

        trace::ExitCurrentFunction();
        return ret;
    }

    // We need to I/O buffers to double-buffer the copy
    core::IOBuffer *active;
    core::IOBuffer *passive;

    // Control variables
    size_t left = size;

    // Adjust the first copy to deal with a single block
    size_t copySize = size < dstObj.blockEnd(dstOffset)? size: dstObj.blockEnd(dstOffset);

    size_t bufSize = size < dstObj.blockSize()? size: dstObj.blockSize();
    active = &mode.createIOBuffer(bufSize);
    ASSERTION(bufSize >= copySize);

    if (copySize < size) {
        passive = &mode.createIOBuffer(bufSize);
    } else {
        passive = NULL;
    }

    // Single copy from the source to fill the buffer
    if (copySize <= blockEnd(srcOffset)) {
        ret = copyToBuffer(*active, copySize, 0, srcOffset);
        ASSERTION(ret == gmacSuccess);
    }
    else { // Two copies from the source to fill the buffer
        size_t firstCopySize = blockEnd(srcOffset);
        size_t secondCopySize = copySize - firstCopySize;
        ASSERTION(bufSize >= firstCopySize + secondCopySize);

        ret = copyToBuffer(*active, firstCopySize, 0, srcOffset);
        ASSERTION(ret == gmacSuccess);
        ret = copyToBuffer(*active, secondCopySize, firstCopySize, srcOffset + firstCopySize);
        ASSERTION(ret == gmacSuccess);
    }

    // Copy first chunk of data
    while(left > 0) {
        active->wait(); // Wait for the active buffer to be full
        ret = dstObj.copyFromBuffer(*active, copySize, 0, dstOffset);
        if(ret != gmacSuccess) {
            trace::ExitCurrentFunction();
            return ret;
        }
        left -= copySize;
        srcOffset += copySize;
        dstOffset += copySize;
        if(left > 0) {
            copySize = (left < dstObj.blockSize()) ? left: dstObj.blockSize();
            ASSERTION(bufSize >= copySize);
            // Avoid overwritting a buffer that is already in use
            passive->wait();

            // Request the next copy
            // Single copy from the source to fill the buffer
            if (copySize <= blockEnd(srcOffset)) {
                ret = copyToBuffer(*passive, copySize, 0, srcOffset);
                ASSERTION(ret == gmacSuccess);
            }
            else { // Two copies from the source to fill the buffer
                size_t firstCopySize = blockEnd(srcOffset);
                size_t secondCopySize = copySize - firstCopySize;
                ASSERTION(bufSize >= firstCopySize + secondCopySize);

                ret = copyToBuffer(*passive, firstCopySize, 0, srcOffset);
                ASSERTION(ret == gmacSuccess);
                ret = copyToBuffer(*passive, secondCopySize, firstCopySize, srcOffset + firstCopySize);
                ASSERTION(ret == gmacSuccess);
            }

            // Swap buffers
            core::IOBuffer *tmp = active;
            active = passive;
            passive = tmp;
        }
    }
    // Clean up buffers after they are idle
    if (passive != NULL) {
        passive->wait();
        mode.destroyIOBuffer(*passive);
    }
    if (active  != NULL) {
        active->wait();
        mode.destroyIOBuffer(*active);
    }

    trace::ExitCurrentFunction();
    return ret;
}

gmacError_t
Object::memcpyFromObject(core::Mode &mode, hostptr_t dst,
                          size_t objOffset, size_t size)
{
    trace::EnterCurrentFunction();
    gmacError_t ret = gmacSuccess;

    // We need to I/O buffers to double-buffer the copy
    core::IOBuffer *active;
    core::IOBuffer *passive;

    // Control variables
    size_t left = size;

    // Adjust the first copy to deal with a single block
    size_t copySize = size < blockEnd(objOffset)? size: blockEnd(objOffset);

    size_t bufSize = size < blockSize()? size: blockSize();
    active = &mode.createIOBuffer(bufSize);
    ASSERTION(bufSize >= copySize);

    if (copySize < size) {
        passive = &mode.createIOBuffer(bufSize);
    } else {
        passive = NULL;
    }

    // Copy the data to the first block
    ret = copyToBuffer(*active, copySize, 0, objOffset);
    ASSERTION(ret == gmacSuccess);
    //if(ret != gmacSuccess) return ret;
    while(left > 0) {
        // Save values to use when copying the buffer to host memory
        size_t previousCopySize = copySize;
        left      -= copySize;
        objOffset += copySize;
        if(left > 0) {
            // Start copying data from host memory to the passive I/O buffer
            copySize = (left < passive->size()) ? left : passive->size();
            ASSERTION(bufSize >= copySize);
            // No need to wait for the buffer, because ::memcpy is a
            // synchronous call
            ret = copyToBuffer(*passive, copySize, 0, objOffset);
            ASSERTION(ret == gmacSuccess);
        }
        // Wait for the active buffer to be full
        active->wait();
        // Copy the active buffer to host
        ::memcpy(dst, active->addr(), previousCopySize);
        dst += previousCopySize;

        // Swap buffers
        core::IOBuffer *tmp = active;
        active = passive;
        passive = tmp;
    }
    // No need to wait for the buffers because we waited for them before ::memcpy
    if (passive != NULL) mode.destroyIOBuffer(*passive);
    if (active  != NULL) mode.destroyIOBuffer(*active);

    trace::ExitCurrentFunction();
    return ret;
}



}}
