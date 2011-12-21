/* Copyright (c) 2009-2011 University of Illinois
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

#ifndef GMAC_MEMORY_OBJECT_MAPPED_H_
#define GMAC_MEMORY_OBJECT_MAPPED_H_

#include <map>

#include "config/common.h"

#include "util/lock.h"
#include "util/Reference.h"

namespace __impl { 

namespace core {
	class address_space;
}

namespace memory {

class object_mapped;
typedef util::shared_ptr<object_mapped> object_mapped_ptr;

//! A set of Host-mapped memory blocks
/*! This class is actually a map, because we need to easily locate blocks
    using host memory addresses
*/
class GMAC_LOCAL set_object_mapped :
protected std::map<host_const_ptr, object_mapped_ptr>,
    public gmac::util::lock_rw<set_object_mapped> {
protected:
    friend class object_mapped;

    typedef std::map<host_const_ptr, object_mapped_ptr> Parent;
    typedef gmac::util::lock_rw<set_object_mapped> Lock;

    //! Inster a host mapped object in the set
    /*!
        \param object Host  mapped object to be inserted
        \return True if the block was inserted
    */
    bool add_object(object_mapped &object);

    //! Find a host mapped object that contains a memory address
    /*!
        \param addr Host memory address within the host mapped object
        \return Host mapped object containing the memory address. NULL if not found
    */
    object_mapped_ptr get_object(host_const_ptr addr) const;
public:
    //! Default constructor
    set_object_mapped();

    //! Default destructor
    ~set_object_mapped();

    //! Remove a block from the list that contains a given host memory address
    /*!
        \param addr Memory address within the block to be removed
        \return True if an object was removed
    */
    bool remove(host_const_ptr addr);
};

//! A memory object that only resides in host memory, but that is accessible from the accelerator
class GMAC_LOCAL object_mapped {
protected:
    //! Starting host memory address of the object
    hal::ptr addr_;

    //! Size (in bytes) of the object
    size_t size_;

    //! Set of all host mapped memory objects
    static set_object_mapped set_;

    hal::ptr alloc(core::address_space_ptr aspace, gmacError_t &err);
    void free(core::address_space_ptr aspace);

    hal::ptr getAccPtr(core::address_space_ptr aspace) const;

    core::address_space_ptr owner_;
public:
    /**
     * Default constructor
     * 
     * \param aspace Execution aspace creating the object
     * \param size Size (in bytes) of the object being created
     */
    object_mapped(core::address_space_ptr aspace, size_t size);

    /// Default destructor
    virtual ~object_mapped();

#ifdef USE_OPENCL
    gmacError_t acquire(core::address_space_ptr current);
    gmacError_t release(core::address_space_ptr current);
#endif
    
    /**
     * Get the starting host memory address of the object
     * 
     * \return Starting host memory address of the object
     */
    host_ptr addr() const;

    /**
     * Get the size (in bytes) of the object
     * 
     * \return Size (in bytes) of the object
     */
    size_t size() const;

    /**
     * Get an accelerator address where a host address within the object can be accessed
     *
     * \param current Reference to the aspace to which generate the address
     * \param addr Host memory address within the object
     * \return Accelerator memory address where the requested host memory address is mapped
     */
    hal::ptr get_device_addr(core::address_space_ptr current, host_const_ptr addr) const;

    /**
     * Remove a host mapped object from the list of all present host mapped object
     *
     * \param addr Host memory address within the object to be removed
     */
    static void remove(host_ptr addr);

    /**
     * Get the host mapped memory object that contains a given host memory address
     *
     * \param addr Host memory address within the object
     * \return Host mapped object cotainig the host memory address. NULL if not found
     */
    static object_mapped_ptr get_object(host_const_ptr addr);
};

}}

#include "object_mapped-impl.h"

#endif