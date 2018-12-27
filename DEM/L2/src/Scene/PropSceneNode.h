#pragma once
#ifndef __DEM_L2_PROP_SCENE_NODE_H__
#define __DEM_L2_PROP_SCENE_NODE_H__

#include <Game/Property.h>
#include <Scene/SceneNode.h>
#include <Math/AABB.h>

// Scene node property associates entity transform with scene graph node

namespace Prop
{

class CPropSceneNode: public Game::CProperty
{
	__DeclareClass(CPropSceneNode);
	__DeclarePropertyStorage;

protected:

	Scene::PSceneNode					Node;
	bool								ExistingNode;
	CDict<CStrID, Scene::CSceneNode*>	ChildCache;
	CArray<CStrID>						ChildrenToSave;

	virtual bool	InternalActivate();
	virtual void	InternalDeactivate();
	virtual void	EnableSI(class CPropScriptable& Prop);
	virtual void	DisableSI(class CPropScriptable& Prop);
	void			FillSaveLoadList(Scene::CSceneNode* pNode, const char* pPath);

	DECLARE_EVENT_HANDLER(OnPropActivated, OnPropActivated);
	DECLARE_EVENT_HANDLER(OnPropDeactivating, OnPropDeactivating);
	DECLARE_EVENT_HANDLER(OnLevelSaving, OnLevelSaving);
	DECLARE_EVENT_HANDLER(SetTransform, OnSetTransform);
	DECLARE_EVENT_HANDLER(AfterTransforms, AfterTransforms);
	DECLARE_EVENT_HANDLER_VIRTUAL(OnRenderDebug, OnRenderDebug);

	virtual void	SetTransform(const matrix44& NewTfm);

public:

	enum
	{
		AABB_Gfx	= 0x01,
		AABB_Phys	= 0x02
	};

	virtual bool		Initialize();

	Scene::CSceneNode*	GetNode() const { return Node.GetUnsafe(); }
	Scene::CSceneNode*	GetChildNode(CStrID ID);
	void				GetAABB(CAABB& OutBox, UPTR TypeFlags = AABB_Gfx | AABB_Phys) const;
};

inline Scene::CSceneNode* CPropSceneNode::GetChildNode(CStrID ID)
{
	if (!ID.IsValid()) return Node.GetUnsafe();
	if (Node.IsNullPtr()) return NULL;

	IPTR NodeIdx = ChildCache.FindIndex(ID);
	if (NodeIdx == INVALID_INDEX)
	{
		Scene::CSceneNode* pNode = Node->GetChild(ID.CStr());
		if (pNode) ChildCache.Add(ID, pNode);
		return pNode;
	}
	else return ChildCache.ValueAt(NodeIdx);
}
//---------------------------------------------------------------------

}

#endif
