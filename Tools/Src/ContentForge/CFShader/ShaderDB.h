#pragma once
#ifndef __DEM_TOOLS_SHADER_DB_H__
#define __DEM_TOOLS_SHADER_DB_H__

#include <Data/String.h> //???or fixed char[] fields?

struct CFileData
{
	DWORD	ModifyTimestamp;
	DWORD	Size;
	DWORD	CRC;
	CString	Path;
};

struct CMacroDBRec
{
	CString	Name;
	CString	Value;
};

struct CShaderDBRec
{
	DWORD		ID;
	DWORD		ShaderType;
	DWORD		Target;
	DWORD		CompilerFlags;
	//???compiler version?
	CString		EntryPoint;
	CFileData	SrcFile;
	CFileData	ObjFile;

	//C???Fixed???Array defines?
};

#endif
