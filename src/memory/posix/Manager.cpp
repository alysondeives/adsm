#include <memory/Manager.h>
#include <debug.h>

namespace gmac { namespace memory {

#ifndef HAVE_POSIX_MEMALIGN
static void *custom_memalign(void **addr, size_t align, size_t count)
{
}
#endif

void *Manager::hostMap(void *addr, size_t count, int prot)
{
	void *cpuAddr = NULL;
#ifndef USE_MMAP
#ifdef HAVE_POSIX_MEMALIGN
	if(posix_memalign(&cpuAddr, pageTable().getPageSize(), count) != 0)
		return NULL;
#else
#endif
	Memory::protect(cpuAddr, count, prot);
#else
	cpuAddr = (void *)((uint8_t *)addr + Context::current()->id() * mmSize);
	if(mmap(cpuAddr, count, prot, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0) != cpuAddr)
		return NULL;
#endif
	return cpuAddr;
}


void Manager::hostUnmap(void *addr, size_t count)
{
#ifdef USE_GLOBAL_HOST
	if(proc->isShared(addr) == true) return;
#endif

#ifndef USE_MMAP
	free(addr);
#else
	munmap(addr, count);
#endif
}

} };
