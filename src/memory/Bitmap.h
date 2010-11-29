/* Copyright (c) 2009 University of Illinois
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

#ifndef GMAC_MEMORY_BITMAP_H_
#define GMAC_MEMORY_BITMAP_H_

#include "config/common.h"

#include "util/Lock.h"
#include "util/Logger.h"


#ifdef USE_VM

#ifdef BITMAP_BYTE
#else
#ifdef BITMAP_BIT
#else
#error "ERROR: Bitmap granularity not defined!"
#endif
#endif


namespace __impl { namespace memory  { namespace vm {

class GMAC_LOCAL Bitmap :
    public __impl::util::Logger,
    protected gmac::util::RWLock {
private:
    typedef uint8_t T;
    uint8_t *bitmap_;

    bool dirty_;
    bool synced_;

    uint8_t *device_;

    static const unsigned entriesPerByte;
    size_t shiftPage_;
#ifdef BITMAP_BIT
    uint32_t bitMask_;
#endif
    size_t size_;

    void allocate();

    template <bool check, bool clear, bool set>
    bool CheckClearSet(const void *addr);
    int minEntry_;
    int maxEntry_;

    void updateMaxMin(unsigned entry);

    unsigned offset(const void *addr) const;
public:
    Bitmap(unsigned bits = 32);
    virtual ~Bitmap();

    void cleanUp();

    void *device();
    void *host() const;

    const size_t size() const;

    bool check(const void *);
    bool checkAndClear(const void *);
    bool checkAndSet(const void *);
    void clear(const void *);
    void set(const void *);

    void newRange(const void * ptr, size_t count);
    void removeRange(const void * ptr, size_t count);

    bool clean() const;

    void syncHost();
    void syncDevice();
    void reset();

#ifdef DEBUG_BITMAP
    void dump();
#endif

    bool synced() const;
    void synced(bool s);
};

}}}

#include "Bitmap-impl.h"

#endif

#endif
