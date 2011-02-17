#ifdef USE_DBC

#include "core/IOBuffer.h"

namespace __dbc { namespace core {


IOBuffer::IOBuffer(void *addr, size_t size, bool async) :
    __impl::core::IOBuffer(addr, size, async)
{
    // This check goes out because OpenCL will always use 0 as base address
    // REQUIRES(addr != NULL);
    REQUIRES(size > 0);
}

IOBuffer::~IOBuffer()
{
}

}}
#endif 

