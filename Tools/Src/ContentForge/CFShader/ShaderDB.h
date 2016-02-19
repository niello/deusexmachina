#pragma once
#ifndef __DEM_TOOLS_SHADER_DB_H__
#define __DEM_TOOLS_SHADER_DB_H__

#include <Data/String.h>
#include <Data/Array.h>

struct CFileData
{
	U32		ID;
	U64		Size;
	U32		CRC;
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
	U32					ID;
	U32					ShaderType;
	U32					Target;
	U32					CompilerFlags;
	CString				EntryPoint;
	CFileData			SrcFile;
	U64					SrcModifyTimestamp;
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
bool FindObjFile(CFileData& InOut, const void* pBinaryData, bool SkipHeader);
bool RegisterObjFile(CFileData& InOut, const char* Extension);
bool ReleaseObjFile(U32 ID, CString& OutPath);

#endif
