#include "ConversationManager.h"
#include <Scripting/Flow/FlowPlayer.h>

namespace DEM::RPG
{

struct CConversation
{
	Flow::CFlowPlayer       Player;
	std::set<Game::HEntity> Participants; //???store owner (target) and initiator separately?
	// TODO: persistent data, e.g. visited phrases. Load here or keep in a separate collection in a manager?
};

CConversationManager::CConversationManager(Game::CGameSession& Owner)
	: _Session(Owner)
{
}
//---------------------------------------------------------------------

void CConversationManager::StartConversation(Game::HEntity Initiator, Game::HEntity Target, EConversationMode Mode)
{
	NOT_IMPLEMENTED;
}
//---------------------------------------------------------------------

void CConversationManager::CancelConversation(Game::HEntity Key)
{
	NOT_IMPLEMENTED;
}
//---------------------------------------------------------------------

bool CConversationManager::SetConversationMode(Game::HEntity Key, EConversationMode Mode)
{
	NOT_IMPLEMENTED;
	return false;
}
//---------------------------------------------------------------------

size_t CConversationManager::GetParticipantCount(Game::HEntity Key) const
{
	auto It = _Conversations.find(Key);
	return (It == _Conversations.cend()) ? 0 : It->second->Participants.size();
}
//---------------------------------------------------------------------

Game::HEntity CConversationManager::GetConversationKey(Game::HEntity Participant) const
{
	auto It = _BusyActors.find(Participant);
	return (It == _BusyActors.cend()) ? Game::HEntity{} : It->second;
}
//---------------------------------------------------------------------

}
