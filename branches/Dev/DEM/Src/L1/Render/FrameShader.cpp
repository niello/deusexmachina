#include "FrameShader.h"

namespace Render
{

//!!!OLD!
int CFrameShader::FindShaderIndex(const nString& ShaderName) const
{
	for (int i = 0; i < shaders.Size(); ++i)
		if (shaders[i].GetName() == ShaderName)
			return i;
	return -1;
}
//---------------------------------------------------------------------

//!!!OLD!
int CFrameShader::FindRenderTargetIndex(const nString& RTName) const
{
	for (int i = 0; i < renderTargets.Size(); ++i)
		if (renderTargets[i].GetName() == RTName)
			return i;
	return -1;
}
//---------------------------------------------------------------------

}