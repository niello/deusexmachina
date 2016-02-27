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
	CArray<CMacroDBRec>	Defines;

	CShaderDBRec(): ID(0), Target(0) {}
};

bool BuildIntermediateShaderObjectFilePath(U32 ID, const char* pExtension, CString& OutPath);
bool OpenDB(const char* pURI);
void CloseDB();
bool FindShaderRec(CShaderDBRec& InOut);
bool WriteShaderRec(CShaderDBRec& InOut);
bool FindObjFile(CFileData& InOut, const void* pBinaryData, bool SkipHeader);
bool FindObjFileByID(U32 ID, CFileData& Out);
bool RegisterObjFile(CFileData& InOut, const char* Extension);
bool ReleaseObjFile(U32 ID, CString& OutPath);

#endif
