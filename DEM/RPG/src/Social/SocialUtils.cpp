#include "SocialUtils.h"
#include <Social/SocialManager.h>
#include <Character/SocialComponent.h>
#include <Character/StatsComponent.h>
#include <Game/GameSession.h>
#include <Game/ECS/GameWorld.h>

namespace DEM::RPG
{

// NB: ToCharacter is considered being a party member now! Can make it optional and skip party-only logic for NPC->NPC.
float GetDisposition(Game::CGameSession& Session, Game::HEntity FromCharacter, Game::HEntity ToCharacter)
{
	constexpr float MaxDisposition = 100.f;
	if (FromCharacter == ToCharacter) return MaxDisposition;

	auto pWorld = Session.FindFeature<Game::CGameWorld>();
	if (!pWorld) return 0.f;

	auto pSocialMgr = Session.FindFeature<CSocialManager>();
	if (!pSocialMgr) return 0.f;

	auto pNPCSocial = pWorld->FindComponent<const CSocialComponent>(FromCharacter);
	if (!pNPCSocial) return 0.f;

	//???!!!TODO: calculate disposition here or use CModifiableValue?! or apply temporary
	// modifications to pNPCSocial->Disposition but keep factions and traits here?!
	float Disposition = pNPCSocial->Disposition;// .GetBaseValue();

	// Modify disposition from player character charisma
	constexpr float CharismaCoeff = 2.5f;
	if (auto* pStats = pWorld->FindComponent<const Sh2::CStatsComponent>(ToCharacter))
		Disposition += (pStats->Charisma - 11) * CharismaCoeff;

	// Modify disposition from party personality traits
	for (const auto [TraitID, Coeff] : pNPCSocial->TraitDispositonCoeffs)
		Disposition += pSocialMgr->GetPartyTrait(TraitID) * Coeff;

	// Collect disposition multipliers for faction reputation
	// TODO: could cache effective coeffs per character?! re-cache on faction relation or membership changes.
	//???!!!TODO: or calculate per target faction to avoid allocations and to request only important factions? can iterate them?
	std::map<CStrID, float> FactionCoeffs;

	// First get values from cross-faction relations
	for (const auto NPCFactionID : pNPCSocial->Factions)
	{
		if (auto* pNPCFaction = pSocialMgr->FindFaction(NPCFactionID))
		{
			for (const auto [TargetFactionID, Coeff] : pNPCFaction->Relations)
			{
				if (Coeff == 0.f) continue;

				// Skip if explicitly overridden
				if (pNPCSocial->FactionDispositonCoeffs.find(TargetFactionID) != pNPCSocial->FactionDispositonCoeffs.cend()) continue;

				auto It = FactionCoeffs.find(TargetFactionID);
				if (It == FactionCoeffs.cend())
				{
					FactionCoeffs.emplace(TargetFactionID, Coeff);
				}
				else
				{
					// Choose the greatest magnitude for each faction, on conflict prefer positive
					const auto CurrMagnitude = std::abs(It->second);
					const auto NewMagnitude = std::abs(Coeff);
					if (NewMagnitude > CurrMagnitude || (NewMagnitude == CurrMagnitude && Coeff > It->second))
						It->second = Coeff;
				}
			}
		}
	}

	// Add explicit overrides from NPC data
	for (const auto [TargetFactionID, Coeff] : pNPCSocial->FactionDispositonCoeffs)
		if (Coeff != 0.f)
			FactionCoeffs.emplace(TargetFactionID, Coeff);

	// Add default self-dispositions if not overridden by any settings
	for (const auto NPCFactionID : pNPCSocial->Factions)
		if (FactionCoeffs.find(NPCFactionID) == FactionCoeffs.cend())
			FactionCoeffs.emplace(NPCFactionID, 1.f);

	// Apply disposition from faction reputation
	constexpr float GoodReputationCoeff = 1.f;
	constexpr float BadReputationCoeff = 1.f;
	for (const auto [FactionID, Coeff] : FactionCoeffs)
		if (auto* pFaction = pSocialMgr->FindFaction(FactionID))
			Disposition += (pFaction->GoodReputation * GoodReputationCoeff - pFaction->BadReputation * BadReputationCoeff) * Coeff;

	// Apply disposition from faction membership of a party
	constexpr float FactionMembershipReputation = 25.f;
	for (const auto FactionID : pSocialMgr->GetPartyFactions())
	{
		auto It = FactionCoeffs.find(FactionID);
		if (It != FactionCoeffs.cend())
			Disposition += FactionMembershipReputation * GoodReputationCoeff * It->second;
	}

	return std::clamp(Disposition, -MaxDisposition, MaxDisposition);
}
//---------------------------------------------------------------------

}
