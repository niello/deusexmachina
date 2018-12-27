#include "Renderer.h"

#include <Render/RenderFwd.h>

namespace Render
{
__ImplementClassNoFactory(Render::IRenderer, Core::CRTTIBaseClass);

// Shader input sets define different sets of data application provides to GPU shader techniques
// through renderers. It is a way to define data flow conventions between a CPU and a GPU.
// Say, "Model" set means a vertex format with POSITION, TEXCOORD0, NORMAL, and a shader constant
// "WorldMatrix". "ModelSkinned" adds BLENDINDICES, BLENDWEIGHTS and "SkinPalette" constant, etc.
// Tracking these conventions in a dynamic dictionary allows to add new conventions at the runtime
// and implement new renderers with corresponding shader techs without touching the base code.
// Since without extensibility it would have been a static enum, I define it globally. Maybe
// later I'll figure out a better solution. Multithreading support also must be added.
CDict<CStrID, UPTR> ShaderInputSets;

UPTR RegisterShaderInputSetID(CStrID InputSetName)
{
	//!!!MT-unsafe!
	IPTR Idx = ShaderInputSets.FindIndex(InputSetName);
	if (Idx == INVALID_INDEX)
	{
		UPTR NewID = ShaderInputSets.GetCount();
		ShaderInputSets.Add(InputSetName, NewID);
		return NewID;
	}
	else return ShaderInputSets.ValueAt(Idx);
}
//---------------------------------------------------------------------

}