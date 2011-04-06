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

#ifndef GMAC_API_OPENCL_HPE_MODE_H_
#define GMAC_API_OPENCL_HPE_MODE_H_

#include "config/common.h"
#include "config/config.h"

#include "api/opencl/Mode.h"
#include "core/hpe/Mode.h"

namespace __impl {
    
namespace core {
class IOBuffer;
}

namespace opencl { namespace hpe {

class Accelerator;
class Context;

class GMAC_LOCAL ContextLock : public gmac::util::Lock {
    friend class Mode;
public:
    ContextLock() : gmac::util::Lock("Context") {}
};

class GMAC_LOCAL KernelList :
    protected std::list<core::hpe::Kernel *>,
    public gmac::util::Lock
{
protected:
    typedef std::list<core::hpe::Kernel *> Parent;
public:
    KernelList();
    virtual ~KernelList();

    void insert(core::hpe::Kernel *);
};

//! A Mode represents a virtual CUDA accelerator on an execution thread
class GMAC_LOCAL Mode : public gmac::core::hpe::Mode, public opencl::Mode {
    DBC_FORCE_TEST(Mode)

    friend class IOBuffer;
protected:
    //! Switch to accelerator mode
    void switchIn();

    //! Switch back to CPU mode
    void switchOut();

    void reload();

    //! Get the main mode context
    /*!
        \return Main mode context
    */
    core::hpe::Context &getContext();
    Context &getCLContext();

    cl_program program_;
    KernelList kernelList_;
public:
    //! Default constructor
    /*!
        \param proc Process where the mode is attached
        \param acc Virtual CUDA accelerator where the mode is executed
    */
    Mode(core::hpe::Process &proc, Accelerator &acc);

    //! Default destructor
    virtual ~Mode();

    //! Allocate GPU-accessible host memory
    /*!
        \param addr Pointer of the memory to be mapped to the accelerator
        \param size Size (in bytes) of the host memory to be mapped
        \return Error code
    */
    gmacError_t hostAlloc(hostptr_t &addr, size_t size);

    //! Release GPU-accessible host memory 
    /*!
        \param addr Starting address of the host memory to be released
        \return Error code
    */
    gmacError_t hostFree(hostptr_t addr);

    /** Gets the GPU memory address where the given GPU-accessible host
     *  memory pointer is mapped
     *
     *  \param addr Host memory address
     *  \return Device memory address
     */
    accptr_t hostMapAddr(const hostptr_t addr);

    core::hpe::KernelLaunch &launch(gmacKernel_t kernel);

    //! Execute a kernel on the accelerator
    /*!
        \param launch Structure defining the kernel to be executed
        \return Error code
    */
	gmacError_t execute(core::hpe::KernelLaunch &launch);

    //! Create an IO buffer to sent / receive data from the accelerator
    /*!
        \param size Size (in bytes) of the IO buffer
        \return Pointer to the created I/O buffer or NULL if not enough memory
    */
    core::IOBuffer &createIOBuffer(size_t size);

    //! Destroy (release) an I/O buffer
    /*!
        \param buffer I/O buffer to be released
    */
    void destroyIOBuffer(core::IOBuffer &buffer);

    /** Send data from an I/O buffer to the accelerator
     *
     *  \param dst Accelerator memory where data will be written to
     *  \param buffer I/O buffer where data will be read from
     *  \param size Size (in bytes) of the data to be copied
     *  \param off Offset (in bytes) in the I/O buffer where to start reading data from
     *  \return Error code
     */
    TESTABLE gmacError_t bufferToAccelerator(accptr_t dst, core::IOBuffer &buffer, size_t size, size_t off = 0);


    /** Fill I/O buffer with data from the accelerator
     *
     *  \param buffer I/O buffer where data will be stored
     *  \param src Accelerator memory where the data will be read from
     *  \param size Size (in bytes) of the data to be copied
     *  \param off Offset (in bytes) in the I/O buffer where to start writing data to
     *  \return Error code
     */
    TESTABLE gmacError_t acceleratorToBuffer(core::IOBuffer &buffer, const accptr_t src, size_t size, size_t off = 0);

    //! Get the accelerator stream where events are recorded
    /*!
        \return Command queue where events are recorded
    */
    cl_command_queue eventStream();


    //! Block the CPU thread until an event happens
    /*!
        \param event Event to wait for
        \return Error code
    */
    gmacError_t waitForEvent(cl_event event);

    //! Get the current (active) execution mode
    /*!
        \return Current (active) execution mode or NULL if no mode is active
    */
    static Mode & getCurrent();

    /** Get the physical accelerator associated to the mode
     * 
     * \return Physical accelerator associated to the mode
     */
    Accelerator &getAccelerator() const;

    gmacError_t call(cl_uint workDim, size_t *globalWorkOffset, size_t *globalWorkSize, size_t *localWorkSize);
	gmacError_t argument(const void *arg, size_t size, unsigned index);

    gmacError_t eventTime(uint64_t &t, cl_event start, cl_event end);
};

}}}

#include "Mode-impl.h"

#ifdef USE_DBC
#include "dbc/Mode.h"
#endif

#endif