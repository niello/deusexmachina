#include "ShaderMetadataCache.h"

#include <DEMShaderCompiler/DEMShaderCompilerDLL.h>

CShaderMetadata* CShaderMetadataCache::GetMetadata(U32 ShaderID)
{
	IPTR Idx = Cache.FindIndex(ShaderID);
	if (Idx != INVALID_INDEX) return Cache.ValueAt(Idx);

	U32 Target;
	CShaderMetadata* pMeta;
	if (!DLLLoadShaderMetadataByObjectFileID(ShaderID, Target, pMeta)) return NULL;

	Cache.Add(ShaderID, pMeta);
	return pMeta;
}
//---------------------------------------------------------------------

void CShaderMetadataCache::Clear()
{
	for (UPTR i = 0; i < Cache.GetCount(); ++i)
		DLLFreeShaderMetadata(Cache.ValueAt(i));
	Cache.Clear(true);
}
//---------------------------------------------------------------------
