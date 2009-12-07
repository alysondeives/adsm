#include "PageTable.h"

#include "config/params.h"

#include <kernel/Context.h>

#include <malloc.h>

namespace gmac { namespace memory {

static const size_t defaultPageSize = 2 * 1024 * 1024;

PARAM_REGISTER(paramPageSize,
               size_t,
               defaultPageSize,
               "GMAC_PAGE",
               PARAM_NONZERO);

size_t PageTable::pageSize;
size_t PageTable::tableShift;

PageTable::PageTable() :
	_clean(false), _valid(true),
	pages(1)
{
	MUTEX_INIT(mutex);
    pageSize = paramPageSize;
	tableShift = log2(pageSize);
	TRACE("Page Size: %d bytes", pageSize);
#ifndef USE_MMAP
	TRACE("Table Shift: %d bits", tableShift);
	TRACE("Table Size: %d entries", (1 << dirShift) / pageSize);
#endif
}


PageTable::~PageTable()
{ 
#ifndef USE_MMAP
	TRACE("Cleaning Page Table");
	for(int i = 0; i < rootTable.size(); i++) {
		if(rootTable.present(i) == false) continue;
		deleteDirectory(rootTable.value(i));
	}
#endif
}

void PageTable::deleteDirectory(Directory *dir)
{
	for(int i = 0; i < dir->size(); i++) {
		if(dir->present(i) == false) continue;
		delete dir->value(i);
	}
	delete dir;
}

void PageTable::insert(void *host, void *dev)
{
#ifndef USE_MMAP
	sync();

	enterFunction(vmAlloc);
	lock();
	_clean = false;
	// Get the root table entry
	if(rootTable.present(entry(host, rootShift, rootTable.size())) == false) {
		rootTable.create(entry(host, rootShift, rootTable.size()));
		pages++;
	}
	Directory &dir = rootTable.get(entry(host, rootShift, rootTable.size()));

	if(dir.present(entry(host, dirShift, dir.size())) == false) {
		dir.create(entry(host, dirShift, dir.size()),
			(1 << dirShift) / pageSize);
		pages++;
	}
	Table &table = dir.get(entry(host, dirShift, dir.size()));

	unsigned e = entry(host, tableShift, table.size());
	assert(table.present(e) == false || (uint8_t *)table.value(e) == dev);

	table.insert(entry(host, tableShift, table.size()), dev);
	TRACE("PT inserts: %p -> %p", entry(host, tableShift, table.size()), dev);
	unlock();
	exitFunction();
#endif
}

void PageTable::remove(void *host)
{
#ifndef USE_MMAP
	sync();
	enterFunction(vmFree);
	lock();
	_clean = false;

	if(rootTable.present(entry(host, rootShift, rootTable.size())) == false) {
		exitFunction();
		return;
	}
	Directory &dir = rootTable.get(entry(host, rootShift, rootTable.size()));
	if(dir.present(entry(host, dirShift, dir.size())) == false) {
		exitFunction();
		return;
	}
	Table &table = dir.get(entry(host, dirShift, dir.size()));
	table.remove(entry(host, tableShift, table.size()));
	unlock();
	exitFunction();
#endif
}

void *PageTable::translate(void *host) 
{
#ifdef USE_MMAP
	return host;
#else
	sync();

	if(rootTable.present(entry(host, rootShift, rootTable.size())) == false)
		return NULL;
	Directory &dir = rootTable.get(entry(host, rootShift, rootTable.size()));
	if(dir.present(entry(host, dirShift, dir.size())) == false) return NULL;
	Table &table = dir.get(entry(host, dirShift, dir.size()));
	uint8_t *addr =
		(uint8_t *)table.value(entry(host, tableShift, table.size()));
	TRACE("PT pre-translate: %p -> %p", host, addr);
	addr += offset(host);
	TRACE("PT translate: %p -> %p", host, addr);
	return (void *)addr;
#endif
}

bool PageTable::dirty(void *host)
{
#ifdef USE_VM
	sync();

	assert(
		rootTable.present(entry(host, rootShift, rootTable.size())) == true);
	Directory &dir = rootTable.get(entry(host, rootShift, rootTable.size()));
	assert(dir.present(entry(host, dirShift, dir.size())) == true);
	Table &table = dir.get(entry(host, dirShift, dir.size()));
	assert(table.present(entry(host, tableShift, table.size())) == true);
	return table.dirty(entry(host, tableShift, table.size()));
#else
	return true;
#endif
}

void PageTable::clear(void *host)
{
#ifdef USE_VM
	sync();
	_clean = false;
	assert(
		rootTable.present(entry(host, rootShift, rootTable.size())) == true);
	Directory &dir = rootTable.get(entry(host, rootShift, rootTable.size()));
	assert(dir.present(entry(host, dirShift, dir.size())) == true);
	Table &table = dir.get(entry(host, dirShift, dir.size()));
	assert(table.present(entry(host, tableShift, table.size())) == true);
	table.clean(entry(host, tableShift, table.size()));
#endif
}


void PageTable::flushDirectory(Directory &dir)
{
#ifdef USE_VM
	for(int i = 0; i < dir.size(); i++) {
		if(dir.present(i) == false) continue;
		TRACE("Flushing Directory entry %d", i);
		Table &table = dir.get(i);
		table.flush();
	}
	dir.flush();
#endif
}

void PageTable::syncDirectory(Directory &dir)
{
#ifdef USE_VM
	for(int i = 0; i < dir.size(); i++) {
		if(dir.present(i) == false) continue;
		TRACE("Sync Directory Entry %d", i);
		dir.get(i).sync();
	}
	dir.sync();
#endif
}

void PageTable::sync()
{
#ifdef USE_VM
		if(_valid == true) return;
		enterFunction(vmSync);
		for(int i = 0; i < rootTable.size(); i++) {
			if(rootTable.present(i) == false) continue;
			syncDirectory(rootTable.get(i));
		}
		rootTable.sync();
		_valid = true;
		exitFunction();
#endif
}


void *PageTable::flush() 
{
#ifdef USE_VM
	// If there haven't been modifications, just return
	if(_clean == true) return rootTable.device();

	TRACE("PT Flush");

	for(int i = 0; i < rootTable.size(); i++) {
		if(rootTable.present(i) == false) continue;
		TRACE("Flusing entry %d", i);
		flushDirectory(rootTable.get(i));
	}

	rootTable.flush();
	_clean = true;
	return rootTable.device();
#else
	return NULL;
#endif
}

}}
