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
using PAnimationBlenderInput = std::unique_ptr<class CAnimationBlenderInput>;
using PAnimationBlender = std::unique_ptr<class CAnimationBlender>;

class CAnimationBlender final
{
protected:

	IPoseOutput*                        _pOutput = nullptr; //???PPoseOutput refcounted?
	std::vector<U16>                    _PortMapping;
	std::vector<PAnimationBlenderInput> _Sources;
	std::vector<UPTR>                   _SourcesByPriority;
	std::vector<Math::CTransformSRT>    _Transforms;   // per node per source
	std::vector<U8>                     _ChannelMasks; // per node per source
	U16                                 _PortCount = 0; // for lazy blend matrix allocation

public:

	CAnimationBlender();
	~CAnimationBlender();

	void Initialize(U8 SourceCount);
	void Apply();

	auto GetInput(U8 Source) const { return (_Sources.size() > Source) ? _Sources[Source].get() : nullptr; }

	U16  BindNode(CStrID NodeID, U16 ParentPort);
	void SetPriority(U8 Source, U16 Priority);
	void SetWeight(U8 Source, float Weight);

	U8 GetActivePortChannels(U16 Port) const
	{
		n_assert_dbg(_pOutput);
		//???PERF: for speed eliminate _pOutput nullptr check and do once on a caller side?
		//???cache port channels once a frame before blending, return cached value here?
		return _pOutput ? _pOutput->GetActivePortChannels(_PortMapping.empty() ? Port : _PortMapping[Port]) : 0;
	}

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

	PAnimationBlender _Blender;
	float             _Weight = 0.f; // Set <= 0.f to deactivate this source
	U16               _Priority = 0;
	U8                _Index;

public:

	virtual U16  BindNode(CStrID NodeID, U16 ParentPort) override { return _Blender->BindNode(NodeID, ParentPort); }
	virtual U8   GetActivePortChannels(U16 Port) const override { return _Blender->GetActivePortChannels(Port); }

	virtual void SetScale(U16 Port, const vector3& Scale) override { _Blender->SetScale(_Index, Port, Scale); }
	virtual void SetRotation(U16 Port, const quaternion& Rotation) override { _Blender->SetRotation(_Index, Port, Rotation); }
	virtual void SetTranslation(U16 Port, const vector3& Translation) override { _Blender->SetTranslation(_Index, Port, Translation); }
	virtual void SetTransform(U16 Port, const Math::CTransformSRT& Tfm) override { _Blender->SetTransform(_Index, Port, Tfm); }

	float        GetWeight() const { return _Weight; }
	U16          GetPriority() const { return _Priority; }
	U8           GetIndex() const { return _Index; }
};

}
