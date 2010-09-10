#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>

#include <gmac.h>

#include "utils.h"
#include "debug.h"
#include "barrier.h"

#include "gmacStencilCommon.cu"


int main(int argc, char *argv[])
{
	setParam<size_t>(&dimRealElems, dimRealElemsStr, dimRealElemsDefault);

    if (dimRealElems % 32 != 0) {
        fprintf(stderr, "Error: wrong dimension %d\n", dimRealElems);
        abort();
    }

    dimElems = dimRealElems + 2 * STENCIL;

    JobDescriptor * descriptor = new JobDescriptor();
    descriptor->gpus  = 1;
    descriptor->gpuId = 1;

    descriptor->prev = NULL;
    descriptor->next = NULL;

    descriptor->dimRealElems = dimRealElems;
    descriptor->dimElems     = dimElems;
    descriptor->slices       = dimRealElems;

    do_stencil((void *) descriptor);

    delete descriptor;

    return 0;
}
