#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

#include <pthread.h>
#include <semaphore.h>

#include <gmac/opencl>

#include "utils.h"
#include "debug.h"

#include "eclCompressCommon.cl"

const char *widthStr = "GMAC_WIDTH";
const char *heightStr = "GMAC_HEIGHT";
const char *framesStr = "GMAC_FRAMES";

const unsigned widthDefault = 128;
const unsigned heightDefault = 128;
const unsigned framesDefault = 32;

unsigned width = 0;
unsigned height = 0;
unsigned frames = 0;
const unsigned blockSize = 16;

static float *quant_in, *idct_in;

static pthread_t dct_id, quant_id, idct_id;
static gmac_sem_t quant_data, idct_data;
static gmac_sem_t quant_free, idct_free;

double timeDCTAlloc = 0.0;
double timeDCTInit  = 0.0;
double timeDCTCopy  = 0.0;
double timeDCTRun   = 0.0;
double timeDCTFree  = 0.0;

double timeQuantAlloc = 0.0;
double timeQuantCopy  = 0.0;
double timeQuantRun   = 0.0;
double timeQuantFree  = 0.0;

double timeIDCTAlloc = 0.0;
double timeIDCTRun   = 0.0;
double timeIDCTFree  = 0.0;

void __randInit(float *a, unsigned size)
{
	for(unsigned i = 0; i < size; i++) {
		a[i] = 10.0 * rand() / RAND_MAX;
	}
}

void *dct_thread(void *args)
{
	float *in, *out;
	gmacError_t ret;
    gmactime_t s, t;

    getTime(&s);
	ret = eclMalloc((void **)&in, width * height * sizeof(float));
	assert(ret == gmacSuccess);
	ret = eclMalloc((void **)&out, width * height * sizeof(float));
	assert(ret == gmacSuccess);
    getTime(&t);
    timeDCTAlloc += getTimeStamp(t) - getTimeStamp(s);

    size_t localSize[2];
    size_t globalSize[2];
    localSize[0] = blockSize;
    localSize[1] = blockSize;
    globalSize[0] = width;
    globalSize[1] = height;
	if(width  % blockSize) globalSize[0] += blockSize;
	if(height % blockSize) globalSize[1] += blockSize;
    ecl::error err;
    ecl::kernel k("dct", err);
    assert(err == eclSuccess);

    assert(k.setArg(0, out)    == eclSuccess);
    assert(k.setArg(1, in)     == eclSuccess);
    assert(k.setArg(2, width)  == eclSuccess);
    assert(k.setArg(3, height) == eclSuccess);

	for(unsigned i = 0; i < frames; i++) {
        getTime(&s);
		__randInit(in, width * height);
        getTime(&t);
        timeDCTInit += getTimeStamp(t) - getTimeStamp(s);

        getTime(&s);
        assert(k.callNDRange(2, NULL, globalSize, localSize) == eclSuccess);
        getTime(&t);
        timeDCTRun += getTimeStamp(t) - getTimeStamp(s);

        getTime(&s);
		gmac_sem_wait(&quant_free, 1); /* Wait for quant to use its data */
		eclMemcpy(quant_in, out, width * height * sizeof(float));
		gmac_sem_post(&quant_data, 1); /* Notify to Quant that data is ready */
        getTime(&t);
        timeDCTCopy += getTimeStamp(t) - getTimeStamp(s);
	}

    getTime(&s);
	eclFree(in);
	eclFree(out);
    getTime(&t);
    timeDCTFree += getTimeStamp(t) - getTimeStamp(s);

	return NULL;
}

void *quant_thread(void *args)
{
	float *out;
	gmacError_t ret;
    gmactime_t s, t;

    getTime(&s);
	ret = eclMalloc((void **)&quant_in, width * height * sizeof(float));
	assert(ret == gmacSuccess);
	ret = eclMalloc((void **)&out, width * height * sizeof(float));
	assert(ret == gmacSuccess);
    getTime(&t);
    timeQuantAlloc += getTimeStamp(t) - getTimeStamp(s);

    size_t localSize[2];
    size_t globalSize[2];
    localSize[0] = blockSize;
    localSize[1] = blockSize;
    globalSize[0] = width;
    globalSize[1] = height;
	if(width  % blockSize) globalSize[0] += blockSize;
	if(height % blockSize) globalSize[1] += blockSize;
    ecl::error err;
    ecl::kernel k("quant", err);
    assert(err == eclSuccess);

    assert(k.setArg(0, quant_in)    == eclSuccess);
    assert(k.setArg(1, out)         == eclSuccess);
    assert(k.setArg(2, width)       == eclSuccess);
    assert(k.setArg(3, height)      == eclSuccess);
    assert(k.setArg(4, float(1e-6)) == eclSuccess);

	gmac_sem_post(&quant_free, 1);

	for(unsigned i = 0; i < frames; i++) {
        getTime(&s);
		gmac_sem_wait(&quant_data, 1);	/* Wait for data to be processed */
        assert(k.callNDRange(2, NULL, globalSize, localSize) == eclSuccess);
        getTime(&t);
        timeQuantRun += getTimeStamp(t) - getTimeStamp(s);
		
        getTime(&s);
		gmac_sem_wait(&idct_free, 1); /* Wait for IDCT to use its data */
		eclMemcpy(idct_in, out, width * height * sizeof(float));
		gmac_sem_post(&quant_free, 1); /* Notify to DCT that Quant is waiting for data */
		gmac_sem_post(&idct_data, 1); /* Nodify to IDCT that data is ready */
        getTime(&t);
        timeQuantCopy += getTimeStamp(t) - getTimeStamp(s);
	}

    getTime(&s);
	eclFree(quant_in);
	eclFree(out);
    getTime(&t);
    timeQuantFree += getTimeStamp(t) - getTimeStamp(s);

	return NULL;
}

void *idct_thread(void *args)
{
	float *out;
	gmacError_t ret;
    gmactime_t s, t;

    getTime(&s);
	ret = eclMalloc((void **)&idct_in, width * height * sizeof(float));
	assert(ret == gmacSuccess);
	ret = eclMalloc((void **)&out, width * height * sizeof(float));
	assert(ret == gmacSuccess);
    getTime(&t);
    timeIDCTAlloc += getTimeStamp(t) - getTimeStamp(s);

    size_t localSize[2];
    size_t globalSize[2];
    localSize[0] = blockSize;
    localSize[1] = blockSize;
    globalSize[0] = width;
    globalSize[1] = height;
	if(width  % blockSize) globalSize[0] += blockSize;
	if(height % blockSize) globalSize[1] += blockSize;
    ecl::error err;
    ecl::kernel k("idct", err);
    assert(err == eclSuccess);

    assert(k.setArg(0, idct_in) == eclSuccess);
    assert(k.setArg(1, out)     == eclSuccess);
    assert(k.setArg(2, width)   == eclSuccess);
    assert(k.setArg(3, height)  == eclSuccess);

	gmac_sem_post(&idct_free, 1);

	for(unsigned i = 0; i < frames; i++) {
        getTime(&s);
		gmac_sem_wait(&idct_data, 1);
        assert(k.callNDRange(2, NULL, globalSize, localSize) == eclSuccess);
		gmac_sem_post(&idct_free, 1);
        getTime(&t);
        timeIDCTRun += getTimeStamp(t) - getTimeStamp(s);
	}

    getTime(&s);
	eclFree(idct_in);
	eclFree(out);
    getTime(&t);
    timeIDCTFree += getTimeStamp(t) - getTimeStamp(s);

	return NULL;
}


int main(int argc, char *argv[])
{
	gmactime_t s,t;
	setParam<unsigned>(&width, widthStr, widthDefault);
	setParam<unsigned>(&height, heightStr, heightDefault);
	setParam<unsigned>(&frames, framesStr, framesDefault);

    assert(eclCompileSource(kernel_code) == eclSuccess);

	gmac_sem_init(&quant_data, 0); 
	gmac_sem_init(&quant_free, 0); 
	gmac_sem_init(&idct_data,  0); 
	gmac_sem_init(&idct_free,  0); 

	srand(time(NULL));

	getTime(&s);

	pthread_create(&dct_id, NULL, dct_thread, NULL);
	pthread_create(&quant_id, NULL, quant_thread, NULL);
	pthread_create(&idct_id, NULL, idct_thread, NULL);

	pthread_join(dct_id, NULL);
	pthread_join(quant_id, NULL);
	pthread_join(idct_id, NULL);

	getTime(&t);

	fprintf(stdout, "DCT:Alloc: %f\n", timeDCTAlloc);
	fprintf(stdout, "DCT:Init: %f\n", timeDCTInit);
	fprintf(stdout, "DCT:Run: %f\n", timeDCTRun);
	fprintf(stdout, "DCT:Copy: %f\n", timeDCTCopy);
	fprintf(stdout, "DCT:Free: %f\n", timeDCTFree);

	fprintf(stdout, "Quant:Alloc: %f\n", timeQuantAlloc);
	fprintf(stdout, "Quant:Run: %f\n", timeQuantRun);
	fprintf(stdout, "Quant:Copy: %f\n", timeQuantCopy);
	fprintf(stdout, "Quant:Free: %f\n", timeQuantFree);

	fprintf(stdout, "IDCT:Alloc: %f\n", timeIDCTAlloc);
	fprintf(stdout, "IDCT:Run: %f\n", timeIDCTRun);
	fprintf(stdout, "IDCT:Free: %f\n", timeIDCTFree);

	printTime(&s, &t, "Total: ", "\n");

    return 0;
}
