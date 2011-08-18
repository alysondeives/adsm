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


#ifndef GMAC_INCLUDE_TYPES_H_
#define GMAC_INCLUDE_TYPES_H_

#if defined(__GNUC__)
#	include <pthread.h>
#	include <sys/types.h>
	typedef pthread_t THREAD_T;
	typedef pid_t     PROCESS_T;
#	define FMT_SIZE	   "%zd"
#	if defined(__LP64__) 
#		define FMT_TID "0x%lx"
#	else
#		if defined(DARWIN)
#			define FMT_TID "%p"
#		else
#			define FMT_TID "0x%lx"
#		endif
#	endif
#elif defined(_MSC_VER)
#	include <windows.h>
	typedef DWORD THREAD_T;
	typedef DWORD PROCESS_T;
#	define FMT_TID "0x%lx"
#	define FMT_SIZE "%Id"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	gmacSuccess = 0,
	gmacErrorMemoryAllocation,
	gmacErrorLaunchFailure,
	gmacErrorNotReady,
	gmacErrorNoAccelerator,
	gmacErrorInvalidValue,
	gmacErrorInvalidAccelerator,
	gmacErrorInvalidAcceleratorFunction,
	gmacErrorInvalidSize,
    gmacErrorAlreadyBound,
	gmacErrorApiFailureBase,
    gmacErrorFeatureNotSupported,
    gmacErrorInsufficientAcceleratorMemory,
    gmacErrorInvalidKernelName,
	gmacErrorUnknown
} gmacError_t;


enum GmacGlobalMallocType {
    GMAC_GLOBAL_MALLOC_REPLICATED  = 0,
    GMAC_GLOBAL_MALLOC_CENTRALIZED = 1
};

enum GmacProtection {
    GMAC_PROT_NONE = 0,
    GMAC_PROT_READ,
    GMAC_PROT_WRITE,
    GMAC_PROT_READWRITE
};

enum GmacMapFlags {
    GMAC_MAP_READ  = 0x1,
    GMAC_MAP_WRITE = 0x2
};

enum GmacRemapFlags {
    GMAC_REMAP_READ  = 0x1,
    GMAC_REMAP_WRITE = 0x2,
    GMAC_REMAP_FIXED = 0x4
};

static const int GMAC_MAP_DEFAULT = GMAC_MAP_READ | GMAC_MAP_WRITE;

#ifdef __cplusplus
};
#endif

#endif /* GMAC_ERROR_H */
