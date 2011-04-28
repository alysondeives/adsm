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


#ifndef GMAC_API_OPENCL_HPE_KERNEL_H_
#define GMAC_API_OPENCL_HPE_KERNEL_H_

#include <CL/cl.h>

#include <list>

#include "config/common.h"
#include "core/hpe/Kernel.h"
#include "util/NonCopyable.h"

namespace __impl { namespace opencl { namespace hpe {

class Mode;

class KernelConfig;
class KernelLaunch;

/** Argument stack for a kernel call */
class GMAC_LOCAL Argument : public util::ReusableObject<Argument> {
	friend class Kernel;
protected:
    /** Maximun size of argument passed to the kernel */
    static const unsigned StackSize_ = 4096;

    /** Size (in bytes) of the arument passed to the kernel */
    size_t size_;

    /** Memory pool to store kernel argument */
    uint8_t stack_[StackSize_];
public:
    /**
     * Default constructor
     */
    Argument();

    /**
     * Sets an argument
     * \param ptr Pointer to memory where the argument value is
     * \param size Size (in bytes) of the argument
     */
    void setArgument(const void *ptr, size_t size);

    /**
     * Get the start of the memory pool where the artument is stored
     * \return Pointer to the first argument
     */
    const void * ptr() const { return stack_; }

    /**
     * Get the size (in bytes) of the argument
     * \return Size (in bytes) of the argument
     */
    size_t size() const { return size_; }
};


/** A kernel that can be executed by an OpenCL accelerator */
class GMAC_LOCAL Kernel : public gmac::core::hpe::Kernel {
    friend class KernelLaunch;
protected:
    /** OpenCL kernel object */
    cl_kernel f_;
    /** Number of arguments requried by the kernel */
    unsigned nArgs_;
public:
    /**
     * Default constructor
     * \param k Kernel descriptor that describes the kernel
     * \param kernel OpenCL kernel containing the kernel code
     */
    Kernel(const core::hpe::KernelDescriptor & k, cl_kernel kernel);

    /**
     * Default destructor
     */
    ~Kernel();

    /**
     * Get a kernel that can be executed by an execution mode
     * \param mode Execution mode capable of executing the kernel
     * \param stream OpenCL queue where the kernel can be executed
     * \return Invocable kernel
     */
    KernelLaunch *launch(Mode &mode, cl_command_queue stream);
};


class GMAC_LOCAL KernelConfig : protected std::vector<Argument> {
protected:
    /** Type containing a vector of arguments for a kernel */
    typedef std::vector<Argument> ArgsVector;
    /** Number of dimensions the kernel will execute */
    cl_uint workDim_;
    /** Index offsets for each kernel dimension */
    size_t *globalWorkOffset_;
    /** Number of elements per kernel dimension */
    size_t *globalWorkSize_;
    /** Number of elements per kernel work-group dimension */
    size_t *localWorkSize_;

public:
    /**
     * Default constructor
     * \param nArgs Number of arguments for the kernel
     */
    KernelConfig(unsigned nArgs);

    /**
     * Default destructor
     */
    ~KernelConfig();

    /**
     * Set the configuration parameters for a kernel
     * \param workDim Number of dimensions for the kernel
     * \param globalWorkOffset Index offsets for each dimenssion
     * \param globalWorkSize Number of elements per dimension
     * \param localWorkSize Number of work-group items per dimension
     */
    void setConfiguration(cl_uint workDim, size_t *globalWorkOffset,
        size_t *globalWorkSize, size_t *localWorkSize);

    /**
     * Set a new argument for the kernel
     * \param arg Pointer to the value for the argument
     * \param size Size (in bytes) of the argument
     * \param index Index of the argument in the argument list
     */
    void setArgument(const void * arg, size_t size, unsigned index);

    /**
     * Default assugment operator
     * \param config Kernel configuration to be assigned
     * \return New kernel configuration object
     */
    KernelConfig &operator=(const KernelConfig &config);

    /**
     * Get the number of dimensions
     * \return Number of dimensions
     */
    cl_uint workDim() const { return workDim_; }

    /**
     * Get global offsets
     * \return Global offsets
     */
    size_t *globalWorkOffset() const { return globalWorkOffset_; }

    /**
     * Get global work size
     * \return Global work size
     */
    size_t *globalWorkSize() const { return globalWorkSize_; }

    /**
     * Get local work size
     * \return Local work size
     */
    size_t *localWorkSize() const { return localWorkSize_; }
};

/** An OpenCL kernel that can be executed */
class GMAC_LOCAL KernelLaunch : public core::hpe::KernelLaunch, public KernelConfig, public util::NonCopyable {
    friend class Kernel;
protected:
    /** OpenCL kernel code */
    cl_kernel f_;
    /** OpenCL command queue where the kernel is executed */
    cl_command_queue stream_;
    /** OpenCL event defining when the kernel execution completes */
    cl_event lastEvent_;

    /**
     * Default constructor
     * \param mode Execution mode executing the kernel
     * \param stream OpenCL command queue executing the kernel
     */
    KernelLaunch(Mode &mode, const Kernel & k, cl_command_queue stream);
public:
    /**
     * Default destructor
     */
    ~KernelLaunch();

    /**
     * Execute the kernel
     * \return Error code
     */
    gmacError_t execute();

    /**
     * Get the OpenCL event that defines when the kernel execution is complete
     * \return OpenCL event which is completed after the kernel execution is done
     */
    cl_event getCLEvent();
};

}}}

#include "Kernel-impl.h"

#endif
