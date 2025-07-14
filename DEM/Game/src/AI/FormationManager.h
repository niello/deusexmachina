#pragma once
#include <Core/RTTIBaseClass.h>
#include <Game/ECS/Entity.h>
#include <rtm/vector4f.h>

// Controls positioning of the group of movable objects in the world

namespace DEM::AI
{
	class CCommandFuture;
}

namespace DEM::Game
{
class CGameSession;

class CFormationManager : public DEM::Core::CRTTIBaseClass
{
	RTTI_CLASS_DECL(CFormationManager, DEM::Core::CRTTIBaseClass);

protected:

	CGameSession& _Session;
	// formation set
	// current formation
	// other settings
	// navigation mesh reference for dest selection? or pass as an argument?

public:

	CFormationManager(CGameSession& Owner);

	bool Move(rtm::vector4f_arg0 WorldPosition, rtm::vector4f_arg1 Direction, const std::vector<HEntity>& Entities, std::vector<AI::CCommandFuture>* pOutCommands, bool Enqueue) const;
};

}
