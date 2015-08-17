#pragma once
#ifndef __DEM_TOOLS_SHADER_DB_H__
#define __DEM_TOOLS_SHADER_DB_H__

#include <Data/String.h>
#include <Data/Array.h>

struct CFileData
{
	DWORD	Size;
	DWORD	CRC;
	DWORD	ModifyTimestamp;
	CString	Path;
};

struct CMacroDBRec
{
	const char*	Name;
	const char*	Value;

	//!!!sorting cmp operators!
};

struct CShaderDBRec
{
	DWORD				ID;
	DWORD				ShaderType;
	DWORD				Target;
	DWORD				CompilerFlags;
	CString				EntryPoint;
	CFileData			SrcFile;
	CFileData			ObjFile;
	char*				pDefineString;
	CArray<CMacroDBRec>	Defines;

	CShaderDBRec(): ID(0), pDefineString(NULL) {}
	~CShaderDBRec() { SAFE_FREE(pDefineString); }
};

bool OpenDB(const char* pURI);
void CloseDB();
bool FindShaderRec(CShaderDBRec& InOut);

#endif
