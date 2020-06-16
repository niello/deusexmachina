#pragma once
#include <Core/RTTIBaseClass.h>
#include <Game/ECS/Entity.h>
#include <Math/Vector3.h>

// Controls positioning of the group of movable objects in the world

namespace DEM::Game
{
class CGameSession;

class CFormationManager : public ::Core::CRTTIBaseClass
{
	RTTI_CLASS_DECL;

protected:

	CGameSession& _Owner;
	// formation set
	// current formation
	// other settings
	// navigation mesh reference for dest selection? or pass as an argument?

public:

	CFormationManager(CGameSession& Owner);

	bool Move(std::vector<HEntity> Entities, const vector3& WorldPosition, const vector3& Direction, bool Enqueue) const;
};

}
