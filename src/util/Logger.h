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

#ifndef __UTIL_LOGGER_H_
#define __UTIL_LOGGER_H_

#include <config.h>

#include "Parameter.h"

#include <map>
#include <string>
#include <iostream>
#include <cstdarg>
#include <cassert>

#define ASSERT_STRING "in function %s [%s:%d]", __func__, __FILE__, __LINE__
#define assertion(c, ...) __assertion(c, ASSERT_STRING)

namespace gmac { namespace util {

class Lock;
class Logger {
protected:
    static Parameter<const char *> *Level;
    static const char *debugString;
    static Lock lock;

    const char *name;
    bool active;
    std::ostream *out;

    static const size_t BufferSize = 1024;
    static char buffer[BufferSize];

    void log(std::string tag, const char *fmt, va_list list) const;

public:
    Logger(const char *name);

    void trace(const char *fmt, ...) const; 
    void warning(const char *fmt, ...) const;
    void __assertion(unsigned c, const char *fmt, ...) const;
    void fatal(const char *fmt, ...) const;
    void cfatal(unsigned c, const char *fmt, ...) const;
};

}}

#include "Logger.ipp"

#endif
