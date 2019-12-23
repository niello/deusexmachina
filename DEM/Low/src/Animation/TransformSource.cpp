#include "TransformSource.h"
#include <Animation/AnimationBlender.h>
#include <Scene/SceneNode.h>

namespace DEM::Anim
{

void CTransformSource::SetBlending(PAnimationBlender Blender, U8 SourceIndex)
{
	if (Blender)
	{
		_Blender = std::move(Blender);
		_SourceIndex = SourceIndex;

		//for (auto& Output : _Outputs)
		//	Output.BlenderPort = _Blender->GetOrCreateNodePort(*Output.Node);
	}
	else
	{
		//for (auto& Output : _Outputs)
		//	Output.Node = _Blender->GetPortNode(Output.BlenderPort);

		_Blender = nullptr;
	}
}
//---------------------------------------------------------------------

}
