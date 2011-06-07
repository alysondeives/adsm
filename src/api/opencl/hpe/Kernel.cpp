#include "Kernel.h"
#include "Mode.h"
#include "Accelerator.h"

#include "trace/Tracer.h"

namespace __impl { namespace opencl { namespace hpe {

gmacError_t
KernelLaunch::execute()
{
    trace_.init(mode_.id());
    gmacError_t ret = dynamic_cast<Mode &>(mode_).getAccelerator().execute(stream_, f_, workDim_,
        globalWorkOffset_, globalWorkSize_, localWorkSize_, event_);
    trace_.trace(f_, event_);
    return ret;
}

}}}
