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

#ifndef __MEMORY_LAZYMANAGER_H_
#define __MEMORY_LAZYMANAGER_H_

#include "MemManager.h"
#include "MemHandler.h"
#include "MemMap.h"
#include "ProtRegion.h"

#include <config/threads.h>
#include <config/debug.h>

#include <map>

namespace gmac {

//! Manager that Moves Memory Regions Lazily
class LazyManager : public MemManager, public MemHandler {
protected:
	MemMap<ProtRegion> memMap;
public:
	LazyManager() : MemManager(), MemHandler() { }
	bool alloc(void *addr, size_t count);
	void *safeAlloc(void *addr, size_t count);
	void release(void *addr);
	void flush(void);
	void sync(void) {};
	size_t filter(const void *addr, size_t size, MemRegion *&region) {
		return memMap.filter(addr, size, region);
	}
	void invalidate(MemRegion *region) {
		dynamic_cast<ProtRegion *>(region)->invalidate();
	}
	void flush(MemRegion *region);

	ProtRegion *find(void *addr) { return memMap.find(addr); }
	void read(ProtRegion *region, void *addr);
	void write(ProtRegion *region, void *addr);
};

};

#endif
