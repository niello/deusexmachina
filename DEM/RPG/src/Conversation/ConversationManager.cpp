#include "ConversationManager.h"
#include <Game/GameSession.h>
#include <Game/ECS/GameWorld.h>
#include <Conversation/TalkingComponent.h>
#include <Character/StatsComponent.h>
#include <Scripting/Flow/FlowPlayer.h>
#include <Scripting/Flow/FlowAsset.h>

namespace DEM::RPG
{
static const CStrID sidConversationInitiator("ConversationInitiator");
static const CStrID sidConversationOwner("ConversationOwner");

struct CConversation
{
	Flow::CFlowPlayer             Player;
	std::map<Game::HEntity, bool> Participants; // Actor -> Is mandatory
	// TODO: persistent data, e.g. visited phrases. Load here or keep in a separate collection in a manager?
	// Can also store in persistent data last checkpoint action ID when conv was interrupted! To start not from beginning!
};

bool CanSpeak(const Game::CGameWorld& World, Game::HEntity EntityID)
{
	// The entity must be talking, even if it has no conversation asset assigned.
	// This also filters out empty and expired entity IDs.
	auto* pTalking = World.FindComponent<const CTalkingComponent>(EntityID);
	if (!pTalking) return false;

	// If this is the character (and not something like a talking door) check if it is not mute
	if (auto* pStats = World.FindComponent<const Sh2::CStatsComponent>(EntityID))
		if (!(pStats->Capabilities & Sh2::ECapability::Talk)) return false;

	return true;
}
//---------------------------------------------------------------------

CConversationManager::CConversationManager(Game::CGameSession& Owner, PConversationView&& View)
	: _Session(Owner)
	, _View(std::move(View))
{
}
//---------------------------------------------------------------------

bool CConversationManager::StartConversation(Game::HEntity Initiator, Game::HEntity Target, EConversationMode Mode)
{
	auto* pWorld = _Session.FindFeature<Game::CGameWorld>();
	if (!pWorld) return false;

	auto* pTalking = pWorld->FindComponent<const CTalkingComponent>(Target);
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

	// Can't have more than one foreground conversation running
	if (Mode == EConversationMode::Foreground && _ForegroundConversation) return false;

	PConversation New(new CConversation());

	if (!New->Player.Start(pFlow)) return false;

	New->Player.GetVars().Set<int>(sidConversationInitiator, Initiator.Raw);
	New->Player.GetVars().Set<int>(sidConversationOwner, Target.Raw);

	_Conversations.emplace(Target, std::move(New));

	// Register first two mandatory participants, target first because it owns the conversation.
	// Failure to engage one will automatically cancel the conversation.
	if (!EngageParticipant(Target, Target, true) || (Initiator && !EngageParticipant(Target, Initiator, true)))
		return false;

	if (Mode == EConversationMode::Foreground)
		_ForegroundConversation = Target;

	return true;
}
//---------------------------------------------------------------------

std::map<Game::HEntity, PConversation>::iterator CConversationManager::CleanupConversation(std::map<Game::HEntity, PConversation>::iterator It)
{
	if (It == _Conversations.cend()) return It;

	if (_View) _View->OnConversationEnd(true);

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

bool CConversationManager::EngageParticipantInternal(Game::HEntity Key, Game::HEntity Actor, bool Mandatory)
{
	// Check if the requested conversation exists
	auto ItConv = _Conversations.find(Key);
	if (ItConv == _Conversations.cend()) return false;

	auto* pWorld = _Session.FindFeature<Game::CGameWorld>();
	if (!pWorld) return false;

	// Can engage something unable to speak
	if (!CanSpeak(*pWorld, Actor)) return false;

	// Check if the actor is already engaged
	auto ItActor = _BusyActors.find(Actor);
	if (ItActor != _BusyActors.cend())
	{
		const auto CurrKey = ItActor->second;
		if (CurrKey == Key)
		{
			// Engaged into the same conversation, update role
			ItConv->second->Participants.insert_or_assign(Actor, Mandatory);
			return true;
		}
		else
		{
			// Engaged into another conversation
			auto ItCurrConv = _Conversations.find(CurrKey);
			if (ItCurrConv != _Conversations.cend())
			{
				auto It = ItCurrConv->second->Participants.find(Actor);
				if (It != ItCurrConv->second->Participants.cend())
				{
					// Mandatory prevails over optional, otherwise fail a request
					const bool WasMandatory = It->second;
					if (Mandatory && !WasMandatory)
						DisengageParticipant(CurrKey, Actor);
					else
						return false;
				}
			}
		}
	}

	ItConv->second->Participants.emplace(Actor, Mandatory);
	_BusyActors.emplace(Actor, Key);

	return true;
}
//---------------------------------------------------------------------

bool CConversationManager::EngageParticipant(Game::HEntity Key, Game::HEntity Actor, bool Mandatory)
{
	if (EngageParticipantInternal(Key, Actor, Mandatory)) return true;
	if (Mandatory) CancelConversation(Key);
	return false;
}
//---------------------------------------------------------------------

bool CConversationManager::DisengageParticipant(Game::HEntity Key, Game::HEntity Actor)
{
	auto ItActor = _BusyActors.find(Actor);
	if (ItActor == _BusyActors.cend() || ItActor->second != Key) return false;

	_BusyActors.erase(ItActor);

	auto ItConv = _Conversations.find(Key);
	if (ItConv == _Conversations.cend()) return false;

	auto& Participants = ItConv->second->Participants;
	auto It = Participants.find(Actor);
	if (It == Participants.cend()) return false;

	const bool WasMandatory = It->second;

	Participants.erase(It);

	if (WasMandatory) CancelConversation(Key);

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
		It->second->Player.Update(_Session, dt);

		if (It->second->Player.IsPlaying())
			++It;
		else
			It = CleanupConversation(It);
	}

	if (_View) _View->Update(dt);
}
//---------------------------------------------------------------------

Events::CConnection CConversationManager::SayPhrase(Game::HEntity Actor, std::string&& Text, float Time, std::function<void()>&& OnEnd)
{
	//!!!check actor valid and can speak!
	//???pass error in callback arg?

	if (Time < 0.f)
	{
		//!!!TODO: calc recommended phrase duration! Can use VO duration, number of vowels in a text etc. Or do it in view?
		Time = 3.f;
	}

	Events::CConnection Conn;
	if (_View)
		Conn = _View->SayPhrase(Actor, std::move(Text), InInForegroundConversation(Actor), Time, std::move(OnEnd));
	else
		OnEnd();

	return Conn;
}
//---------------------------------------------------------------------

Events::CConnection CConversationManager::ProvideChoices(Game::HEntity Actor, std::vector<std::string>&& Texts, std::function<void(size_t)>&& OnChoose)
{
	//!!!check actor valid and can speak!
	//???pass error in callback arg or as a special choice value?

	Events::CConnection Conn;
	if (_View && !Texts.empty())
		Conn = _View->ProvideChoices(Actor, std::move(Texts), std::move(OnChoose));
	else
		NOT_IMPLEMENTED; //!!!TODO: call OnChoose with some special value meaning forced cancellation with no choice made!

	return Conn;
}
//---------------------------------------------------------------------

}
