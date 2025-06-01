#if DEM_PLATFORM_WIN32

#include "Memory.h"
#include <System/System.h>

#define CRTDBG_MAP_ALLOC
#include <crtdbg.h>

// Debug memory management functions

bool DEM_LogMemory = false;

void* n_malloc_aligned_dbg(size_t size, size_t Alignment, const char* filename, int line)
{
	void* res = _aligned_malloc_dbg(size, Alignment, filename, line);
	if (DEM_LogMemory) Sys::Log("%lx = n_malloc_aligned(size=%d, align=%d, file=%s, line=%d)\n", res, size, Alignment, filename, line);
	return res;
}
//---------------------------------------------------------------------

void* n_realloc_aligned_dbg(void* memblock, size_t size, size_t Alignment, const char* filename, int line)
{
	void* res = _aligned_realloc_dbg(memblock, size, Alignment, filename, line);
	if (DEM_LogMemory) Sys::Log("%lx = n_realloc_aligned(ptr=%lx, size=%d, align=%d, file=%s, line=%d)\n", res, memblock, size, Alignment, filename, line);
	return res;
}
//---------------------------------------------------------------------

void n_free_aligned_dbg(void* memblock, const char* filename, int line)
{
	_aligned_free_dbg(memblock);
	if (DEM_LogMemory) Sys::Log("n_free_aligned(ptr=%lx, file=%s, line=%d)\n", memblock, filename, line);
}
//---------------------------------------------------------------------

// Initialize the debug memory system.
// Enables automatic memory leak check at end of application
void n_dbgmeminit()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
}
//---------------------------------------------------------------------

// Dumps all the memory blocks in the debug heap when a memory leak has occurred (debug version only).
// Returns TRUE if a memory leak is found. Otherwise, the function returns FALSE
int n_dbgmemdumpleaks()
{
	return _CrtDumpMemoryLeaks();
}
//---------------------------------------------------------------------

// Create debug memory statistics.
CMemoryStats n_dbgmemgetstats()
{
	_CrtMemState crtState = { 0 };
	_CrtMemCheckpoint(&crtState);
	CMemoryStats MemStats = { 0 };
	MemStats.HighWaterSize = crtState.lHighWaterCount;

	for (int i = 0; i < _MAX_BLOCKS; ++i)
	{
		MemStats.TotalCount += crtState.lCounts[i];
		MemStats.TotalSize  += crtState.lSizes[i];
	}
	return MemStats;
}
//---------------------------------------------------------------------

#endif

