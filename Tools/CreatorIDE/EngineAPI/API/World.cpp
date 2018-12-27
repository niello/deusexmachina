#include <StdAPI.h>
#include <Game/GameServer.h>

// Time, physics etc control

API void World_TogglePause()
{
	GameSrv->ToggleGamePause();
}
//---------------------------------------------------------------------

API void World_SetPause(bool Pause)
{
	GameSrv->PauseGame(Pause);
}
//---------------------------------------------------------------------
