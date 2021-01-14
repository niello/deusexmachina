#pragma once
#include <Resources/Resource.h>
#include <Resources/ResourceManager.h>
#include <Game/ECS/Entity.h>
#include <Data/Params.h>

// Timeline task is a description enough for the timeline player to play
// a timeline asset and to output results into appropriate receivers

namespace DEM::Game
{
	class CGameWorld;
}

namespace DEM::Anim
{
class CTimelinePlayer;

struct CTimelineTask
{
	Resources::PResource Timeline;
	Data::CParams        OutputDescs;
	float                Speed = 1.f;
	float                StartTime = 0.f;
	float                EndTime = 1.f; //???use absolute or relative time here?
	U32                  LoopCount = 0;
};

void InitTimelineTask(CTimelineTask& Task, const Data::CParams& Desc, Resources::CResourceManager& ResMgr);
bool LoadTimelineTaskIntoPlayer(CTimelinePlayer& Player, Game::CGameWorld& World, Game::HEntity Owner, const CTimelineTask& Task, const CTimelineTask* pPrevTask = nullptr);

}
