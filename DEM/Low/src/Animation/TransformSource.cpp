#include "TransformSource.h"

namespace DEM::Anim
{

void CTransformSource::SetBlending(PAnimationBlender Blender, U8 SourceIndex)
{
	// The same blender, no port remapping required
	if (_BlendInfo && Blender == _BlendInfo->Blender)
	{
		_BlendInfo->SourceIndex = SourceIndex;
		return;
	}

	// Reset previous blending, restore nodes
	if (_BlendInfo)
	{
		const auto OutputCount = _BlendInfo->Ports.size();
		_Nodes.resize(OutputCount);
		for (size_t i = 0; i < OutputCount; ++i)
			_Nodes[i] = _BlendInfo->Blender->GetPortNode(_BlendInfo->Ports[i]);

		_BlendInfo.reset();
	}

	// Enable new blending, map nodes to ports
	if (Blender)
	{
		_BlendInfo = std::make_unique<CBlendInfo>();
		_BlendInfo->Blender = std::move(Blender);
		_BlendInfo->SourceIndex = SourceIndex;

		const auto OutputCount = _Nodes.size();
		_BlendInfo->Ports.resize(OutputCount);
		for (size_t i = 0; i < OutputCount; ++i)
			_BlendInfo->Ports[i] = _BlendInfo->Blender->GetOrCreateNodePort(_Nodes[i]);

		std::swap(_Nodes, std::vector<Scene::PSceneNode>{});
	}
}
//---------------------------------------------------------------------

}
