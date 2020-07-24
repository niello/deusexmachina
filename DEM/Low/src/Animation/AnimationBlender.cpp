#include "AnimationBlender.h"
#include <Scene/SceneNode.h>

namespace DEM::Anim
{

CAnimationBlender::CAnimationBlender() = default;
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
	const auto SourceCount = _Sources.size();
	if (!_PortCount || !SourceCount) return;

	const bool DirectMapping = _PortMapping.empty();

	for (UPTR Port = 0; Port < _PortCount; ++Port)
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

		const auto OutputPort = DirectMapping ? Port : _PortMapping[Port];

		if (FinalMask & ETransformChannel::Scaling)
			_pOutput->SetScale(OutputPort, FinalTfm.Scale);

		if (FinalMask & ETransformChannel::Rotation)
		{
			if (RotationWeights < 1.f) FinalTfm.Rotation.normalize();
			_pOutput->SetRotation(OutputPort, FinalTfm.Rotation);
		}

		if (FinalMask & ETransformChannel::Translation)
			_pOutput->SetTranslation(OutputPort, FinalTfm.Translation);
	}

	// Source data is blended into the output, no more channels are ready to be blended
	std::memset(_ChannelMasks.data(), 0, _ChannelMasks.size());
}
//---------------------------------------------------------------------

// NB: slow operation, try to bind all your sources once on blender init
U16 CAnimationBlender::BindNode(CStrID NodeID, U16 ParentPort)
{
	if (!_pOutput) return IPoseOutput::InvalidPort;

	const bool DirectMapping = _PortMapping.empty();

	const auto ParentOutputPort = DirectMapping ? ParentPort : _PortMapping[ParentPort];
	const auto OutputPort = _pOutput->BindNode(NodeID, ParentOutputPort);
	if (OutputPort == IPoseOutput::InvalidPort) return IPoseOutput::InvalidPort;

	// Check if this output port is alread mapped to our input port
	if (DirectMapping)
	{
		if (OutputPort < _PortCount) return OutputPort;
	}
	else
	{
		auto It = std::find(_PortMapping.cbegin(), _PortMapping.cend(), OutputPort);
		if (It != _PortMapping.cend()) return static_cast<U16>(std::distance(_PortMapping.cbegin(), It));
	}

	// Allocate new port
	const auto Port = _PortCount++;
	if (!DirectMapping || Port != OutputPort)
	{
		if (DirectMapping)
		{
			// Direct mapping satisfies our data no more, build explicit mapping
			_PortMapping.resize(Port);
			for (U16 i = 0; i < Port; ++i)
				_PortMapping[i] = i;
		}

		_PortMapping.push_back(OutputPort);
		n_assert_dbg(_PortMapping.size() == _PortCount);
	}

	return Port;
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

}
