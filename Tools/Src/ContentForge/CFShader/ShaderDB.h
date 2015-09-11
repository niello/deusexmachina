#pragma once
#ifndef __DEM_TOOLS_SHADER_DB_H__
#define __DEM_TOOLS_SHADER_DB_H__

#include <Data/String.h>
#include <Data/Array.h>

struct CFileData
{
	DWORD	ID;
	DWORD	Size;
	DWORD	CRC;
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
	DWORD				SrcModifyTimestamp;
	CFileData			ObjFile;
	CFileData			InputSigFile;
	char*				pDefineString;
	CArray<CMacroDBRec>	Defines;

	CShaderDBRec(): ID(0), pDefineString(NULL) {}
	~CShaderDBRec() { SAFE_FREE(pDefineString); }
};

bool OpenDB(const char* pURI);
void CloseDB();
bool FindShaderRec(CShaderDBRec& InOut);
bool WriteShaderRec(CShaderDBRec& InOut);
bool FindObjFile(CFileData& InOut, const void* pBinaryData);
bool RegisterObjFile(CFileData& InOut, const char* Extension);
bool ReleaseObjFile(DWORD ID, CString& OutPath);

#endif
