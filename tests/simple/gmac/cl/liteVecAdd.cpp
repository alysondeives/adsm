#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cassert>

#include "gmac/cl.h"

#include "utils.h"
#include "debug.h"


const char *vecSizeStr = "GMAC_VECSIZE";
const unsigned vecSizeDefault = 32 * 1024 * 1024;
unsigned vecSize = 0;

const size_t blockSize = 32;

const char *msg = "Done!";

const char *kernel_source = "\
__kernel void vecAdd(__global float *c, __global const float *a, __global const float *b, unsigned size)\
{\
    unsigned i = get_global_id(0);\
    if(i >= size) return;\
\
    c[i] = a[i] + b[i];\
}\
";


int main(int argc, char *argv[])
{
    cl_platform_id platform;
    cl_device_id device;
    cl_int error_code;
    cl_context context;
    cl_command_queue command_queue;
    cl_program program;
    cl_kernel kernel;
	float *a, *b, *c;
	gmactime_t s, t;

	setParam<unsigned>(&vecSize, vecSizeStr, vecSizeDefault);
	fprintf(stdout, "Vector: %f\n", 1.0 * vecSize / 1024 / 1024);

    error_code = clGetPlatformIDs(1, &platform, NULL);
    assert(error_code == CL_SUCCESS);
    error_code = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    assert(error_code == CL_SUCCESS);
    context = clCreateContext(0, 1, &device, NULL, NULL, &error_code);
    assert(error_code == CL_SUCCESS);
    command_queue = clCreateCommandQueue(context, device, 0, &error_code);
    assert(error_code == CL_SUCCESS);
    program = clCreateProgramWithSource(context, 1, &kernel_source, NULL, &error_code);
    assert(error_code == CL_SUCCESS);
    error_code = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    assert(error_code == CL_SUCCESS);
    kernel = clCreateKernel(program, "vecAdd", &error_code);
    assert(error_code == CL_SUCCESS);

    getTime(&s);
    // Alloc & init input data
    assert(clMalloc(context, (void **)&a, vecSize * sizeof(float)) == CL_SUCCESS);
    assert(clMalloc(context, (void **)&b, vecSize * sizeof(float)) == CL_SUCCESS);
    // Alloc output data
    assert(clMalloc(context, (void **)&c, vecSize * sizeof(float)) == CL_SUCCESS);
    getTime(&t);
    printTime(&s, &t, "Alloc: ", "\n");


    float sum = 0.f;

    getTime(&s);
    valueInit(a, 1.f, vecSize);
    valueInit(b, 1.f, vecSize);
    getTime(&t);
    printTime(&s, &t, "Init: ", "\n");

    for(unsigned i = 0; i < vecSize; i++) {
        sum += a[i] + b[i];
    }
    

    // Call the kernel
    getTime(&s);
    size_t local_size = blockSize;
    size_t global_size = vecSize / blockSize;
    if(vecSize % blockSize) global_size++;
    global_size *= local_size;

    cl_mem c_device = clBuffer(context, c);
    assert(clSetKernelArg(kernel, 0, sizeof(cl_mem), &c_device) == CL_SUCCESS);
    cl_mem a_device = clBuffer(context, a);
    assert(clSetKernelArg(kernel, 1, sizeof(cl_mem), &a_device) == CL_SUCCESS);
    cl_mem b_device = clBuffer(context, b);
    assert(clSetKernelArg(kernel, 2, sizeof(cl_mem), &b_device) == CL_SUCCESS);
    assert(clSetKernelArg(kernel, 3, sizeof(vecSize), &vecSize) == CL_SUCCESS);

    assert(clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, &global_size, &local_size, 0, NULL, NULL) == CL_SUCCESS);
    assert(clFinish(command_queue) == CL_SUCCESS);

    getTime(&t);
    printTime(&s, &t, "Run: ", "\n");


    getTime(&s);
    float error = 0.f;
    float check = 0.f;
    for(unsigned i = 0; i < vecSize; i++) {
        error += c[i] - (a[i] + b[i]);
        check += c[i];
    }
    getTime(&t);
    printTime(&s, &t, "Check: ", "\n");
    fprintf(stderr, "Error: %f\n", error);

    if (sum != check) {
        printf("Sum: %f vs %f\n", sum, check);
        abort();
    }

    clFree(context, a);
    clFree(context, b);
    clFree(context, c);
    return 0;
}
