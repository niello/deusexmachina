#pragma once
#include <Data/RefCounted.h>
#include <Math/TransformSRT.h>
#include <Scene/SceneFwd.h>

// Order independent priority+weight animation blender. Accepts only local transformation sources.
// Animation players and static poses, to name a few. Each animated node is associated with
// a blender port (referenced by stable index). Port receives incoming transforms and applies
// the final one to the node after blending. Source count must be specified at creation,
// changing the input count invalidates current transformations.

// TODO: can use as an input into another blender!

namespace Scene
{
	class CSceneNode;
}

namespace DEM::Anim
{
typedef Ptr<class CAnimationBlender> PAnimationBlender;

class CAnimationBlender : public Data::CRefCounted
{
public:

	constexpr static U32 InvalidPort = std::numeric_limits<U32>().max();

protected:

	struct CSourceInfo
	{
		U32   Priority;
		float Weight;   // Set <= 0.f to deactivate this source
	};

	std::vector<CSourceInfo>         _SourceInfo;
	std::vector<Scene::CSceneNode*>  _Nodes;
	std::vector<Math::CTransformSRT> _Transforms;   // per node per source
	std::vector<U8>                  _ChannelMasks; // per node per source

public:

	CAnimationBlender() = default;
	CAnimationBlender(U8 SourceCount) { Initialize(SourceCount); }

	void               Initialize(U8 SourceCount);
	void               Apply();

	void               SetPriority(U8 Source, U32 Priority) { if (Source < _SourceInfo.size()) _SourceInfo[Source].Priority = Priority; }
	void               SetWeight(U8 Source, float Weight) { if (Source < _SourceInfo.size()) _SourceInfo[Source].Weight = Weight; }

	void               SetScale(U8 Source, U16 Port, const vector3& Scale);
	void               SetRotation(U8 Source, U16 Port, const quaternion& Rotation);
	void               SetTranslation(U8 Source, U16 Port, const vector3& Translation);
	void               SetTransform(U8 Source, U16 Port, const Math::CTransformSRT& Tfm);

	U32                GetOrCreateNodePort(Scene::CSceneNode* pNode);
	Scene::CSceneNode* GetPortNode(U16 Port) const;
};

inline void CAnimationBlender::SetScale(U8 Source, U16 Port, const vector3& Scale)
{
	const UPTR Idx = Port * _SourceInfo.size() + Source;
	_Transforms[Idx].Scale = Scale;
	_ChannelMasks[Idx] |= Scene::Tfm_Scaling;
}
//---------------------------------------------------------------------

inline void CAnimationBlender::SetRotation(U8 Source, U16 Port, const quaternion& Rotation)
{
	const UPTR Idx = Port * _SourceInfo.size() + Source;
	_Transforms[Idx].Rotation = Rotation;
	_ChannelMasks[Idx] |= Scene::Tfm_Rotation;
}
//---------------------------------------------------------------------

inline void CAnimationBlender::SetTranslation(U8 Source, U16 Port, const vector3& Translation)
{
	const UPTR Idx = Port * _SourceInfo.size() + Source;
	_Transforms[Idx].Translation = Translation;
	_ChannelMasks[Idx] |= Scene::Tfm_Translation;
}
//---------------------------------------------------------------------

inline void CAnimationBlender::SetTransform(U8 Source, U16 Port, const Math::CTransformSRT& Tfm)
{
	const UPTR Idx = Port * _SourceInfo.size() + Source;
	_Transforms[Idx] = Tfm;
	_ChannelMasks[Idx] |= (Scene::Tfm_Scaling | Scene::Tfm_Rotation | Scene::Tfm_Translation);
}
//---------------------------------------------------------------------

inline Scene::CSceneNode* CAnimationBlender::GetPortNode(U16 Port) const
{
	return (Port < _Nodes.size()) ? _Nodes[Port] : nullptr;
}
//---------------------------------------------------------------------

}
