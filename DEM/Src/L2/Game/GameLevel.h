#pragma once
#ifndef __DEM_L2_GAME_LEVEL_H__
#define __DEM_L2_GAME_LEVEL_H__

// Represents one game location, including all entities in it and property worlds (physics, AI, scene)

#include <Core/RefCounted.h>
#include <Scripting/ScriptObject.h> //???fwd decl?

namespace Scene
{
	typedef Ptr<class CScene> PScene;
}

namespace Physics
{
	typedef Ptr<class CPhysicsLevel> PPhysicsLevel;
}

namespace AI
{
	typedef Ptr<class CAILevel> PAILevel;
}

namespace Game
{

class CGameLevel: public Core::CRefCounted
{
protected:

	CStrID						ID;
	nString						Name;
	Scripting::PScriptObject	Script;

	Scene::PScene				Scene;
	Physics::PPhysicsLevel		PhysicsLevel;
	AI::PAILevel				AILevel;

public:

	bool			Init(CStrID LevelID, const Data::CParams& Desc);
	void			Trigger();
	void			RenderScene();
	void			RenderDebug();

	CStrID			GetID() const { return ID; }
	const nString&	GetName() const { return Name; }
};

typedef Ptr<CGameLevel> PGameLevel;

}

#endif
