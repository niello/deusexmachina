#pragma once
#include <Core/RTTIBaseClass.h>
#include <Game/ECS/Entity.h>
#include <rtm/vector4f.h>

// Controls positioning of the group of movable objects in the world

namespace DEM::Game
{
class CGameSession;

class CFormationManager : public ::Core::CRTTIBaseClass
{
	RTTI_CLASS_DECL(CFormationManager, ::Core::CRTTIBaseClass);

protected:

	CGameSession& _Session;
	// formation set
	// current formation
	// other settings
	// navigation mesh reference for dest selection? or pass as an argument?

public:

	CFormationManager(CGameSession& Owner);

	bool Move(std::vector<HEntity> Entities, const rtm::vector4f& WorldPosition, const rtm::vector4f& Direction, bool Enqueue) const;
};

}
