#include <Game/GameSession.h>
//#include <Game/ECS/GameWorld.h>
#include <AI/Perception.h>

namespace DEM::RPG
{

AI::EAwareness SenseVisualStimulus(Game::CGameSession& Session, Game::HEntity SensorID, Game::HEntity StimulusID, float Modifier)
{
	// TODO: process

	// get visibile component and use visibility coeff/state/modifier from there

	// here is a detection check for invisible and hidden objects/characters against the sensor

	// choose an awareness level based on Modifier and detection

	// can apply sensor cheats and buffs here

	return AI::EAwareness::Full;
}
//---------------------------------------------------------------------

}
