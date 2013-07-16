#if defined(__WIN32__) || defined(DOXYGEN)

#define CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <StdDEM.h>

// Debug memory management functions

bool DEM_LogMemory = false;

void* n_malloc_dbg(size_t size, const char* filename, int line)
{
	void* res = _malloc_dbg(size, _NORMAL_BLOCK, filename, line);
	if (DEM_LogMemory) n_printf("%lx = n_malloc(size=%d, file=%s, line=%d)\n", res, size, filename, line);
	return res;
}
//---------------------------------------------------------------------

void* n_calloc_dbg(size_t num, size_t size, const char* filename, int line)
{
	void* res = _calloc_dbg(num, size, _NORMAL_BLOCK, filename, line);
	if (DEM_LogMemory) n_printf("%lx = n_calloc(num=%d, size=%d, file=%s, line=%d)\n", res, num, size, filename, line);
	return res;
}
//---------------------------------------------------------------------

void* n_realloc_dbg(void* memblock, size_t size, const char* filename, int line)
{
	void* res = _realloc_dbg(memblock, size, _NORMAL_BLOCK, filename, line);
	if (DEM_LogMemory) n_printf("%lx = n_realloc(ptr=%lx, size=%d, file=%s, line=%d)\n", res, memblock, size, filename, line);
	return res;
}
//---------------------------------------------------------------------

void n_free_dbg(void* memblock, const char* filename, int line)
{
	_free_dbg(memblock, _NORMAL_BLOCK);
	if (DEM_LogMemory) n_printf("n_free(ptr=%lx, file=%s, line=%d)\n", memblock, filename, line);
}
//---------------------------------------------------------------------

// Replacement global new operator without location reporting. This
// catches calls which don't use n_new for some reason.
void* operator new(size_t size)
{
	void* res = _malloc_dbg(size, _NORMAL_BLOCK, __FILE__, __LINE__);
	if (DEM_LogMemory) n_printf("%lx = new(size=%d)\n", res, size);
	return res;
}
//---------------------------------------------------------------------

// Replacement global new operator with location reporting (redirected from n_new()).
void* operator new(size_t size, const char* file, int line)
{
	void* res = _malloc_dbg(size, _NORMAL_BLOCK, file, line);
	if (DEM_LogMemory) n_printf("%lx = new(size=%d, file=%s, line=%d)\n", res, size, file, line);
	return res;
}
//---------------------------------------------------------------------

// Placement global new operator without location reporting. This
// catches calls which don't use n_new for some reason.
void* operator new(size_t size, void *place, const char* file, int line)
{
	void *res = place;
	if (DEM_LogMemory) n_printf("%lx = new(size=%d, place=0x%x, file=%s, line=%d)\n", res, size, place, file, line);
	return res;
}
//---------------------------------------------------------------------

// Replacement global new[] operator without location reporting.
void* operator new[](size_t size)
{
	void* res = _malloc_dbg(size, _NORMAL_BLOCK, __FILE__, __LINE__);
	if (DEM_LogMemory) n_printf("%lx = new[](size=%d)\n", res, size);
	return res;
}
//---------------------------------------------------------------------

// Replacement global new[] operator with location reporting.
void* operator new[](size_t size, const char* file, int line)
{
	void* res = _malloc_dbg(size, _NORMAL_BLOCK, file, line);
	if (DEM_LogMemory) n_printf("%lx = new[](size=%d, file=%s, line=%d)\n", res, size, file, line);
	return res;
}
//---------------------------------------------------------------------

void operator delete(void* p)
{
	if (DEM_LogMemory) n_printf("delete(ptr=%lx)\n", p);
	_free_dbg(p, _NORMAL_BLOCK);
}
//---------------------------------------------------------------------

// Replacement global delete operator to match the new with location reporting.
void operator delete(void* p, const char* /*file*/, int /*line*/)
{
	if (DEM_LogMemory) n_printf("delete(ptr=%lx)\n", p);
	_free_dbg(p, _NORMAL_BLOCK);
}
//---------------------------------------------------------------------

// Placement global delete operator to match the new with location reporting. do nothing
void operator delete(void*, void*, const char* /*file*/, int /*line*/)
{
}
//---------------------------------------------------------------------

// Replacement global delete[] operator.
void operator delete[](void* p)
{
	if (DEM_LogMemory) n_printf("delete[](ptr=%lx)\n", p);
	_free_dbg(p, _NORMAL_BLOCK);
}
//---------------------------------------------------------------------

// Replacement global delete[] operator to match the new with location reporting.
void operator delete[](void* p, const char* /*file*/, int /*line*/)
{
	if (DEM_LogMemory) n_printf("delete[](ptr=%lx)\n", p);
	_free_dbg(p, _NORMAL_BLOCK);
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
nMemoryStats n_dbgmemgetstats()
{
	_CrtMemState crtState = { 0 };
	_CrtMemCheckpoint(&crtState);
	nMemoryStats MemStats = { 0 };
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

