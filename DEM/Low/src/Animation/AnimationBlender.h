#pragma once
#include <Animation/PoseOutput.h>
#include <System/System.h>

// Order independent priority+weight animation blender. Accepts only local transformation sources.
// Animation players and static poses, to name a few. Each animated node is associated with
// a blender port (referenced by stable index). Port receives incoming transforms and applies
// the final one to the node after blending. Source count must be specified at creation,
// changing the input count invalidates current transformations.

// NB: higher priority values mean higher logical priority

namespace DEM::Anim
{
using PAnimationBlenderInput = Ptr<class CAnimationBlenderInput>;
using PAnimationBlender = std::unique_ptr<class CAnimationBlender>;

class CAnimationBlender final
{
protected:

	std::vector<PAnimationBlenderInput> _Sources; // FIXME: by value, if pose outputs will not be refcounted?
	std::vector<UPTR>                   _SourcesByPriority;
	std::vector<Math::CTransformSRT>    _Transforms;   // per port per source
	std::vector<U8>                     _ChannelMasks; // per port per source
	U16                                 _PortCount = 0;
	bool                                _PrioritiesChanged = false;

public:

	CAnimationBlender();
	~CAnimationBlender();

	void Initialize(U8 SourceCount, U8 PortCount);
	void EvaluatePose(IPoseOutput& Output);

	auto GetInput(U8 Source) const { return (_Sources.size() > Source) ? _Sources[Source].Get() : nullptr; }

	void SetPriority(U8 Source, U16 Priority);
	void SetWeight(U8 Source, float Weight);

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

class CAnimationBlenderInput : public IPoseOutput
{
protected:

	friend class CAnimationBlender; // for write access to weight and priority

	CAnimationBlender* _pBlender = nullptr;
	float              _Weight = 0.f; // Set <= 0.f to deactivate this source
	U16                _Priority = 0;
	U8                 _Index;

public:

	CAnimationBlenderInput(CAnimationBlender& Blender, U8 Index) : _pBlender(&Blender), _Index(Index) {}

	virtual void SetScale(U16 Port, const vector3& Scale) override { _pBlender->SetScale(_Index, Port, Scale); }
	virtual void SetRotation(U16 Port, const quaternion& Rotation) override { _pBlender->SetRotation(_Index, Port, Rotation); }
	virtual void SetTranslation(U16 Port, const vector3& Translation) override { _pBlender->SetTranslation(_Index, Port, Translation); }
	virtual void SetTransform(U16 Port, const Math::CTransformSRT& Tfm) override { _pBlender->SetTransform(_Index, Port, Tfm); }

	float        GetWeight() const { return _Weight; }
	U16          GetPriority() const { return _Priority; }
	U8           GetIndex() const { return _Index; }
};

}
