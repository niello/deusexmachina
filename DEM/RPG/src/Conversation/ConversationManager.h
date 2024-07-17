#pragma once
#include <Core/RTTIBaseClass.h>
#include <Game/ECS/Entity.h>
#include <Events/Signal.h>

// Conversation manager controls all talking logic, both Player-NPC and NPC-NPC

namespace DEM::Game
{
	class CGameSession;
}

namespace DEM::Flow
{
	class CFlowPlayer;
}

namespace DEM::RPG
{
using PConversation = std::unique_ptr<struct CConversation>;

enum class EConversationMode
{
	Foreground,	// Main UI window, single instance
	Background,	// Phrases above characters' heads, multiple instances
	Auto		// Decide from participant factions and presence of choices in a flow asset
};

class CConversationManager : public ::Core::CRTTIBaseClass
{
	RTTI_CLASS_DECL(CConversationManager, ::Core::CRTTIBaseClass);

protected:

	Game::CGameSession&                    _Session;
	std::map<Game::HEntity, PConversation> _Conversations; // Indexed by target (asset owner)
	std::map<Game::HEntity, Game::HEntity> _BusyActors;    // Entity -> Conversation key, for quick "is talking" check
	Game::HEntity                          _ForegroundConversation;

public:

	CConversationManager(Game::CGameSession& Owner);

	bool                StartConversation(Game::HEntity Initiator, Game::HEntity Target, EConversationMode Mode = EConversationMode::Auto);
	void                CancelConversation(Game::HEntity Key);
	bool                SetConversationMode(Game::HEntity Key, EConversationMode Mode);
	bool                RegisterParticipant(Game::HEntity Key, Game::HEntity Actor);
	bool                UnregisterParticipant(Game::HEntity Key, Game::HEntity Actor);
	size_t              GetParticipantCount(Game::HEntity Key) const;
	Game::HEntity       GetConversationKey(Game::HEntity Participant) const;
	Game::HEntity       GetForegroundConversationKey() const { return _ForegroundConversation; }
	size_t              GetActiveConversationCount() const { return _Conversations.size(); }

	void                Update(float dt);

	Events::CConnection SayPhrase(Game::HEntity Actor, std::string&& Text, float Time = -1.f, std::function<void()>&& OnEnd = nullptr);
	Events::CConnection ProvideChoices(Game::HEntity Actor, std::vector<std::string>&& Texts, std::function<void(size_t)>&& OnChoose);
};

}
