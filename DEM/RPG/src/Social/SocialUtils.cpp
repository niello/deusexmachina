#include "SocialUtils.h"
#include <Social/SocialManager.h>
#include <Character/SocialComponent.h>
#include <Game/GameSession.h>
#include <Game/ECS/GameWorld.h>

namespace DEM::RPG
{

float GetDisposition(Game::CGameSession& Session, Game::HEntity NPCID)
{
	auto pWorld = Session.FindFeature<Game::CGameWorld>();
	if (!pWorld) return 0.f;

	auto pSocialMgr = Session.FindFeature<CSocialManager>();
	if (!pSocialMgr) return 0.f;

	auto pNPCSocial = pWorld->FindComponent<const CSocialComponent>(NPCID);
	if (!pNPCSocial) return 0.f;

	//???!!!calculate disposition here or use CModifiableValue?!

	float Disposition = pNPCSocial->Disposition;// .GetBaseValue();
	//for (const auto [TraitID, Coeff] : pNPCSocial->TraitDispositonCoeffs)
	//	Disposition += pSocialMgr->GetPartyTrait(TraitID) * Coeff;

	// collect faction coeffs from factions if not overridden
	// apply faction reputations, including constant for being a member

	return Disposition;
}
//---------------------------------------------------------------------

}
