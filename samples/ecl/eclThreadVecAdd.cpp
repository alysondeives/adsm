#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <gmac/opencl.h>

#include "utils.h"

const unsigned nIter = 2;
const unsigned vecSize = 32 * 1024 * 1024;

static float **s;

const char *kernel = "\
__kernel void vecAdd(__global float *c, __global const float *a, __global const float *b)\
{\
    unsigned i = get_global_id(0);\
\
    c[i] = a[i] + b[i];\
}\
";

void *addVector(void *ptr)
{
	float *a, *b;
	float **c = (float **)ptr;
	ecl_error ret = eclSuccess;

	// Alloc input data
	ret = eclMalloc((void **)&a, vecSize * sizeof(float));
	assert(ret == eclSuccess);
	ret = eclMalloc((void **)&b, vecSize * sizeof(float));
	assert(ret == eclSuccess);

    for(unsigned i = 0; i < vecSize; i++) {
        a[i] = 1.f * rand() / RAND_MAX;
        b[i] = 1.f * rand() / RAND_MAX;
    }

	// Alloc output data
	ret = eclMalloc((void **)c, vecSize * sizeof(float));
	assert(ret == eclSuccess);

	// Call the kernel
    ecl_kernel kernel;

    assert(eclGetKernel("vecAdd", &kernel) == eclSuccess);
    cl_mem d_c = cl_mem(eclPtr(*c));
    assert(eclSetKernelArg(kernel, 0, sizeof(cl_mem), &d_c) == eclSuccess);
    cl_mem d_a = cl_mem(eclPtr(a));                        
    assert(eclSetKernelArg(kernel, 1, sizeof(cl_mem), &d_a) == eclSuccess);
    cl_mem d_b = cl_mem(eclPtr(b));                        
    assert(eclSetKernelArg(kernel, 2, sizeof(cl_mem), &d_b) == eclSuccess);
    assert(eclCallNDRange(kernel, 1, NULL, &vecSize, NULL) == eclSuccess);

    // Check the result in the CPU
	float error = 0;
	for(unsigned i = 0; i < vecSize; i++) {
		error += (*c)[i] - (a[i] + b[i]);
	}
	fprintf(stdout, "Error: %.02f\n", error);

    eclReleaseKernel(kernel);

	eclFree(a);
	eclFree(b);
	eclFree(*c);

	return NULL;
}

int main(int argc, char *argv[])
{
	thread_t *nThread;
	unsigned n = 0;

    assert(eclCompileSource(kernel) == eclSuccess);

	vecSize = vecSize / nIter;
	if(vecSize % nIter) vecSize++;

	nThread = (thread_t *)malloc(nIter * sizeof(thread_t));
	s = (float **)malloc(nIter * sizeof(float **));

	for(n = 0; n < nIter; n++) {
		nThread[n] = thread_create(addVector, &s[n]);
	}

	for(n = 0; n < nIter; n++) {
		thread_wait(nThread[n]);
	}

	free(s);
	free(nThread);

    return 0;
}
