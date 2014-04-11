#pragma once
#ifndef __DEM_L2_GAME_STATIC_OBJ_H__
#define __DEM_L2_GAME_STATIC_OBJ_H__

#include <Core/RefCounted.h>
#include <Data/StringID.h>

// Simplified entity, that represents a piece of static level geometry with
// optional graphics and collision shapes. It can be referenced by UID too.

namespace Data
{
	typedef Ptr<class CParams> PParams;
}

namespace Physics
{
	typedef Ptr<class CCollisionObjStatic> PCollisionObjStatic;
}

namespace Scene
{
	typedef Ptr<class CSceneNode> PSceneNode;
}

namespace Game
{
typedef Ptr<class CGameLevel> PGameLevel;

class CStaticObject: public Core::CRefCounted
{
	__DeclareClassNoFactory;

protected:

	friend class CStaticEnvManager;

	CStrID							UID;
	PGameLevel						Level;
	Data::PParams					Desc;

	Scene::PSceneNode				Node;
	bool							ExistingNode;
	Physics::PCollisionObjStatic	CollObj;

	void SetUID(CStrID NewUID);
	//???!!!void SetLevel(CGameLevel* NewLevel); - for transitions!

	CStaticObject(CStrID _UID, CGameLevel& _Level);

public:

	~CStaticObject();

	void				Init(const Data::CParams& ObjDesc);
	void				Term();
	bool				IsValid() const { return Desc.IsValid(); }

	void				SetTransform(const matrix44& Tfm);

	CStrID				GetUID() const { n_assert_dbg(UID.IsValid()); return UID; }
	CGameLevel&			GetLevel() const { return *Level.GetUnsafe(); }
	Scene::CSceneNode*	GetNode() const { return Node.GetUnsafe(); }
};

typedef Ptr<CStaticObject> PStaticObject;

}

#endif
