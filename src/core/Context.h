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

#ifndef GMAC_CORE_CONTEXT_H_
#define GMAC_CORE_CONTEXT_H_

#include "config/common.h"

#include "include/gmac/types.h"

#include "util/Lock.h"
#include "util/NonCopyable.h"
#include "util/Private.h"

namespace __impl { namespace core {

class Accelerator;
class Kernel;
class KernelLaunch;

/*!
	\brief Generic Context Class
*/
class GMAC_LOCAL Context : public gmac::util::RWLock, public util::NonCopyable {
protected:
    Accelerator &acc_;
    unsigned id_;

	Context(Accelerator &acc, unsigned id);
public:
	virtual ~Context();

    static void init();

	virtual gmacError_t copyToAccelerator(accptr_t acc, const hostptr_t host, size_t size);
	virtual gmacError_t copyToHost(hostptr_t host, const accptr_t acc, size_t size);
	virtual gmacError_t copyAccelerator(accptr_t dst, const accptr_t src, size_t size);

    virtual gmacError_t memset(accptr_t addr, int c, size_t size) = 0;

    virtual KernelLaunch &launch(Kernel &kernel) = 0;
    virtual gmacError_t sync() = 0;
};

}}

#endif
