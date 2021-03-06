#include "AnimationBlender.h"
#include <Animation/MappedPoseOutput.h>
#include <Scene/SceneNode.h>

namespace DEM::Anim
{
CAnimationBlender::CAnimationBlender() = default;
CAnimationBlender::~CAnimationBlender() = default;

void CAnimationBlender::Initialize(U8 SourceCount, U8 PortCount)
{
	_Sources.clear();
	_Sources.reserve(SourceCount);
	for (U8 i = 0; i < SourceCount; ++i)
		_Sources.emplace_back(*this, i);

	// All sources initially have the same priority, order is not important
	_SourcesByPriority.resize(SourceCount);
	for (U8 i = 0; i < SourceCount; ++i)
		_SourcesByPriority[i] = i;
	_PrioritiesChanged = false;

	// Allocate blend matrix (sources * ports)
	const auto MatrixCellCount = PortCount * SourceCount;
	_Transforms.resize(MatrixCellCount);
	_ChannelMasks.resize(MatrixCellCount, 0);

	_PortCount = PortCount;
}
//---------------------------------------------------------------------

void CAnimationBlender::EvaluatePose(IPoseOutput& Output)
{
	const auto SourceCount = _Sources.size();
	if (!SourceCount) return;

	if (_PrioritiesChanged)
	{
		std::sort(_SourcesByPriority.begin(), _SourcesByPriority.end(), [this](U8 a, U8 b)
		{
			return _Sources[a]._Priority > _Sources[b]._Priority;
		});
		_PrioritiesChanged = false;
	}

	for (UPTR Port = 0; Port < _PortCount; ++Port)
	{
		Math::CTransformSRT FinalTfm;
		U8 FinalMask = 0;
		float ScaleWeights = 0.f;
		float RotationWeights = 0.f;
		float TranslationWeights = 0.f;

		const auto Offset = Port * SourceCount;

		for (const U8 SourceIndex : _SourcesByPriority)
		{
			const float SourceWeight = _Sources[SourceIndex]._Weight;
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

				quaternion Q;
				Q.x = CurrTfm.Rotation.x * Weight;
				Q.y = CurrTfm.Rotation.y * Weight;
				Q.z = CurrTfm.Rotation.z * Weight;
				Q.w = CurrTfm.Rotation.w * Weight;

				if (FinalMask & ETransformChannel::Rotation)
				{
					// Blend with shortest arc, based on a 4D dot product sign
					if (Q.x * FinalTfm.Rotation.x + Q.y * FinalTfm.Rotation.y + Q.z * FinalTfm.Rotation.z + Q.w * FinalTfm.Rotation.w < 0.f)
						FinalTfm.Rotation -= Q;
					else
						FinalTfm.Rotation += Q;
				}
				else
				{
					FinalTfm.Rotation = Q;
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
			Output.SetScale(Port, FinalTfm.Scale);

		if (FinalMask & ETransformChannel::Rotation)
		{
			if (RotationWeights < 1.f) FinalTfm.Rotation.normalize();
			Output.SetRotation(Port, FinalTfm.Rotation);
		}

		if (FinalMask & ETransformChannel::Translation)
			Output.SetTranslation(Port, FinalTfm.Translation);
	}

	// Source data is blended into the output, no more channels are ready to be blended
	std::memset(_ChannelMasks.data(), 0, _ChannelMasks.size());
}
//---------------------------------------------------------------------

void CAnimationBlender::SetPriority(U8 Source, U16 Priority)
{
	if (Source < _Sources.size() && _Sources[Source]._Priority != Priority)
	{
		_Sources[Source]._Priority = Priority;
		_PrioritiesChanged = true;
	}
}
//---------------------------------------------------------------------

void CAnimationBlender::SetWeight(U8 Source, float Weight)
{
	if (Source < _Sources.size()) _Sources[Source]._Weight = Weight;
}
//---------------------------------------------------------------------

}
