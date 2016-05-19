#include "ShaderLibrary.h"

#include <Core/Factory.h>

namespace Render
{
__ImplementClass(Render::CShaderLibrary, 'SLIB', Resources::CResourceObject);

PShader CShaderLibrary::GetShaderByID(PGPUDriver GPU, U32 ID)
{
	if (GPU.IsNullPtr() || !ID) return NULL;

	//!!!find index sorted!
	IPTR Idx = INVALID_INDEX;
	if (Idx == INVALID_INDEX) return NULL;

	CRecord& Rec = TOC[Idx];
	if (Rec.LoadedShader.IsValidPtr()) return Rec.LoadedShader;

	//load shader

	return Rec.LoadedShader;
}
//---------------------------------------------------------------------

}