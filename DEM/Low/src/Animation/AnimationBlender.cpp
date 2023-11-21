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
	_Transforms.resize(MatrixCellCount, rtm::qvv_identity());
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
		rtm::qvvf FinalTfm = rtm::qvv_identity();
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
					FinalTfm.scale = rtm::vector_mul_add(CurrTfm.scale, Weight, FinalTfm.scale);
				else
					FinalTfm.scale = rtm::vector_mul(CurrTfm.scale, Weight);

				FinalMask |= ETransformChannel::Scaling;
				ScaleWeights += Weight;
			}

			if ((ChannelMask & ETransformChannel::Rotation) && RotationWeights < 1.f)
			{
				const float Weight = std::min(SourceWeight, 1.f - RotationWeights);

				const rtm::vector4f Q = rtm::vector_mul(rtm::quat_to_vector(CurrTfm.rotation), Weight);

				if (FinalMask & ETransformChannel::Rotation)
				{
					// Blend with shortest arc, based on a 4D dot product sign
					// TODO PERF: can instead of condition (Dot < 0.f) extract sign mask and apply to Q? Will it be faster?
					const rtm::vector4f FinalRotation = rtm::quat_to_vector(FinalTfm.rotation);
					const float Dot = rtm::vector_dot(Q, FinalRotation);
					if (Dot < 0.f)
						FinalTfm.rotation = rtm::vector_to_quat(rtm::vector_sub(FinalRotation, Q));
					else
						FinalTfm.rotation = rtm::vector_to_quat(rtm::vector_add(FinalRotation, Q));
				}
				else
				{
					FinalTfm.rotation = rtm::vector_to_quat(Q);
				}

				FinalMask |= ETransformChannel::Rotation;
				RotationWeights += Weight;
			}

			if ((ChannelMask & ETransformChannel::Translation) && TranslationWeights < 1.f)
			{
				const float Weight = std::min(SourceWeight, 1.f - TranslationWeights);
				FinalTfm.translation = rtm::vector_mul_add(CurrTfm.translation, Weight, FinalTfm.translation);
				FinalMask |= ETransformChannel::Translation;
				TranslationWeights += Weight;
			}
		}

		// Apply accumulated transform

		if (FinalMask & ETransformChannel::Scaling)
			Output.SetScale(Port, FinalTfm.scale);

		if (FinalMask & ETransformChannel::Rotation)
		{
			if (RotationWeights < 1.f) FinalTfm.rotation = rtm::quat_normalize(FinalTfm.rotation);
			Output.SetRotation(Port, FinalTfm.rotation);
		}

		if (FinalMask & ETransformChannel::Translation)
			Output.SetTranslation(Port, FinalTfm.translation);
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
