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

#ifndef GMAC_API_OPENCL_HPE_CONTEXT_H_
#define GMAC_API_OPENCL_HPE_CONTEXT_H_

#include <CL/cl.h>

#include <map>
#include <vector>

#include "config/common.h"
#include "config/config.h"

#include "core/hpe/Context.h"
#include "util/Lock.h"

namespace __impl {

namespace opencl {

namespace hpe {

class Accelerator;
class Mode;
class Kernel;
class KernelLaunch;

/**
 * Context implementation for OpenCL
 */
class GMAC_LOCAL Context : public gmac::core::hpe::Context {
    friend class ContextFactory;
protected:
    /** Delay for spin-locking */
        static const unsigned USleepLaunch_ = 100;

    /**
     * Default OpenCL context constructor
     * \param mode OpenCL execution mode associated to the context
     * \param stream OpenCL command queue to perform operations
     */
    Context(Mode &mode, cl_command_queue stream);

    /**
     * Default OpenCL context destructor
     */
    ~Context();


public:
    /**
     * Get the accelerator associated to the context
     * \return Reference to an OpenCL accelerator
     */
    Accelerator & accelerator();

    /**
     * Create a descriptor of a kernel invocation
     * \param kernel OpenCL kernel to be executed
     * \return Descriptor of the kernel invocation
     */
    KernelLaunch &launch(Kernel &kernel);

    /**
     * Wait for the accelerator to finish activities in all OpenCL command queues
     * \return Error code
     */
    gmacError_t waitAccelerator();

    /**
     * Get the default OpenCL command queue to request events
     * \return Default OpenCL command queue
     */
    const stream_t eventStream() const;

    /**
     * \todo: move this function to the generic context
     */
    gmacError_t memset(accptr_t addr, int c, size_t size);
};

}}}

#include "Context-impl.h"

#endif
