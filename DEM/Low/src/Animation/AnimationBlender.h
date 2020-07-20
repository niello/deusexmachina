#pragma once
#include <Animation/PoseOutput.h>
#include <Scene/SceneNode.h>

// Order independent priority+weight animation blender. Accepts only local transformation sources.
// Animation players and static poses, to name a few. Each animated node is associated with
// a blender port (referenced by stable index). Port receives incoming transforms and applies
// the final one to the node after blending. Source count must be specified at creation,
// changing the input count invalidates current transformations.

// NB: higher priority values mean higher logical priority

namespace DEM::Anim
{
using PAnimationBlenderInput = Ptr<class CAnimationBlenderInput>;
using PAnimationBlender = Ptr<class CAnimationBlender>;

class CAnimationBlenderInput : public IPoseOutput
{
protected:

	PAnimationBlender _Blender;
	U32               _Priority = 0;
	float             _Weight = 0.f; // Set <= 0.f to deactivate this source
	U8                _Index;

public:

	virtual U16  BindNode(CStrID NodeID, U16 ParentPort) override;
	virtual U8   GetActivePortChannels(U16 Port) const override;

	virtual void SetScale(U16 Port, const vector3& Scale) override { _Blender->SetScale(_Index, Port, Scale); }
	virtual void SetRotation(U16 Port, const quaternion& Rotation) override { _Blender->SetRotation(_Index, Port, Rotation); }
	virtual void SetTranslation(U16 Port, const vector3& Translation) override { _Blender->SetTranslation(_Index, Port, Translation); }
	virtual void SetTransform(U16 Port, const Math::CTransformSRT& Tfm) override { _Blender->SetTransform(_Index, Port, Tfm); }

	U8           GetIndex() const { return _Index; }
};

class CAnimationBlender
{
protected:

	std::vector<PAnimationBlenderInput> _Sources;
	std::vector<UPTR>                   _SourcesByPriority;
	std::vector<Scene::CSceneNode*>     _Nodes;
	std::vector<Math::CTransformSRT>    _Transforms;   // per node per source
	std::vector<U8>                     _ChannelMasks; // per node per source

public:

	CAnimationBlender() = default;
	CAnimationBlender(U8 SourceCount) { Initialize(SourceCount); }

	void               Initialize(U8 SourceCount);
	void               Apply();

	void               SetPriority(U8 Source, U32 Priority);
	void               SetWeight(U8 Source, float Weight) { if (Source < _Sources.size()) _Sources[Source].Weight = Weight; }

	void               SetScale(U8 Source, U16 Port, const vector3& Scale);
	void               SetRotation(U8 Source, U16 Port, const quaternion& Rotation);
	void               SetTranslation(U8 Source, U16 Port, const vector3& Translation);
	void               SetTransform(U8 Source, U16 Port, const Math::CTransformSRT& Tfm);

	U32                GetOrCreateNodePort(Scene::CSceneNode* pNode);
	Scene::CSceneNode* GetPortNode(U16 Port) const;
	bool               IsNodeActive(U16 Port) const { return _Nodes[Port] && _Nodes[Port]->IsActive(); }
};

inline void CAnimationBlender::SetScale(U8 Source, U16 Port, const vector3& Scale)
{
	if (Port >= _Nodes.size()) return;
	const UPTR Idx = Port * _Sources.size() + Source;
	_Transforms[Idx].Scale = Scale;
	_ChannelMasks[Idx] |= ETransformChannel::Scaling;
}
//---------------------------------------------------------------------

inline void CAnimationBlender::SetRotation(U8 Source, U16 Port, const quaternion& Rotation)
{
	if (Port >= _Nodes.size()) return;
	const UPTR Idx = Port * _Sources.size() + Source;
	_Transforms[Idx].Rotation = Rotation;
	_ChannelMasks[Idx] |= ETransformChannel::Rotation;
}
//---------------------------------------------------------------------

inline void CAnimationBlender::SetTranslation(U8 Source, U16 Port, const vector3& Translation)
{
	if (Port >= _Nodes.size()) return;
	const UPTR Idx = Port * _Sources.size() + Source;
	_Transforms[Idx].Translation = Translation;
	_ChannelMasks[Idx] |= ETransformChannel::Translation;
}
//---------------------------------------------------------------------

inline void CAnimationBlender::SetTransform(U8 Source, U16 Port, const Math::CTransformSRT& Tfm)
{
	if (Port >= _Nodes.size()) return;
	const UPTR Idx = Port * _Sources.size() + Source;
	_Transforms[Idx] = Tfm;
	_ChannelMasks[Idx] |= ETransformChannel::All;
}
//---------------------------------------------------------------------

inline Scene::CSceneNode* CAnimationBlender::GetPortNode(U16 Port) const
{
	return (Port < _Nodes.size()) ? _Nodes[Port] : nullptr;
}
//---------------------------------------------------------------------

}
