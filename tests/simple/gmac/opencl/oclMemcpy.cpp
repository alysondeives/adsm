#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <gmac/opencl.h>

#include "utils.h"

enum MemcpyType {
    GMAC_TO_GMAC = 1,
    HOST_TO_GMAC = 2,
    GMAC_TO_HOST = 3,
};

int type;
int typeDefault = GMAC_TO_GMAC;
const char *typeStr = "GMAC_MEMCPY_TYPE";

bool memcpyFn;
bool memcpyFnDefault = false;
const char *memcpyFnStr = "GMAC_MEMCPY_GMAC";

const size_t minCount = 1024;
const size_t maxCount = 2 * 1024 * 1024;

const char *kernel = "\
__kernel void null()\
{\
	return;\
}\
";

void init(long *ptr, int s, long v)
{
	for(int i = 0; i < s; i++) {
		ptr[i] = v;
	}
}

int memcpyTest(MemcpyType type, bool callKernel, void *(*memcpy_fn)(void *, const void *, size_t n))
{
    int error = 0;

    ocl_kernel kernel;
    size_t globalSize = 1;
    size_t localSize = 1;

    assert(oclGetKernel("null", &kernel) == oclSuccess);

    long *baseSrc, *baseDst;
    long *oclSrc, *oclDst;


    for (size_t count = minCount; count <= maxCount; count *= 2) {
        fprintf(stderr, "ALLOC: "FMT_SIZE"\n", count * sizeof(long));
        baseSrc = (long *)malloc(count * sizeof(long));
        baseDst = (long *)malloc(count * sizeof(long));

        if (type == GMAC_TO_GMAC) {
            assert(oclMalloc((void **)&oclSrc, count * sizeof(long)) == oclSuccess);
            assert(oclMalloc((void **)&oclDst, count * sizeof(long)) == oclSuccess);
        } else if (type == HOST_TO_GMAC) {
            oclSrc = (long *)malloc(count * sizeof(long));
            assert(oclMalloc((void **)&oclDst, count * sizeof(long)) == oclSuccess);
        } else if (type == GMAC_TO_HOST) {
            assert(oclMalloc((void **)&oclSrc, count * sizeof(long)) == oclSuccess);
            oclDst = (long *)malloc(count * sizeof(long));
        }

        for (size_t stride = 0, i = 1; stride < count/3; stride = i, i *= 2) {
            for (size_t copyCount = 0, j = 1; copyCount < count/3; copyCount = j, j *= 2) {
                init(baseSrc, int(count), 1);
                init(baseDst, int(count), 0);

                init(oclSrc, int(count), 1);
                init(oclDst, int(count), 0);
                assert(stride + copyCount <= count);

                if (callKernel) {
                    assert(oclCallNDRange(kernel, 1, NULL, &globalSize, &localSize) == oclSuccess);
                }
                memcpy   (baseDst + stride, baseSrc + stride, copyCount * sizeof(long));
                memcpy_fn(oclDst + stride, oclSrc + stride, copyCount * sizeof(long));

                int ret = memcmp(oclDst, baseDst, count * sizeof(long));

                if (ret != 0) {
#if 0
                    fprintf(stderr, "Error: oclToGmacTest size: %zd, stride: %zd, copy: %zd\n",
                            count     * sizeof(long),
                            stride    * sizeof(long),
                            copyCount * sizeof(long));
#endif
                    error = 1;
                    goto exit_test;
                }
#if 0
                for (unsigned k = 0; k < count; k++) {
                    int ret = baseDst[k] != oclDst[k];
                    if (ret != 0) {
                        fprintf(stderr, "Error: oclToGmacTest size: %zd, stride: %zd, copy: %zd. Pos %u\n", count     * sizeof(long),
                                stride    * sizeof(long),
                                copyCount * sizeof(long), k);
                        error = 1;
                    }
                }
#endif
            }
        }

        if (type == GMAC_TO_GMAC) {
            assert(oclFree(oclSrc) == oclSuccess);
            assert(oclFree(oclDst) == oclSuccess);
        } else if (type == HOST_TO_GMAC) {
            free(oclSrc);
            assert(oclFree(oclDst) == oclSuccess);
        } else if (type == GMAC_TO_HOST) {
            assert(oclFree(oclSrc) == oclSuccess);
            free(oclDst);
        }

        free(baseSrc);
        free(baseDst);
    }

    oclReleaseKernel(kernel);

    return error;

exit_test:

    if (type == GMAC_TO_GMAC) {
        assert(oclFree(oclSrc) == oclSuccess);
        assert(oclFree(oclDst) == oclSuccess);
    } else if (type == HOST_TO_GMAC) {
        free(oclSrc);
        assert(oclFree(oclDst) == oclSuccess);
    } else if (type == GMAC_TO_HOST) {
        assert(oclFree(oclSrc) == oclSuccess);
        free(oclDst);
    }

    free(baseSrc);
    free(baseDst);

    return error;
}

static void *oclMemcpyWrapper(void *dst, const void *src, size_t size)
{
	return oclMemcpy(dst, src, size);
}

int main(int argc, char *argv[])
{
	setParam<int>(&type, typeStr, typeDefault);
	setParam<bool>(&memcpyFn, memcpyFnStr, memcpyFnDefault);

    assert(oclCompileSource(kernel) == oclSuccess);

    int ret;
    
    if (memcpyFn == true) {
        ret = memcpyTest(MemcpyType(type), false, oclMemcpyWrapper);
        if (ret == 0) ret = memcpyTest(MemcpyType(type), true, oclMemcpyWrapper);
    } else {
        ret = memcpyTest(MemcpyType(type), false, memcpy);
        if (ret == 0) ret = memcpyTest(MemcpyType(type), true, memcpy);
    }

    return ret;
}