#include "Pass.h"

#include <Render/FrameShader.h> //!!!TMP for Validate!

namespace Render
{

//!!!OLD!
void CPass::Validate()
{
	n_assert(pFrameShader);

	// find shader
	if (rpShaderIndex == -1 && shaderAlias.IsValid())
	{
		rpShaderIndex = pFrameShader->FindShaderIndex(shaderAlias);
		if (rpShaderIndex == -1)
			n_error("nRpPass::Validate(): couldn't find shader alias '%s' in render path xml file!", shaderAlias.Get());
	}
}
//---------------------------------------------------------------------

}
