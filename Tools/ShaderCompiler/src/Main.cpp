#include <ShaderCompiler.h>
#include <ShaderDB.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

BOOL WINAPI DllMain(_In_ HINSTANCE hinstDLL, _In_ DWORD fdwReason, _In_ LPVOID lpvReserved)
{
	if (fdwReason == DLL_PROCESS_DETACH)
	{
		// DLL is being unloaded for the current process
		DB::CloseConnection();
	}

	return TRUE;
}
//---------------------------------------------------------------------
