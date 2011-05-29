#include <cstdio>
#include <sys/stat.h>

#include <vector>

#include <CL/cl.h>

#include <config/common.h>
#include <include/gmac/cl.h>

static std::vector<cl_helper> helpers;

static cl_int clHelperInitPlatform(cl_platform_id platform, cl_helper &state)
{
    cl_int error_code;
    cl_uint i, num_devices = 0, num_contexts = 0, num_queues = 0;

    state.platform = platform;

    /* Get the devices */
    error_code = clGetDeviceIDs(state.platform, CL_DEVICE_TYPE_GPU, 0, NULL, &num_devices);
    if(error_code != CL_SUCCESS) return error_code;
    state.devices = NULL;
    state.devices = (cl_device_id *)malloc(num_devices * sizeof(cl_device_id));
    if(state.devices == NULL) return CL_OUT_OF_HOST_MEMORY;
    error_code = clGetDeviceIDs(state.platform, CL_DEVICE_TYPE_GPU, num_devices, state.devices, NULL);
    if(error_code != CL_SUCCESS) return error_code;

    /* Create contexts */
    state.contexts = NULL;
    state.contexts = (cl_context *)malloc(num_devices * sizeof(cl_context));
    if(state.contexts == NULL) return CL_OUT_OF_HOST_MEMORY;
    for(num_contexts = 0; num_contexts < num_devices; num_contexts++) {
        state.contexts[num_contexts] = clCreateContext(NULL, 1, &state.devices[num_contexts], NULL, NULL, &error_code);
        if(error_code != CL_SUCCESS) goto cleanup_contexts;
    }

    /* Create command queues */
    state.command_queues = NULL;
    state.command_queues = (cl_command_queue *)malloc(num_devices * sizeof(cl_command_queue));
    if(state.command_queues == NULL) {
        error_code = CL_OUT_OF_HOST_MEMORY;
        goto cleanup_contexts;
    }
    for(num_queues = 0; num_queues < num_devices; num_queues++) {
        state.command_queues[num_queues] = clCreateCommandQueue(state.contexts[num_queues], state.devices[num_queues], 0, &error_code);
        if(error_code != CL_SUCCESS) goto cleanup_queues;
    }

    state.num_devices = num_devices;
    return CL_SUCCESS;

cleanup_queues:
    for(i = 0; i < num_queues; i++) clReleaseCommandQueue(state.command_queues[i]);
    free(state.command_queues);

cleanup_contexts:
    for(i = 0; i < num_contexts; i++) clReleaseContext(state.contexts[i]);
    free(state.contexts);
    
    return error_code;
}

GMAC_API cl_int clInitHelpers(size_t *platforms)
{
    cl_int error_code;
    cl_uint num_platforms = 0;

    error_code = clGetPlatformIDs(0, NULL, &num_platforms);
    if(error_code != CL_SUCCESS) return error_code;

    *platforms = size_t(num_platforms);
    cl_platform_id *tmp_platforms = new cl_platform_id[num_platforms];

    /* Get the platoforms */
    error_code = clGetPlatformIDs(num_platforms, tmp_platforms, NULL);
    if(error_code != CL_SUCCESS) goto cleanup;

    for (size_t i = 0; i < num_platforms; i++) {
        cl_helper helper;
        helper.platform = 0;
        helper.num_devices = 0;
        helper.devices = NULL;
        helper.contexts = NULL;
        helper.command_queues = NULL;
        error_code = clHelperInitPlatform(tmp_platforms[i], helper);
        if (error_code != CL_SUCCESS) goto cleanup;
        helpers.push_back(helper);
    }

cleanup:
    delete [] tmp_platforms;

    return error_code;
}

GMAC_API cl_helper *clGetHelpers()
{
    cl_helper *ret = new cl_helper[helpers.size()];
    for (size_t i = 0; i < helpers.size(); i++) {
        ret[i] = helpers[i];
    }
    return ret;
}

GMAC_API cl_int clReleaseHelpers()
{
    cl_int error_code = CL_SUCCESS;
    for (size_t i = 0; i < helpers.size(); i++) {
        cl_helper &helper = helpers[i];
        cl_uint i;
        for(i = 0; i < helper.num_devices; i++) {
            error_code = clReleaseCommandQueue(helper.command_queues[i]);
            if(error_code != CL_SUCCESS) return error_code;
            error_code = clReleaseContext(helper.contexts[i]);
            if(error_code != CL_SUCCESS) return error_code;
        }

        free(helper.command_queues);
        free(helper.contexts);
        free(helper.devices);
    }

    helpers.clear();
    
    return CL_SUCCESS;
}


static const char *build_flags = "-I.";

GMAC_API cl_program clHelperLoadProgramFromFile(cl_helper state, const char *file_name, cl_int *error_code)
{
    /* Let's all thank Microsoft for such a great compatibility */
#if defined(_MSC_VER)
#   define stat _stat
#endif

    cl_program ret = NULL;
    FILE *fp;
    struct stat file_stats;
    char *buffer = NULL;
    size_t read_bytes;
    cl_uint i = 0;

    if(stat(file_name, &file_stats) < 0) { *error_code = CL_INVALID_VALUE; return ret; }
#if defined(_MSC_VER)
#   undef stat
#endif

    buffer = (char *)malloc(file_stats.st_size * sizeof(char));
    if(buffer == NULL) { *error_code = CL_OUT_OF_HOST_MEMORY; return ret; }

#if defined(_MSC_VER)
	if(fopen_s(&fp, file_name, "rt") != 0) { *error_code = CL_INVALID_VALUE; goto cleanup; }
#else
    fp = fopen(file_name, "rt");
    if(fp == NULL) { *error_code = CL_INVALID_VALUE; goto cleanup; }
#endif
    read_bytes = fread(buffer, file_stats.st_size, sizeof(char), fp);
    fclose(fp);
    if(read_bytes != (size_t)file_stats.st_size) {
        *error_code = CL_INVALID_VALUE;
        goto cleanup;
    }

    for(i = 0; i < state.num_devices; i++) {
        ret = clCreateProgramWithSource(state.contexts[i], 1, (const char **)&buffer, &read_bytes, error_code);
        if(*error_code != CL_SUCCESS) goto cleanup;
    }

    *error_code = clBuildProgram(ret, state.num_devices, state.devices, build_flags, NULL, NULL);

cleanup:
    free(buffer);

    return ret;
}
