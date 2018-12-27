#pragma once
#ifndef __DEM_TOOLS_SHADER_METADATA_CHAHE_H__
#define __DEM_TOOLS_SHADER_METADATA_CHAHE_H__

#include <ShaderReflection.h>
#include <Data/Dictionary.h>

// Shader metadata cache object helps to avoid redundant shader loading and manages DLL-allocated metadata objects
// NB: don't terminate shader compiler DLL if cache contains DLL-allocated metadata objects

class CShaderMetadataCache
{
private:

	CDict<U32, CShaderMetadata*> Cache;

public:

	~CShaderMetadataCache() { Clear();}

	CShaderMetadata*	GetMetadata(U32 ShaderID);
	void				Clear();
};

#endif
