#pragma once
#ifndef __DEM_L2_ENV_QUERY_MANAGER_H__
#define __DEM_L2_ENV_QUERY_MANAGER_H__

#include <Game/Manager.h>
#include <Game/Entity.h>
#include <Events/Events.h>
#include <mathlib/rectangle.h>

// The EnvQueryManager implements environment queries into the game world,
// like stabbing queries, line-of-sight checks, etc...
// Based on mangalore EnvQueryManager_(C) 2005 Radon Labs GmbH

namespace Physics
{
	typedef int CMaterialType;
}

struct CEnvInfo
{
	//float					TerrainHeight;
	float					WorldHeight;
	Physics::CMaterialType	Material;
	//???where to ignore dynamic/AI objects, where not to?
	//???how to check multilevel ground (bridge above a road etc)?
	//???how to check is point inside world geom?
};

namespace Game
{
#define EnvQueryMgr Game::CEnvQueryManager::Instance()

class CEnvQueryManager: public CManager
{
	DeclareRTTI;
	DeclareFactory(CEnvQueryManager);
	__DeclareSingleton(CEnvQueryManager);

protected:

	CStrID	EntityUnderMouse;
	vector3	MousePos3D;
	vector3	UpVector;
	bool	MouseIntersection;

	DECLARE_EVENT_HANDLER(OnFrame, OnFrame);

public:

	CEnvQueryManager();
	virtual ~CEnvQueryManager();

	virtual void	Activate();
	virtual void	Deactivate();

	CEntity*		GetEntityUnderMouse() const;
	void			GetEntitiesUnderMouseDragDropRect(const rectangle& dragDropRect, nArray<Ptr<Game::CEntity> >& entities);
	nArray<PEntity>	GetEntitiesInSphere(const vector3& midPoint, float radius);
	nArray<PEntity>	GetEntitiesInBox(const vector3& scale, const matrix44& m);
	vector2			GetEntityScreenPositionUpper(const Game::CEntity& Entity);
	vector2			GetEntityScreenPositionRel(const Game::CEntity& Entity, const vector3* Offset = NULL);
	rectangle		GetEntityScreenRectangle(const Game::CEntity& Entity, const vector3* const Offset = NULL);

	bool			GetEnvInfoAt(const vector3& Position, CEnvInfo& Info, float ProbeLength = 1000.f, int SelfPhysicsID = -1) const;

	bool			HasMouseIntersection() const { return MouseIntersection; }
	const vector3&	GetMousePos3D() const { return MousePos3D; }
	const vector3&	GetUpVector() const { return UpVector; }
};

RegisterFactory(CEnvQueryManager);

}

#endif
