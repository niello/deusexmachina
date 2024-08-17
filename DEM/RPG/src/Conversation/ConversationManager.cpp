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
	Flow::CFlowPlayer       Player;
	std::set<Game::HEntity> Participants; //???FIXME: need this or iterate _BusyActors?!
	// TODO: persistent data, e.g. visited phrases. Load here or keep in a separate collection in a manager?
	// Can also store in persistent data last checkpoint action ID when conv was interrupted! To start not from beginning!
};

bool CanSpeak(const Game::CGameWorld& World, Game::HEntity EntityID)
{
	// The entity must be talking, even if it has no conversation asset assigned.
	// This also filters out empty and expired entity IDs.
	auto* pTalking = World.FindComponent<const CTalkingComponent>(EntityID);
	if (!pTalking) return false;

	// If this is the character (and not something like a talking door) check if it is not mute.
	// NB: death or unconsciousness will disable Talk capability, they souldn't be checked directly.
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
	if (!pTalking || !pTalking->Asset) return false;

	auto* pFlow = pTalking->Asset->ValidateObject<Flow::CFlowAsset>();
	return StartConversation(pFlow, Initiator, Target, Mode);
}
//---------------------------------------------------------------------

bool CConversationManager::StartConversation(Flow::PFlowAsset Asset, Game::HEntity Initiator, Game::HEntity Target, EConversationMode Mode)
{
	if (!Asset) return false;

	if (Mode == EConversationMode::Auto)
	{
		// Conversations with choice actions must be foreground
		static const CStrID sidChoiceActionClass("DEM::RPG::CChoiceAction");
		if (Asset->HasAction(sidChoiceActionClass))
			Mode = EConversationMode::Foreground;
		else
			Mode = EConversationMode::Background;
	}

	// Can't have more than one foreground conversation running
	if (Mode == EConversationMode::Foreground && _ForegroundConversation) return false;

	PConversation New(new CConversation());

	if (!New->Player.Start(std::move(Asset))) return false;

	New->Player.GetVars().Set<int>(sidConversationInitiator, Initiator.Raw);
	New->Player.GetVars().Set<int>(sidConversationOwner, Target.Raw);

	_Conversations.emplace(Target, std::move(New));

	// Register first two mandatory participants, target first because it owns the conversation.
	// Failure to engage one will automatically cancel the conversation.
	if (!EngageParticipant(Target, Target, true)) return false;
	if (Initiator && Initiator != Target && !EngageParticipant(Target, Initiator, true)) return false;

	if (Mode == EConversationMode::Foreground)
		_ForegroundConversation = Target;

	return true;
}
//---------------------------------------------------------------------

std::map<Game::HEntity, PConversation>::iterator CConversationManager::CleanupConversation(std::map<Game::HEntity, PConversation>::iterator It)
{
	if (It == _Conversations.cend()) return It;

	const bool IsForeground = (It->first == _ForegroundConversation);

	if (_View) _View->OnConversationEnd(IsForeground);

	for (Game::HEntity Participant : It->second->Participants)
		_Actors.erase(Participant);

	if (IsForeground) _ForegroundConversation = {};

	// Protect conversations from being deleted from inside their flow script
	if (It->second->Player.IsPlaying())
	{
		_TerminatingConversations.emplace_back(std::move(It->second));
		return It;
	}

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
	if (Mode == EConversationMode::Auto) return false;

	if (Mode == EConversationMode::Foreground)
	{
		if (_ForegroundConversation) return (_ForegroundConversation == Key);
		_ForegroundConversation = Key;
	}
	else
	{
		if (_ForegroundConversation == Key)
		{
			_ForegroundConversation = {};
			if (_View) _View->OnConversationEnd(true); // FIXME: better name/API? Closes FG UI.
		}
	}

	return true;
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
	auto ItActor = _Actors.find(Actor);
	if (ItActor != _Actors.cend())
	{
		const auto CurrKey = ItActor->second.ConversationKey;
		if (CurrKey == Key)
		{
			// Engaged into the same conversation, update role
			ItActor->second.Mandatory = Mandatory;
			return true;
		}
		else
		{
			// Engaged into another conversation. Mandatory prevails over optional, otherwise fail a request.
			if (Mandatory && !ItActor->second.Mandatory)
				DisengageParticipant(CurrKey, Actor);
			else
				return false;
		}
	}

	ItConv->second->Participants.emplace(Actor);
	_Actors.emplace(Actor, CActorInfo{ Key, Mandatory });

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
	auto ItActor = _Actors.find(Actor);
	if (ItActor == _Actors.cend() || ItActor->second.ConversationKey != Key) return false;

	const bool WasMandatory = ItActor->second.Mandatory;

	_Actors.erase(ItActor);

	auto ItConv = _Conversations.find(Key);
	if (ItConv != _Conversations.cend())
		ItConv->second->Participants.erase(Actor);

	if (WasMandatory) CancelConversation(Key);

	return true;
}
//---------------------------------------------------------------------

bool CConversationManager::DisengageParticipant(Game::HEntity Actor)
{
	return DisengageParticipant(GetConversationKey(Actor), Actor);
}
//---------------------------------------------------------------------

size_t CConversationManager::GetParticipantCount(Game::HEntity Key) const
{
	auto It = _Conversations.find(Key);
	return (It == _Conversations.cend()) ? 0 : It->second->Participants.size();
}
//---------------------------------------------------------------------

bool CConversationManager::IsParticipantMandatory(Game::HEntity Participant) const
{
	auto It = _Actors.find(Participant);
	return It != _Actors.cend() && It->second.Mandatory;
}
//---------------------------------------------------------------------

Game::HEntity CConversationManager::GetConversationKey(Game::HEntity Participant) const
{
	auto It = _Actors.find(Participant);
	return (It == _Actors.cend()) ? Game::HEntity{} : It->second.ConversationKey;
}
//---------------------------------------------------------------------

void CConversationManager::Update(float dt)
{
	for (auto It = _Conversations.begin(); It != _Conversations.end(); /**/)
	{
		if (It->second)
			It->second->Player.Update(_Session, dt);

		if (!It->second)
			It = _Conversations.erase(It); // Conversation was terminated from inside itself or externally
		else if (It->second->Player.IsPlaying())
			++It;
		else
			It = CleanupConversation(It); // Conversation has ended
	}

	// Now it is safe because flow player updates are finished
	_TerminatingConversations.clear();

	if (_View) _View->Update(dt);
}
//---------------------------------------------------------------------

Events::CConnection CConversationManager::SayPhrase(Game::HEntity Actor, std::string&& Text, float Time, std::function<void(bool)>&& OnEnd)
{
	Events::CConnection Conn;

	auto ItActor = _Actors.find(Actor);
	if (ItActor == _Actors.cend())
	{
		// If the actor was mandatory, the conversation would cancel before reaching here. This means that
		// an actor is either optional or not engaged at all. In both cases we can silently skip.
		OnEnd(true);
		return Conn;
	}

	auto* pWorld = _Session.FindFeature<Game::CGameWorld>();
	if (!pWorld || !CanSpeak(*pWorld, Actor))
	{
		//???or instead of bool use the same callback signature and same error codes as in ProvideChoices?!
		OnEnd(!ItActor->second.Mandatory);

		return Conn;
	}

	if (Time < 0.f)
	{
		//!!!TODO: calc recommended phrase duration! Can use VO duration, number of vowels in a text etc. Or do it in view?
		Time = 3.f;
	}

	if (_View)
		Conn = _View->SayPhrase(Actor, std::move(Text), InInForegroundConversation(Actor), Time, std::move(OnEnd));
	else
		OnEnd(true);

	return Conn;
}
//---------------------------------------------------------------------

Events::CConnection CConversationManager::ProvideChoices(std::vector<std::string>&& Texts, std::vector<bool>&& ValidFlags, std::function<void(size_t)>&& OnChoose)
{
	Events::CConnection Conn;

	if (_View && !Texts.empty())
		Conn = _View->ProvideChoices(std::move(Texts), std::move(ValidFlags), std::move(OnChoose));
	else
		OnChoose(Texts.size());

	return Conn;
}
//---------------------------------------------------------------------

}
