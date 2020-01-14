#pragma once
#include <Animation/AnimationBlender.h>
#include <Scene/SceneNode.h>

// Transformation source for scene node animation. Supports direct writing and blending.
// One source can't be reused for animating different node hierarchies at the same time.

namespace Scene
{
	typedef Ptr<class CSceneNode> PSceneNode;
}

namespace DEM::Anim
{

class CTransformSource
{
protected:

	struct CBlendInfo
	{
		PAnimationBlender Blender;
		std::vector<U16>  Ports;
		U8                SourceIndex;
	};

	std::vector<Scene::PSceneNode> _Nodes;
	std::unique_ptr<CBlendInfo>    _BlendInfo; // If empty, direct writing will be performed

public:

	void SetBlending(PAnimationBlender Blender, U8 SourceIndex);

	void SetScale(UPTR Index, const vector3& Scale);
	void SetRotation(UPTR Index, const quaternion& Rotation);
	void SetTranslation(UPTR Index, const vector3& Translation);
	void SetTransform(UPTR Index, const Math::CTransformSRT& Tfm);

	UPTR GetTransformCount() const { return _BlendInfo ? _BlendInfo->Ports.size() : _Nodes.size(); }
	bool IsNodeActive(UPTR Index) const;
};

inline void CTransformSource::SetScale(UPTR Index, const vector3& Scale)
{
	if (_BlendInfo)
		_BlendInfo->Blender->SetScale(_BlendInfo->SourceIndex, _BlendInfo->Ports[Index], Scale);
	else
		_Nodes[Index]->SetLocalScale(Scale);
}
//---------------------------------------------------------------------

inline void CTransformSource::SetRotation(UPTR Index, const quaternion& Rotation)
{
	if (_BlendInfo)
		_BlendInfo->Blender->SetRotation(_BlendInfo->SourceIndex, _BlendInfo->Ports[Index], Rotation);
	else
		_Nodes[Index]->SetLocalRotation(Rotation);
}
//---------------------------------------------------------------------

inline void CTransformSource::SetTranslation(UPTR Index, const vector3& Translation)
{
	if (_BlendInfo)
		_BlendInfo->Blender->SetTranslation(_BlendInfo->SourceIndex, _BlendInfo->Ports[Index], Translation);
	else
		_Nodes[Index]->SetLocalPosition(Translation);
}
//---------------------------------------------------------------------

inline void CTransformSource::SetTransform(UPTR Index, const Math::CTransformSRT& Tfm)
{
	if (_BlendInfo)
		_BlendInfo->Blender->SetTransform(_BlendInfo->SourceIndex, _BlendInfo->Ports[Index], Tfm);
	else
		_Nodes[Index]->SetLocalTransform(Tfm);
}
//---------------------------------------------------------------------

inline bool CTransformSource::IsNodeActive(UPTR Index) const
{
	return _BlendInfo ?
		_BlendInfo->Blender->IsNodeActive(Index) :
		(_Nodes[Index] && _Nodes[Index]->IsActive());
}
//---------------------------------------------------------------------

}
