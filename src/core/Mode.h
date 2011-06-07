/* Copyright (c) 2009, 2010 University of Illinois
                   Universitat Politecnica de Catalunya
                   All rights reserved.

Developed by: IMPACT Research Group / Grup de Sistemes Operatius
              University of Illinois / Universitat Politecnica de Catalunya
              http://impact.crhc.illinois.edu/
              http://gso.ac.upc.edu/

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to
deal with the Software without restriction, including without limitation the
rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
  1. Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimers.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimers in the
     documentation and/or other materials provided with the distribution.
  3. Neither the names of IMPACT Research Group, Grup de Sistemes Operatius,
     University of Illinois, Universitat Politecnica de Catalunya, nor the
     names of its contributors may be used to endorse or promote products
     derived from this Software without specific prior written permission.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
WITH THE SOFTWARE.  */

#ifndef GMAC_CORE_MODE_H_
#define GMAC_CORE_MODE_H_

#include "config/common.h"

#ifdef USE_VM
#include "memory/vm/Bitmap.h"
#endif
#include "util/Lock.h"
#include "util/NonCopyable.h"
#include "util/Reference.h"
#include "util/Atomics.h"

#include "memory/ObjectMap.h"

namespace __impl {

namespace memory {
class Protocol;
}

namespace core {

class IOBuffer;
class Process;

/**
 * A Mode represents the address space of a thread in an accelerator. Each
 * thread has one mode per accelerator type in the system
 */
class GMAC_LOCAL Mode :
    public util::Reference,
    public util::NonCopyable,
    public gmac::util::SpinLock {
protected:
    static Atomic Count_;

    unsigned id_;
    memory::Protocol *protocol_;

    bool validObjects_;
    bool releasedObjects_;

    gmacError_t error_;

#ifdef USE_VM
    __impl::memory::vm::Bitmap bitmap_;
#endif

    virtual memory::ObjectMap &getObjectMap() = 0;
    virtual const memory::ObjectMap &getObjectMap() const = 0;

    /**
     * Mode constructor
     */
    Mode();

    /**
     * Mode destructor
     */
    virtual ~Mode();

public:

    /**
     * Gets a reference to the memory protocol used by the mode
     * \return A reference to the memory protocol used by the mode
     */
    memory::Protocol &protocol();

    /**
     * Gets a numeric identifier for the mode. This identifier must be unique.
     * \return A numeric identifier for the mode
     */
    unsigned id() const;

    /**
     * Returns the last error code
     * \return The last error code
     */
    gmacError_t error() const;

    /**
     * Sets up the last error code
     * \param err Error code
     */
    void error(gmacError_t err);


    /**
     * Adds an object to the map of the mode
     * \param obj A reference to the object to be added
     */
    virtual void addObject(memory::Object &obj);

    /**
     * Removes an object from the map of the mode
     * \param obj A reference to the object to be removed
     */
    virtual void removeObject(memory::Object &obj);

    /**
     * Insert an object into the orphan list
     * \param obj Object to be inserted
     */
    virtual void insertOrphan(memory::Object &obj) = 0;

    /**
     * Gets the first object that belongs to the memory range
     * \param addr Starting address of the memory range
     * \param size Size of the memory range
     * \return A pointer of the Object that contains the address or NULL if
     * there is no Object at that address
     */
    memory::Object *getObject(const hostptr_t addr, size_t size = 0) const;

    /**
     * Applies a constant memory operation to all the objects that belong to
     * the mode
     * \param op Memory operation to be executed
     * \sa __impl::memory::Object::acquire
     * \sa __impl::memory::Object::toHost
     * \sa __impl::memory::Object::toAccelerator
     * \return Error code
     */
    gmacError_t forEachObject(gmacError_t (memory::Object::*op)(void));


    /**
     * Applies a constant memory operation to all the objects that belong to
     * the mode
     * \param op Memory operation to be executed
     * \sa __impl::memory::Object::acquire
     * \sa __impl::memory::Object::toHost
     * \sa __impl::memory::Object::toAccelerator
     * \return Error code
     */
    gmacError_t forEachObject(gmacError_t (memory::Object::*op)(void) const) const;

    /**
     * Tells if the objects of the mode have been already invalidated
     * \return Boolean that tells if objects of the mode have been already
     * invalidated 
     */
    bool validObjects() const;

    /**
     * Notifies the mode that one (or several) of its objects have been validated
     */
    void validateObjects();

    /**
     * Notifies the mode that one (or several) of its objects has been invalidated
     */
    void invalidateObjects();

    /**
     * Tells if the objects of the mode have been already released to the
     * accelerator
     * \return Boolean that tells if objects of the mode have been already
     * released to the accelerator
     */
    bool releasedObjects() const;

    /**
     * Releases the ownership of the objects of the mode to the accelerator
     * and waits for pending transfers
     */
    virtual gmacError_t releaseObjects() = 0;

    /**
     * Waits for kernel execution and acquires the ownership of the objects
     * of the mode from the accelerator
     */
    virtual gmacError_t acquireObjects() = 0;


    /**
     * Maps the given host memory on the accelerator memory
     * \param dst Reference to a pointer where to store the accelerator
     * address of the mapping
     * \param src Host address to be mapped
     * \param size Size of the mapping
     * \param align Alignment of the memory mapping. This value must be a
     * power of two
     * \return Error code
     */
    virtual gmacError_t map(accptr_t &dst, hostptr_t src, size_t size, unsigned align = 1) = 0;

    /** Allocate GPU-accessible host memory
     *
     *  \param addr Pointer of the memory to be mapped to the accelerator
     *  \param size Size (in bytes) of the host memory to be mapped
     *  \return Error code
     */
    virtual gmacError_t hostAlloc(hostptr_t &addr, size_t size) = 0;

    /** Release GPU-accessible host memory 
     *
     *  \param addr Starting address of the host memory to be released
     *  \return Error code
     */
    virtual gmacError_t hostFree(hostptr_t addr) = 0;


    /** Gets the GPU memory address where the given GPU-accessible host
     *  memory pointer is mapped
     *
     *  \param addr Host memory address
     *  \return Device memory address
     */
    virtual accptr_t hostMapAddr(const hostptr_t addr) = 0;


    /**
     * Unmaps the memory previously mapped by map
     * \param addr Host memory allocation to be unmap
     * \param size Size of the unmapping
     * \return Error code
     */
    virtual gmacError_t unmap(hostptr_t addr, size_t size) = 0;

    /**
     * Copies data from system memory to accelerator memory
     * \param acc Destination accelerator pointer
     * \param host Source host pointer
     * \param size Number of bytes to be copied
     * \return Error code
     */
    virtual gmacError_t copyToAccelerator(accptr_t acc, const hostptr_t host, size_t size) = 0;

    /**
     * Copies data from accelerator memory to system memory
     * \param host Destination host pointer
     * \param acc Source accelerator pointer
     * \param size Number of bytes to be copied
     * \return Error code
     */
    virtual gmacError_t copyToHost(hostptr_t host, const accptr_t acc, size_t size) = 0;

    /** Copies data from accelerator memory to accelerator memory
     * \param dst Destination accelerator memory
     * \param src Source accelerator memory
     * \param size Number of bytes to be copied
     * \return Error code
     */
    virtual gmacError_t copyAccelerator(accptr_t dst, const accptr_t src, size_t size) = 0;

    /**
     * Sets the contents of accelerator memory
     * \param addr Pointer to the accelerator memory to be set
     * \param c Value used to fill the memory
     * \param size Number of bytes to be set
     * \return Error code
     */
    virtual gmacError_t memset(accptr_t addr, int c, size_t size) = 0;

    /**
     * Creates an IOBuffer
     * \param size Minimum size of the buffer
     * \return A pointer to the created IOBuffer or NULL if there is not enough
     *         memory
     */
    virtual IOBuffer &createIOBuffer(size_t size) = 0;

    /**
     * Destroys an IOBuffer
     * \param buffer Pointer to the buffer to be destroyed
     */
    virtual void destroyIOBuffer(IOBuffer &buffer) = 0;

    /** Copies size bytes from an IOBuffer to accelerator memory
     * \param dst Pointer to accelerator memory
     * \param buffer Reference to the source IOBuffer
     * \param size Number of bytes to be copied
     * \param off Offset within the buffer
     */
    virtual gmacError_t bufferToAccelerator(accptr_t dst, IOBuffer &buffer, size_t size, size_t off = 0) = 0;

    /**
     * Copies size bytes from accelerator memory to a IOBuffer
     * \param buffer Reference to the destination buffer
     * \param dst Pointer to accelerator memory
     * \param size Number of bytes to be copied
     * \param off Offset within the buffer
     */
    virtual gmacError_t acceleratorToBuffer(IOBuffer &buffer, const accptr_t dst, size_t size, size_t off = 0) = 0;

    /** Returns the memory information of the accelerator on which the mode runs
     * \param free A reference to a variable to store the memory available on the
     * accelerator
     * \param total A reference to a variable to store the total amount of memory
     * on the accelerator
     */
    virtual void memInfo(size_t &free, size_t &total) = 0;

#ifdef USE_VM
    memory::vm::Bitmap &getBitmap();
    const memory::vm::Bitmap &getBitmap() const;
#endif
};

}}

#include "Mode-impl.h"

#endif

/* vim:set backspace=2 tabstop=4 shiftwidth=4 textwidth=120 foldmethod=marker expandtab: */
