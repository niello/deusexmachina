#pragma once
#ifndef __DEM_TOOLS_SHADER_DB_H__
#define __DEM_TOOLS_SHADER_DB_H__

#include <Data/String.h>
#include <Data/Array.h>

namespace DB
{
	class CValueTable;
}

namespace Data
{
	class CParams;
}

struct CSrcFileData
{
	U32		ID;
	U64		Size;
	U32		CRC;
	CString	Path;
};

struct CObjFileData
{
	U32		ID;
	U64		Size;
	U64		BytecodeSize;
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
	CSrcFileData		SrcFile;
	U64					SrcModifyTimestamp;
	CObjFileData		ObjFile;
	CObjFileData		InputSigFile;
	CArray<CMacroDBRec>	Defines;

	CShaderDBRec(): ID(0), Target(0) {}
};

bool	OpenDB(const char* pURI);
void	CloseDB();
bool	FindShaderRec(CShaderDBRec& InOut);
bool	WriteShaderRec(CShaderDBRec& InOut);
bool	FindObjFile(CObjFileData& InOut, const void* pBinaryData, bool SkipHeader);
bool	FindObjFileByID(U32 ID, CObjFileData& Out);
U32		CreateObjFileRecord();
bool	UpdateObjFileRecord(const CObjFileData& Record);
bool	ReleaseObjFile(U32 ID, CString& OutPath);
bool	ExecuteSQLQuery(const char* pSQL, DB::CValueTable* pOutTable = NULL, const Data::CParams* pParams = NULL);

#endif
