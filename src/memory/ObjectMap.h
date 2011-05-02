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

#ifndef GMAC_MEMORY_MAP_H_
#define GMAC_MEMORY_MAP_H_

#include <map>
#include <set>

#include "config/common.h"
#include "util/Lock.h"
#include "util/NonCopyable.h"

#include "protocol/common/BlockState.h"

namespace __impl {

namespace core {
class Mode;
class Process;
namespace hpe { class Map; }
}

namespace memory {
class Object;
class Protocol;


//! A map of objects that is not bound to any Mode
class GMAC_LOCAL ObjectMap :
     protected gmac::util::RWLock, protected std::map<const hostptr_t, Object *> {
protected:
    friend class core::hpe::Map;
    typedef std::map<const hostptr_t, Object *> Parent;

#ifdef DEBUG
    unsigned nextObjectId_;
#endif

    /**
     * Find an object in the map
     *
     * \param addr Starting memory address within the object to be found
     * \param size Size (in bytes) of the memory range where the object can be
     * found
     * \return First object inside the memory range. NULL if no object is found
     */
    Object *mapFind(const hostptr_t addr, size_t size) const;
public:
    /**
     * Default constructor
     *
     * \param name Name of the object map used for tracing
     */
    ObjectMap(const char *name);

    /**
     * Default destructor
     */
    virtual ~ObjectMap();

    /**
     * Get the number of objects in the map
     *
     * \return Number of objects in the map
     */
    size_t size() const;

    /**
     * Insert an object in the map
     *
     * \param obj Object to insert in the map
     * \return True if the object was successfuly inserted
     */
    virtual bool insert(Object &obj);

    /**
     * Remove an object from the map
     *
     * \param obj Object to remove from the map
     * \return True if the object was successfuly removed
     */
    virtual bool remove(Object &obj);

    /**
     * Find the firs object in a memory range
     *
     * \param addr Starting address of the memory range where the object is
     * located
     * \param size Size (in bytes) of the memory range where the object is
     * located
     * \return First object within the memory range. NULL if no object is found
     */
    virtual Object *get(const hostptr_t addr, size_t size) const;

    /**
     * Get the amount of memory consumed by all objects in the map
     *
     * \return Size (in bytes) of the memory consumed by all objects in the map
     */
    size_t memorySize() const;

    /**
     * Execute an operation on all the objects in the map
     *
     * \param f Operation to be executed
     * \sa __impl::memory::Object::acquire
     * \sa __impl::memory::Object::toHost
     * \sa __impl::memory::Object::toAccelerator
     * \return Error code
     */
    gmacError_t forEachObject(gmacError_t (Object::*f)(void));
    gmacError_t forEachObject(gmacError_t (Object::*f)(void) const) const;

#ifdef DEBUG
    gmacError_t dumpObjects(const std::string &dir, std::string prefix, protocol::common::Statistic stat) const;
    gmacError_t dumpObject(const std::string &dir, std::string prefix, protocol::common::Statistic stat, hostptr_t ptr) const;
#endif

    /**
     * Execute an operation on all the objects in the map passing an argument
     *
     * \param f Operation to be executed
     * \param p1 Parameter to be passed
     * \sa __impl::memory::Object::removeOwner
     * \sa __impl::memory::Object::realloc
     * \return Error code
     */
    template <typename T>
    gmacError_t forEachObject(gmacError_t (Object::*f)(T &), T &p);
    template <typename T>
    gmacError_t forEachObject(gmacError_t (Object::*f)(T &) const, T &p) const;
};

}}

#include "ObjectMap-impl.h"

#endif
