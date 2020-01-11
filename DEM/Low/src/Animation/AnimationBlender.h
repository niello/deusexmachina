#pragma once
#include <Data/RefCounted.h>
#include <Scene/SceneNode.h>

// Order independent priority+weight animation blender. Accepts only local transformation sources.
// Animation players and static poses, to name a few. Each animated node is associated with
// a blender port (referenced by stable index). Port receives incoming transforms and applies
// the final one to the node after blending. Source count must be specified at creation,
// changing the input count invalidates current transformations.
// NB: higher priority values are first in a queue.

// TODO: can use as an input into another blender!

namespace DEM::Anim
{
typedef Ptr<class CAnimationBlender> PAnimationBlender;

enum ETransformChannel
{
	Tfm_Translation	= 0x01,
	Tfm_Rotation	= 0x02,
	Tfm_Scaling		= 0x04
};

class CAnimationBlender : public Data::CRefCounted
{
public:

	constexpr static U32 InvalidPort = std::numeric_limits<U32>().max();

protected:

	struct CSourceInfo
	{
		U32   Priority = 0;
		float Weight = 0.f; // Set <= 0.f to deactivate this source
	};

	std::vector<CSourceInfo>         _SourceInfo;
	std::vector<UPTR>                _SourcesByPriority;
	std::vector<Scene::CSceneNode*>  _Nodes;
	std::vector<Math::CTransformSRT> _Transforms;   // per node per source
	std::vector<U8>                  _ChannelMasks; // per node per source

public:

	CAnimationBlender() = default;
	CAnimationBlender(U8 SourceCount) { Initialize(SourceCount); }

	void               Initialize(U8 SourceCount);
	void               Apply();

	void               SetPriority(U8 Source, U32 Priority);
	void               SetWeight(U8 Source, float Weight) { if (Source < _SourceInfo.size()) _SourceInfo[Source].Weight = Weight; }

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
	const UPTR Idx = Port * _SourceInfo.size() + Source;
	_Transforms[Idx].Scale = Scale;
	_ChannelMasks[Idx] |= Tfm_Scaling;
}
//---------------------------------------------------------------------

inline void CAnimationBlender::SetRotation(U8 Source, U16 Port, const quaternion& Rotation)
{
	if (Port >= _Nodes.size()) return;
	const UPTR Idx = Port * _SourceInfo.size() + Source;
	_Transforms[Idx].Rotation = Rotation;
	_ChannelMasks[Idx] |= Tfm_Rotation;
}
//---------------------------------------------------------------------

inline void CAnimationBlender::SetTranslation(U8 Source, U16 Port, const vector3& Translation)
{
	if (Port >= _Nodes.size()) return;
	const UPTR Idx = Port * _SourceInfo.size() + Source;
	_Transforms[Idx].Translation = Translation;
	_ChannelMasks[Idx] |= Tfm_Translation;
}
//---------------------------------------------------------------------

inline void CAnimationBlender::SetTransform(U8 Source, U16 Port, const Math::CTransformSRT& Tfm)
{
	if (Port >= _Nodes.size()) return;
	const UPTR Idx = Port * _SourceInfo.size() + Source;
	_Transforms[Idx] = Tfm;
	_ChannelMasks[Idx] |= (Tfm_Scaling | Tfm_Rotation | Tfm_Translation);
}
//---------------------------------------------------------------------

inline Scene::CSceneNode* CAnimationBlender::GetPortNode(U16 Port) const
{
	return (Port < _Nodes.size()) ? _Nodes[Port] : nullptr;
}
//---------------------------------------------------------------------

}
