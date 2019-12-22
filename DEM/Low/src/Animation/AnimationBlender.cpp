#include "AnimationBlender.h"
#include <Scene/SceneNode.h>

namespace DEM::Anim
{

// NB: this invalidates all current transforms at least for now
void CAnimationBlender::Initialize(U8 SourceCount)
{
	_SourceInfo.resize(SourceCount);
	_Nodes.clear();
	_Transforms.clear();
	_ChannelMasks.clear();
}
//---------------------------------------------------------------------

void CAnimationBlender::Apply()
{
	const auto PortCount = _Nodes.size();
	const auto SourceCount = _SourceInfo.size();
	if (!PortCount || !SourceCount) return;

	UPTR i = 0;
	for (UPTR Port = 0; Port < PortCount; ++Port)
	{
		Math::CTransformSRT FinalTfm;
		U8 FinalMask = 0;
		//!!!accumulate weights and priorities per channel!

		for (UPTR Source = 0; Source < SourceCount; ++Source, ++i)
		{
			const U8 ChannelMask = _ChannelMasks[i];

			if (ChannelMask & Scene::Tfm_Scaling)
			{
				FinalTfm.Scale = _Transforms[i].Scale;
				// blend

				FinalMask |= Scene::Tfm_Scaling;
			}

			if (ChannelMask & Scene::Tfm_Rotation)
			{
				FinalTfm.Rotation = _Transforms[i].Rotation;
				// blend

				FinalMask |= Scene::Tfm_Rotation;
			}

			if (ChannelMask & Scene::Tfm_Translation)
			{
				FinalTfm.Translation = _Transforms[i].Translation;
				// blend

				FinalMask |= Scene::Tfm_Translation;
			}
		}

		// Apply accumulated transform

		if (FinalMask & Scene::Tfm_Scaling)
			_Nodes[Port]->SetScale(FinalTfm.Scale);

		if (FinalMask & Scene::Tfm_Rotation)
			_Nodes[Port]->SetRotation(FinalTfm.Rotation);

		if (FinalMask & Scene::Tfm_Translation)
			_Nodes[Port]->SetPosition(FinalTfm.Translation);
	}

	std::memset(_ChannelMasks.data(), 0, _ChannelMasks.size());
}
//---------------------------------------------------------------------

// NB: slow operation
U32 CAnimationBlender::GetOrCreateNodePort(Scene::CSceneNode& Node)
{
	if (_Nodes.size() >= std::numeric_limits<U32>().max())
		return InvalidPort;

	const auto PortCount = _Nodes.size();
	for (UPTR Port = 0; Port < PortCount; ++Port)
		if (_Nodes[Port] == &Node)
			return Port;

	_Nodes.push_back(&Node);
	_Transforms.resize(_Transforms.size() + _SourceInfo.size());
	_ChannelMasks.resize(_ChannelMasks.size() + _SourceInfo.size());

	return static_cast<U32>(_Nodes.size() - 1);
}
//---------------------------------------------------------------------

}
