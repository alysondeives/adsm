#include "MemManager.h"
#include "BatchManager.h"
#include "LazyManager.h"
#include "RollingManager.h"

#include <debug.h>

#include <strings.h>


namespace gmac {

MemManager *getManager(const char *managerName)
{
	if(managerName == NULL) return new RollingManager();
	TRACE("Using %s Manager", managerName);
	if(strcasecmp(managerName, "None") == 0)
		return NULL;
	else if(strcasecmp(managerName, "Lazy") == 0)
		return new LazyManager();
	else if(strcasecmp(managerName, "Batch") == 0)
		return new BatchManager();
	return new RollingManager();
}

void MemManager::insertVirtual(void *cpuPtr, void *devPtr, size_t count)
{
	uint8_t *cpuAddr = (uint8_t *)cpuPtr;
	uint8_t *devAddr = (uint8_t *)devPtr;
	count += ((unsigned long)cpuPtr & (pageSize -1));
	enterLock(pageTable);
	MUTEX_LOCK(mutex);
	exitLock();
	for(size_t off = 0; off < count; off += pageSize)
		virtTable[cpuAddr + off] = devAddr + off;
	MUTEX_UNLOCK(mutex);
}

};
