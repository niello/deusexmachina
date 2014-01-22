#pragma once
#ifndef __DEM_L1_CORE_H__
#define __DEM_L1_CORE_H__

// Core functions

namespace Core
{
	bool			ReportAssertionFailure(const char* pExpression, const char* pMessage, const char* pFile, int Line, const char* pFunc = NULL);
	void __cdecl	Error(const char* pMsg, ...);
}

#endif

