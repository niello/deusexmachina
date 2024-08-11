#pragma once
#include <Core/RTTIBaseClass.h>
#include <Game/ECS/Entity.h>
#include <Events/Signal.h>

// Conversation manager controls all talking logic, both Player-NPC and NPC-NPC

namespace DEM::Game
{
	class CGameSession;
	class CGameWorld;
}

namespace DEM::Flow
{
	class CFlowPlayer;
}

namespace DEM::RPG
{
using PConversation = std::unique_ptr<struct CConversation>;
using PConversationView = std::unique_ptr<class IConversationView>;

bool CanSpeak(const Game::CGameWorld& World, Game::HEntity EntityID);

enum class EConversationMode
{
	Foreground,	// Main UI window, single instance
	Background,	// Phrases above characters' heads, multiple instances
	Auto		// Decide from participant factions and presence of choices in a flow asset
};

class IConversationView
{
public:

	virtual ~IConversationView() = default;

	virtual void                Update(float dt) = 0;
	virtual void                OnConversationEnd(bool Foreground) = 0;
	virtual Events::CConnection SayPhrase(Game::HEntity Actor, std::string&& Text, bool Foreground, float Time, std::function<void(bool)>&& OnEnd) = 0;
	virtual Events::CConnection ProvideChoices(Game::HEntity Actor, std::vector<std::string>&& Texts, std::function<void(size_t)>&& OnChoose) = 0;
};

class CConversationManager : public ::Core::CRTTIBaseClass
{
	RTTI_CLASS_DECL(CConversationManager, ::Core::CRTTIBaseClass);

protected:

	Game::CGameSession&                    _Session;
	std::map<Game::HEntity, PConversation> _Conversations; // Indexed by target (asset owner)
	std::map<Game::HEntity, Game::HEntity> _BusyActors;    // Entity -> Conversation key, for quick "is talking" check
	Game::HEntity                          _ForegroundConversation;
	PConversationView                      _View;

	bool                                             EngageParticipantInternal(Game::HEntity Key, Game::HEntity Actor, bool Mandatory);
	std::map<Game::HEntity, PConversation>::iterator CleanupConversation(std::map<Game::HEntity, PConversation>::iterator It);

public:

	CConversationManager(Game::CGameSession& Owner, PConversationView&& View = nullptr);

	void                SetView(PConversationView&& View) { _View = std::move(View); }

	bool                StartConversation(Game::HEntity Initiator, Game::HEntity Target, EConversationMode Mode = EConversationMode::Auto);
	void                CancelConversation(Game::HEntity Key);
	bool                SetConversationMode(Game::HEntity Key, EConversationMode Mode);
	bool                EngageParticipant(Game::HEntity Key, Game::HEntity Actor, bool Mandatory);
	bool                DisengageParticipant(Game::HEntity Key, Game::HEntity Actor);
	size_t              GetParticipantCount(Game::HEntity Key) const;
	Game::HEntity       GetConversationKey(Game::HEntity Participant) const;
	Game::HEntity       GetForegroundConversationKey() const { return _ForegroundConversation; }
	bool                InInForegroundConversation(Game::HEntity Participant) const { return _ForegroundConversation && GetConversationKey(Participant) == _ForegroundConversation; }
	size_t              GetActiveConversationCount() const { return _Conversations.size(); }

	void                Update(float dt);

	Events::CConnection SayPhrase(Game::HEntity Actor, std::string&& Text, float Time = -1.f, std::function<void(bool)>&& OnEnd = nullptr);
	Events::CConnection ProvideChoices(Game::HEntity Actor, std::vector<std::string>&& Texts, std::function<void(size_t)>&& OnChoose);
	IConversationView*  GetView() const { return _View.get(); }
};

}
