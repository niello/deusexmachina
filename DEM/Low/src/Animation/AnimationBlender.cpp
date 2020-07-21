#include "AnimationBlender.h"
#include <Scene/SceneNode.h>

namespace DEM::Anim
{

CAnimationBlender::CAnimationBlender() = default;
CAnimationBlender::CAnimationBlender(U8 SourceCount) { Initialize(SourceCount); }
CAnimationBlender::~CAnimationBlender() = default;

// NB: this invalidates all current transforms at least for now
void CAnimationBlender::Initialize(U8 SourceCount)
{
	_Sources.resize(SourceCount);
	_PortMapping.clear();
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
	const auto PortCount = _PortMapping.size();
	const auto SourceCount = _Sources.size();
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
			const float SourceWeight = _Sources[SourceIndex]->GetWeight();
			if (SourceWeight <= 0.f) continue;

			const auto& CurrTfm = _Transforms[Offset + SourceIndex];
			const U8 ChannelMask = _ChannelMasks[Offset + SourceIndex];

			if ((ChannelMask & ETransformChannel::Scaling) && ScaleWeights < 1.f)
			{
				const float Weight = std::min(SourceWeight, 1.f - ScaleWeights);

				// Scale is 1.f by default. To blend correctly, we must reset it to zero before applying the first source.
				if (FinalMask & ETransformChannel::Scaling)
					FinalTfm.Scale += CurrTfm.Scale * Weight;
				else
					FinalTfm.Scale = CurrTfm.Scale * Weight;

				FinalMask |= ETransformChannel::Scaling;
				ScaleWeights += Weight;
			}

			if ((ChannelMask & ETransformChannel::Rotation) && RotationWeights < 1.f)
			{
				const float Weight = std::min(SourceWeight, 1.f - RotationWeights);

				// TODO: check this hardcore stuff for multiple quaternion blending:
				// https://ntrs.nasa.gov/archive/nasa/casi.ntrs.nasa.gov/20070017872.pdf
				// Impl: http://wiki.unity3d.com/index.php/Averaging_Quaternions_and_Vectors
				// Impl: https://github.com/christophhagen/averaging-quaternions/blob/master/averageQuaternions.py
				if (FinalMask & ETransformChannel::Rotation)
				{
					quaternion Q;
					Q.slerp(quaternion::Identity, CurrTfm.Rotation, Weight);
					FinalTfm.Rotation *= Q;
				}
				else
				{
					FinalTfm.Rotation.slerp(quaternion::Identity, CurrTfm.Rotation, Weight);
				}

				FinalMask |= ETransformChannel::Rotation;
				RotationWeights += Weight;
			}

			if ((ChannelMask & ETransformChannel::Translation) && TranslationWeights < 1.f)
			{
				const float Weight = std::min(SourceWeight, 1.f - TranslationWeights);
				FinalTfm.Translation += CurrTfm.Translation * Weight;
				FinalMask |= ETransformChannel::Translation;
				TranslationWeights += Weight;
			}
		}

		// Apply accumulated transform

		if (FinalMask & ETransformChannel::Scaling)
			_pOutput->SetScale(Port, FinalTfm.Scale);

		if (FinalMask & ETransformChannel::Rotation)
		{
			if (RotationWeights < 1.f) FinalTfm.Rotation.normalize();
			_pOutput->SetRotation(Port, FinalTfm.Rotation);
		}

		if (FinalMask & ETransformChannel::Translation)
			_pOutput->SetTranslation(Port, FinalTfm.Translation);
	}

	// Source data is blended into the output, no more channels are ready to be blended
	std::memset(_ChannelMasks.data(), 0, _ChannelMasks.size());
}
//---------------------------------------------------------------------

void CAnimationBlender::SetPriority(U8 Source, U16 Priority)
{
	if (Source < _Sources.size() && _Sources[Source]->GetPriority() != Priority)
	{
		_Sources[Source]->_Priority = Priority;

		std::sort(_SourcesByPriority.begin(), _SourcesByPriority.end(), [this](UPTR a, UPTR b)
		{
			return _Sources[a]->GetPriority() > _Sources[b]->GetPriority();
		});
	}
}
//---------------------------------------------------------------------

void CAnimationBlender::SetWeight(U8 Source, float Weight)
{
	if (Source < _Sources.size()) _Sources[Source]->_Weight = Weight;
}
//---------------------------------------------------------------------

U16 CAnimationBlenderInput::BindNode(CStrID NodeID, U16 ParentPort)
{
	// redirect to blender
	// blender redirects to output, receives port, then resizes port-dependent arrays if required
}
//---------------------------------------------------------------------

U8 CAnimationBlenderInput::GetActivePortChannels(U16 Port) const
{
	// request availability from blender output?
}
//---------------------------------------------------------------------

/*
// NB: slow operation
U32 CAnimationBlender::GetOrCreateNodePort(Scene::CSceneNode* pNode)
{
	if (!pNode || _PortMapping.size() >= std::numeric_limits<U32>().max())
		return InvalidPort;

	const auto PortCount = _PortMapping.size();
	for (UPTR Port = 0; Port < PortCount; ++Port)
		if (_PortMapping[Port] == pNode)
			return Port;

	_PortMapping.push_back(pNode);
	_Transforms.resize(_Transforms.size() + _Sources.size());
	_ChannelMasks.resize(_ChannelMasks.size() + _Sources.size());

	return static_cast<U32>(_PortMapping.size() - 1);
}
//---------------------------------------------------------------------
*/

}
