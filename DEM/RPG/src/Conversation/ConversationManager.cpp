#include "ConversationManager.h"
#include <Game/GameSession.h>
#include <Game/ECS/GameWorld.h>
#include <Conversation/TalkingComponent.h>
#include <Scripting/Flow/FlowPlayer.h>
#include <Scripting/Flow/FlowAsset.h>

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

bool CConversationManager::StartConversation(Game::HEntity Initiator, Game::HEntity Target, EConversationMode Mode)
{
	auto* pWorld = _Session.FindFeature<Game::CGameWorld>();
	if (!pWorld) return false;

	auto* pTalking = pWorld->FindComponent<CTalkingComponent>(Target);
	if (!pTalking) return false;

	auto* pFlow = pTalking->Asset->ValidateObject<Flow::CFlowAsset>();
	if (!pFlow) return false;

	if (Mode == EConversationMode::Auto)
	{
		// Conversations with choice actions must be foreground
		static const CStrID sidChoiceActionClass("DEM::RPG::CAnswerChoiceAction");
		if (pFlow->HasAction(sidChoiceActionClass))
			Mode = EConversationMode::Foreground;
		else
			Mode = EConversationMode::Background;
	}

	if (Mode == EConversationMode::Foreground)
	{
		// Can't have more than one foreground conversation running
		if (_ForegroundConversation) return false;

		_ForegroundConversation = Target;
	}

	PConversation New(new CConversation);

	//!!!TODO: can write better?! Maybe provide callback into Player.Start directly? or separate SetAsset and Start?!
	auto FillVarsConn = New->Player.OnStart.Subscribe([pNew = New.get(), Initiator, Target]()
	{
		static const CStrID sidConversationInitiator("ConversationInitiator");
		static const CStrID sidConversationOwner("ConversationOwner");
		pNew->Player.GetVars().Set<int>(sidConversationInitiator, Initiator.Raw);
		pNew->Player.GetVars().Set<int>(sidConversationOwner, Target.Raw);
	});

	if (!New->Player.Start(pFlow)) return false;

	_Conversations.emplace(Target, std::move(New));

	// Register first two mandatory participants
	if (!RegisterParticipant(Target, Initiator) || !RegisterParticipant(Target, Target))
	{
		CancelConversation(Target);
		return false;
	}

	return true;
}
//---------------------------------------------------------------------

std::map<Game::HEntity, PConversation>::iterator CConversationManager::CleanupConversation(std::map<Game::HEntity, PConversation>::iterator It)
{
	if (It == _Conversations.cend()) return It;

	for (auto ItActor = _BusyActors.begin(); ItActor != _BusyActors.end(); /**/)
	{
		if (ItActor->second == It->first)
			ItActor = _BusyActors.erase(ItActor);
		else
			++ItActor;
	}

	if (It->first == _ForegroundConversation)
		_ForegroundConversation = {};

	return _Conversations.erase(It);
}
//---------------------------------------------------------------------

void CConversationManager::CancelConversation(Game::HEntity Key)
{
	CleanupConversation(_Conversations.find(Key));
}
//---------------------------------------------------------------------

bool CConversationManager::SetConversationMode(Game::HEntity Key, EConversationMode Mode)
{
	NOT_IMPLEMENTED;
	return false;
}
//---------------------------------------------------------------------

bool CConversationManager::RegisterParticipant(Game::HEntity Key, Game::HEntity Actor)
{
	// Check if the actor is already busy in this or another conversation
	auto ItActor = _BusyActors.find(Actor);
	if (ItActor != _BusyActors.cend())
		return (ItActor->second == Key);

	// Check if the requested conversation exists
	auto It = _Conversations.find(Key);
	if (It == _Conversations.cend()) return false;

	_BusyActors.emplace(Actor, Key);
	It->second->Participants.insert(Actor);
	return true;
}
//---------------------------------------------------------------------

bool CConversationManager::UnregisterParticipant(Game::HEntity Key, Game::HEntity Actor)
{
	auto ItActor = _BusyActors.find(Actor);
	if (ItActor == _BusyActors.cend() || ItActor->second != Key) return false;

	_BusyActors.erase(ItActor);

	auto It = _Conversations.find(Key);
	if (It != _Conversations.cend())
		It->second->Participants.erase(Actor);

	return true;
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

void CConversationManager::Update(float dt)
{
	for (auto It = _Conversations.begin(); It != _Conversations.end(); /**/)
	{
		It->second->Player.Update(dt);

		if (It->second->Player.IsPlaying())
			++It;
		else
			It = CleanupConversation(It);
	}
}
//---------------------------------------------------------------------

}
