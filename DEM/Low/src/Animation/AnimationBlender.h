#pragma once
#include <Animation/PoseOutput.h>
#include <Data/Ptr.h>

// Order independent priority+weight animation blender. Accepts only local transformation sources.
// Animation players and static poses, to name a few. Each animated node is associated with
// a blender port (referenced by stable index). Port receives incoming transforms and applies
// the final one to the node after blending. Source count must be specified at creation,
// changing the input count invalidates current transformations.

// NB: higher priority values mean higher logical priority

namespace DEM::Anim
{
using PAnimationBlender = std::unique_ptr<class CAnimationBlender>;

class CAnimationBlender final
{
protected:

	class CInput : public IPoseOutput
	{
	public:

		CAnimationBlender& _Blender;
		float              _Weight = 0.f; // Set <= 0.f to deactivate this source
		U16                _Priority = 0;
		U8                 _Index;

		CInput(CAnimationBlender& Blender, U8 Index) : _Blender(Blender), _Index(Index) {}

		virtual void SetScale(U16 Port, const vector3& Scale) override { _Blender.SetScale(_Index, Port, Scale); }
		virtual void SetRotation(U16 Port, const quaternion& Rotation) override { _Blender.SetRotation(_Index, Port, Rotation); }
		virtual void SetTranslation(U16 Port, const vector3& Translation) override { _Blender.SetTranslation(_Index, Port, Translation); }
		virtual void SetTransform(U16 Port, const Math::CTransformSRT& Tfm) override { _Blender.SetTransform(_Index, Port, Tfm); }
	};

	std::vector<CInput>              _Sources;
	std::vector<U8>                  _SourcesByPriority;
	std::vector<Math::CTransformSRT> _Transforms;   // per port per source
	std::vector<U8>                  _ChannelMasks; // per port per source
	U16                              _PortCount = 0;
	bool                             _PrioritiesChanged = false;

public:

	CAnimationBlender();
	~CAnimationBlender();

	void         Initialize(U8 SourceCount, U8 PortCount);
	void         EvaluatePose(IPoseOutput& Output);

	IPoseOutput* GetInput(U8 Source) { return (_Sources.size() > Source) ? &_Sources[Source] : nullptr; }

	void         SetPriority(U8 Source, U16 Priority);
	void         SetWeight(U8 Source, float Weight);
	U16          GetPriority(U8 Source) const { return (_Sources.size() > Source) ? _Sources[Source]._Priority : 0; }
	float        GetWeight(U8 Source) const { return (_Sources.size() > Source) ? _Sources[Source]._Weight : 0.f; }

	void SetScale(U8 Source, U16 Port, const vector3& Scale)
	{
		const UPTR Idx = Port * _Sources.size() + Source;
		_Transforms[Idx].Scale = Scale;
		_ChannelMasks[Idx] |= ETransformChannel::Scaling;
	}

	void SetRotation(U8 Source, U16 Port, const quaternion& Rotation)
	{
		const UPTR Idx = Port * _Sources.size() + Source;
		_Transforms[Idx].Rotation = Rotation;
		_ChannelMasks[Idx] |= ETransformChannel::Rotation;
	}

	void SetTranslation(U8 Source, U16 Port, const vector3& Translation)
	{
		const UPTR Idx = Port * _Sources.size() + Source;
		_Transforms[Idx].Translation = Translation;
		_ChannelMasks[Idx] |= ETransformChannel::Translation;
	}

	void SetTransform(U8 Source, U16 Port, const Math::CTransformSRT& Tfm)
	{
		const UPTR Idx = Port * _Sources.size() + Source;
		_Transforms[Idx] = Tfm;
		_ChannelMasks[Idx] = ETransformChannel::All;
	}
};

}
