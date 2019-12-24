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

	// All sources initially have the same priority, order is not important
	_SourcesByPriority.resize(SourceCount);
	for (size_t i = 0; i < SourceCount; ++i)
		_SourcesByPriority[i] = i;
}
//---------------------------------------------------------------------

void CAnimationBlender::Apply()
{
	const auto PortCount = _Nodes.size();
	const auto SourceCount = _SourceInfo.size();
	if (!PortCount || !SourceCount) return;

	for (UPTR Port = 0; Port < PortCount; ++Port)
	{
		Math::CTransformSRT FinalTfm;
		U8 FinalMask = 0;
		float ScaleWeights = 0.f;
		float RotationWeights = 0.f;
		float TranslationWeights = 0.f;

		const auto Offset = Port * SourceCount;

		for (const auto& SourceIndex : _SourcesByPriority)
		{
			const float SourceWeight = _SourceInfo[SourceIndex].Weight;
			if (SourceWeight <= 0.f) continue;

			const auto CurrTfm = _Transforms[Offset + SourceIndex];
			const U8 ChannelMask = _ChannelMasks[Offset + SourceIndex];

			if ((ChannelMask & Scene::Tfm_Scaling) && ScaleWeights < 1.f)
			{
				const float Weight = std::min(SourceWeight, 1.f - ScaleWeights);

				// Scale is 1.f by default. To blend correctly, we must reset it to zero before applying the first source.
				if (FinalMask & Scene::Tfm_Scaling)
					FinalTfm.Scale += CurrTfm.Scale * Weight;
				else
					FinalTfm.Scale = CurrTfm.Scale * Weight;

				FinalMask |= Scene::Tfm_Scaling;
				ScaleWeights += Weight;
			}

			if ((ChannelMask & Scene::Tfm_Rotation) && RotationWeights < 1.f)
			{
				const float Weight = std::min(SourceWeight, 1.f - RotationWeights);

				// TODO: check this hardcore stuff for multiple quaternion blending:
				// https://ntrs.nasa.gov/archive/nasa/casi.ntrs.nasa.gov/20070017872.pdf
				if (FinalMask & Scene::Tfm_Rotation)
				{
					quaternion Q;
					Q.slerp(quaternion::Identity, CurrTfm.Rotation, Weight);
					FinalTfm.Rotation *= Q;
				}
				else
				{
					FinalTfm.Rotation.slerp(quaternion::Identity, CurrTfm.Rotation, Weight);
				}

				FinalMask |= Scene::Tfm_Rotation;
				RotationWeights += Weight;
			}

			if ((ChannelMask & Scene::Tfm_Translation) && TranslationWeights < 1.f)
			{
				const float Weight = std::min(SourceWeight, 1.f - TranslationWeights);
				FinalTfm.Translation += CurrTfm.Translation * Weight;
				FinalMask |= Scene::Tfm_Translation;
				TranslationWeights += Weight;
			}
		}

		// Apply accumulated transform

		if (FinalMask & Scene::Tfm_Scaling)
			_Nodes[Port]->SetScale(FinalTfm.Scale);

		if (FinalMask & Scene::Tfm_Rotation)
		{
			if (RotationWeights < 1.f) FinalTfm.Rotation.normalize();
			_Nodes[Port]->SetRotation(FinalTfm.Rotation);
		}

		if (FinalMask & Scene::Tfm_Translation)
			_Nodes[Port]->SetPosition(FinalTfm.Translation);
	}

	std::memset(_ChannelMasks.data(), 0, _ChannelMasks.size());
}
//---------------------------------------------------------------------

void CAnimationBlender::SetPriority(U8 Source, U32 Priority)
{
	if (Source < _SourceInfo.size() && _SourceInfo[Source].Priority != Priority)
	{
		_SourceInfo[Source].Priority = Priority;

		std::sort(_SourcesByPriority.begin(), _SourcesByPriority.end(), [this](UPTR a, UPTR b)
		{
			return _SourceInfo[a].Priority > _SourceInfo[b].Priority;
		});
	}
}
//---------------------------------------------------------------------

// NB: slow operation
U32 CAnimationBlender::GetOrCreateNodePort(Scene::CSceneNode* pNode)
{
	if (!pNode || _Nodes.size() >= std::numeric_limits<U32>().max())
		return InvalidPort;

	const auto PortCount = _Nodes.size();
	for (UPTR Port = 0; Port < PortCount; ++Port)
		if (_Nodes[Port] == pNode)
			return Port;

	_Nodes.push_back(pNode);
	_Transforms.resize(_Transforms.size() + _SourceInfo.size());
	_ChannelMasks.resize(_ChannelMasks.size() + _SourceInfo.size());

	return static_cast<U32>(_Nodes.size() - 1);
}
//---------------------------------------------------------------------

}
