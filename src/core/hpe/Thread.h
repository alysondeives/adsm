/* Copyright (c) 2009, 2010, 2011 University of Illinois
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

#ifndef GMAC_CORE_HPE_THREAD_H_
#define GMAC_CORE_HPE_THREAD_H_

#include "core/Thread.h"

#include "util/Private.h"

namespace __impl { namespace core { namespace hpe {

class Mode;

class Thread;

class GMAC_LOCAL TLS :
    public __impl::core::TLS {
    friend class Thread;
private:
    static PRIVATE Mode *CurrentMode_;
    static PRIVATE Thread *CurrentThread_;

public:
    static Thread &getCurrentThread();
};

class Process;

/** Contains some thread-dependent values */
class GMAC_LOCAL Thread :
    public __impl::core::Thread {
private:
    Process &process_;

public:
    Thread(Process &proc);
    ~Thread();

    static Mode &getCurrentMode();
    static bool hasCurrentMode();
    static void setCurrentMode(Mode *mode);
};

}}}

#include "Thread-impl.h"

#ifdef USE_DBC
namespace __dbc { namespace core { namespace hpe {
    typedef __impl::core::hpe::Thread Thread;
}}}
#endif

#endif

/* vim:set backspace=2 tabstop=4 shiftwidth=4 textwidth=120 foldmethod=marker expandtab: */
