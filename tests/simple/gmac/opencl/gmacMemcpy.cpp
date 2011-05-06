#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <gmac/opencl.h>

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

enum MemcpyType {
    GMAC_TO_GMAC = 1,
    HOST_TO_GMAC = 2,
    GMAC_TO_HOST = 3,
};

int memcpyTest(MemcpyType type, bool callKernel, void *(*memcpy_fn)(void *, const void *, size_t n))
{
    int error = 0;

    ocl_kernel kernel;
    size_t globalSize = 1;
    size_t localSize = 1;

    assert(__oclKernelGet("null", &kernel) == oclSuccess);
    assert(__oclKernelConfigure(&kernel, 1, NULL, &globalSize, &localSize) == oclSuccess);


    for (size_t count = minCount; count <= maxCount; count *= 2) {
        fprintf(stderr, "ALLOC: %zd\n", count * sizeof(long));
        long *baseSrc = (long *)malloc(count * sizeof(long));
        long *baseDst = (long *)malloc(count * sizeof(long));

        long *gmacSrc;
        long *gmacDst;

        if (type == GMAC_TO_GMAC) {
            assert(gmacMalloc((void **)&gmacSrc, count * sizeof(long)) == oclSuccess);
            assert(gmacMalloc((void **)&gmacDst, count * sizeof(long)) == oclSuccess);
        } else if (type == HOST_TO_GMAC) {
            gmacSrc = (long *)malloc(count * sizeof(long));
            assert(gmacMalloc((void **)&gmacDst, count * sizeof(long)) == oclSuccess);
        } else if (type == GMAC_TO_HOST) {
            assert(gmacMalloc((void **)&gmacSrc, count * sizeof(long)) == oclSuccess);
            gmacDst = (long *)malloc(count * sizeof(long));
        }

        for (size_t stride = 0, i = 1; stride < count/3; stride = i, i *= 2) {
            for (size_t copyCount = 0, j = 1; copyCount < count/3; copyCount = j, j *= 2) {
                init(baseSrc, int(count), 1);
                init(baseDst, int(count), 0);

                init(gmacSrc, int(count), 1);
                init(gmacDst, int(count), 0);
                assert(stride + copyCount <= count);

                if (callKernel) {
                    assert(__oclKernelLaunch(&kernel) == oclSuccess);
                }
                memcpy   (baseDst + stride, baseSrc + stride, copyCount * sizeof(long));
                memcpy_fn(gmacDst + stride, gmacSrc + stride, copyCount * sizeof(long));

                int ret = memcmp(gmacDst, baseDst, count * sizeof(long));

                if (ret != 0) {
#if 0
                    fprintf(stderr, "Error: gmacToGmacTest size: %zd, stride: %zd, copy: %zd\n",
                            count     * sizeof(long),
                            stride    * sizeof(long),
                            copyCount * sizeof(long));
#endif
                    error = 1;
                    goto exit_test;
                }
#if 0
                for (unsigned k = 0; k < count; k++) {
                    int ret = baseDst[k] != gmacDst[k];
                    if (ret != 0) {
                        fprintf(stderr, "Error: gmacToGmacTest size: %zd, stride: %zd, copy: %zd. Pos %u\n", count     * sizeof(long),
                                stride    * sizeof(long),
                                copyCount * sizeof(long), k);
                        error = 1;
                    }
                }
#endif
            }
        }

        if (type == GMAC_TO_GMAC) {
            assert(gmacFree(gmacSrc) == oclSuccess);
            assert(gmacFree(gmacDst) == oclSuccess);
        } else if (type == HOST_TO_GMAC) {
            free(gmacSrc);
            assert(gmacFree(gmacDst) == oclSuccess);
        } else if (type == GMAC_TO_HOST) {
            assert(gmacFree(gmacSrc) == oclSuccess);
            free(gmacDst);
        }

        free(baseSrc);
        free(baseDst);
    }

exit_test:
    return error;
}

static void *oclMemcpyWrapper(void *dst, const void *src, size_t size)
{
	return oclMemcpy(dst, src, size);
}

int main(int argc, char *argv[])
{
    assert(__oclPrepareCLCode(kernel) == oclSuccess);

    int           ret = memcpyTest(GMAC_TO_GMAC, false, oclMemcpyWrapper);
    if (ret == 0) ret = memcpyTest(GMAC_TO_GMAC, true, oclMemcpyWrapper);
    if (ret == 0) ret = memcpyTest(GMAC_TO_GMAC, false, memcpy);
    if (ret == 0) ret = memcpyTest(GMAC_TO_GMAC, true, memcpy);

    if (ret == 0) ret = memcpyTest(HOST_TO_GMAC, false, oclMemcpyWrapper);
    if (ret == 0) ret = memcpyTest(HOST_TO_GMAC, true, oclMemcpyWrapper);
    if (ret == 0) ret = memcpyTest(HOST_TO_GMAC, false, memcpy);
    if (ret == 0) ret = memcpyTest(HOST_TO_GMAC, true, memcpy);

    if (ret == 0) ret = memcpyTest(GMAC_TO_HOST, false, oclMemcpyWrapper);
    if (ret == 0) ret = memcpyTest(GMAC_TO_HOST, true, oclMemcpyWrapper);
    if (ret == 0) ret = memcpyTest(GMAC_TO_HOST, false, memcpy);
    if (ret == 0) ret = memcpyTest(GMAC_TO_HOST, true, memcpy);

    return ret;
}
