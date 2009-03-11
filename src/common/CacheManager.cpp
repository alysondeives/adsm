#include "CacheManager.h"
#include "debug.h"

#include <unistd.h>
#include <cuda_runtime.h>

#include <algorithm>

namespace gmac {
CacheRegion::CacheRegion(MemHandler &memHandler, void *addr, size_t size,
		size_t cacheLine) :
	MemRegion(addr, size),
	memHandler(memHandler),
	cacheLine(cacheLine)
{
	for(size_t s = 0; s < size; s += cacheLine) {
		void *p = (void *)((uint8_t *)addr + s);
		set.push_back(new ProtRegion(memHandler, p, cacheLine));
	}
}

CacheRegion::~CacheRegion()
{
	Set::const_iterator i;
	for(i = set.begin(); i != set.end(); i++)
		delete (*i);
	set.clear();
}

void CacheRegion::invalidate()
{
	Set::const_iterator i;
	for(i = set.begin(); i != set.end(); i++) {
		(*i)->clear();
		(*i)->noAccess();
	}
}

void CacheManager::writeBack(pthread_t tid)
{
	TRACE("Write Back");
	ProtRegion *r = cache[tid].front();
	cache[tid].pop_front();
	cudaMemcpy(safe(r->getAddress()), r->getAddress(), r->getSize(),
			cudaMemcpyHostToDevice);
	r->clear();
	r->noAccess();
}

void CacheManager::flushToDevice(pthread_t tid) 
{
	Cache::iterator i;
	for(i = cache[tid].begin(); i != cache[tid].end(); i++) {
		TRACE("DMA to Device from %p (%d bytes)", (*i)->getAddress(),
				(*i)->getSize());
		cudaMemcpy(safe((*i)->getAddress()), (*i)->getAddress(),
			(*i)->getSize(), cudaMemcpyHostToDevice);
		(*i)->clear();
		(*i)->noAccess();
	}
	cache[tid].clear();
}

CacheManager::CacheManager() :
	MemManager(),
	pageSize(getpagesize())
{
	MUTEX_INIT(memMutex);
}

bool CacheManager::alloc(void *addr, size_t size)
{
	if(map(addr, size, PROT_NONE) == MAP_FAILED) return false;
	MUTEX_LOCK(memMutex);
	memMap[addr] = new CacheRegion(*this, addr, size, lineSize * pageSize);
	MUTEX_UNLOCK(memMutex);
	return true;
}

void *CacheManager::safeAlloc(void *addr, size_t size)
{
	void *cpuAddr = NULL;
	if((cpuAddr = safeMap(addr, size, PROT_NONE)) == MAP_FAILED) return NULL;
	MUTEX_LOCK(memMutex);
	memMap[cpuAddr] = new CacheRegion(*this, cpuAddr, size, lineSize * pageSize);
	MUTEX_UNLOCK(memMutex);
	return cpuAddr;
}

void CacheManager::release(void *addr)
{
	void *cpuAddr = safe(addr);
	MUTEX_LOCK(memMutex);
	if(memMap.find(cpuAddr) != memMap.end())
		delete memMap[addr];
	MUTEX_UNLOCK(memMutex);
}

void CacheManager::execute()
{
	Map::const_iterator i;

	TRACE("Kernel execution scheduled");

	flushToDevice(gettid());

	MUTEX_LOCK(memMutex);
	for(i = memMap.begin(); i != memMap.end(); i++) {
		if(i->second->isOwner() == false) continue;
		i->second->invalidate();
	}
	MUTEX_UNLOCK(memMutex);
}

void CacheManager::sync()
{
}

void CacheManager::read(ProtRegion *region, void *addr)
{
	region->readWrite();
	cudaMemcpy(region->getAddress(), safe(region->getAddress()), region->getSize(), cudaMemcpyDeviceToHost);
	region->readOnly();
}


void CacheManager::write(ProtRegion *region, void *addr)
{
	pthread_t tid = gettid();
	if(cache[tid].size() == lruSize) writeBack(tid);
	region->setDirty();
	region->readWrite();
	cache[tid].push_back(region);
}

};
