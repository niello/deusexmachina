#include <Game/ECS/GameWorld.h>
#include <Game/GameSession.h>
#include <Items/ItemComponent.h>
#include <Items/ArmorComponent.h>
#include <Items/EquipmentChangesComponent.h>
#include <Items/EquippableComponent.h>
#include <Combat/DestructibleComponent.h>
#include <Character/AppearanceComponent.h>
#include <Character/AppearanceAsset.h>
#include <Scene/SceneComponent.h>

// A set of ECS systems required for functioning of the equipment logic

namespace DEM::RPG
{

//!!!DBG TMP! Must be not here, maybe in Damage.h?
class CArmorAbsorptionModifier : public CParameterModifier<CDamageAbsorption>
{
public:

	Game::CGameWorld& _World;
	Game::HEntity _SourceID;

	CArmorAbsorptionModifier(Game::CGameWorld& World, Game::HEntity SourceID) : _World(World), _SourceID(SourceID) {}

	virtual I32 GetPriority() const override { return 0; }

	virtual bool Apply(CDamageAbsorption& Value) override
	{
		auto pArmor = FindItemComponent<const CArmorComponent>(_World, _SourceID);
		if (!pArmor) return false;
		Value += pArmor->Absorption;
		return true;
	}
};

void InitEquipment(Game::CGameWorld& World, Resources::CResourceManager& ResMgr)
{
	World.ForEachComponent<CEquipmentComponent>([&ResMgr, &World](auto EntityID, CEquipmentComponent& Component)
	{
		// Setup equipment assets
		if (Component.SchemeID)
			Component.Scheme = ResMgr.RegisterResource<CEquipmentScheme>(Component.SchemeID.CStr())->ValidateObject<CEquipmentScheme>();

		// Request application of equipment effects for already equipped stacks
		for (const auto [SlotID, StackID] : Component.Equipment)
		{
			if (auto pEquipped = World.AddComponent<CEquippedComponent>(StackID))
			{
				pEquipped->OwnerID = EntityID;
				ScheduleStackReequipment(World, StackID, EItemStorage::Equipment, FindMainOccupiedSlot(World, Component, StackID));
			}
		}

		// Request application of equipment effects for already equipped stacks in quick slots
		for (size_t i = 0; i < Component.QuickSlots.size(); ++i)
		{
			const auto StackID = Component.QuickSlots[i];
			if (auto pEquipped = World.AddComponent<CEquippedComponent>(StackID))
			{
				pEquipped->OwnerID = EntityID;
				ScheduleStackReequipment(World, StackID, EItemStorage::QuickSlot, GetQuickSlotID(i));
			}
		}
	});
}
//---------------------------------------------------------------------

void RebuildCharacterAppearance(Game::CGameWorld& World, Game::HEntity EntityID, CAppearanceComponent& AppearanceComponent)
{
	std::vector<std::pair<Resources::PResource, std::string>> NewLook;
	std::set<CStrID> FilledBodyParts;

	if (auto pEquipment = FindItemComponent<const CEquipmentComponent>(World, EntityID))
	{
		std::set<Game::HEntity> ProcessedStacks;

		// TODO: process slots in order of priority!
		for (const auto& [SlotID, StackID] : pEquipment->Equipment)
		{
			if (ProcessedStacks.find(StackID) != ProcessedStacks.cend()) continue;

			ProcessedStacks.insert(StackID);

			auto pEquippable = FindItemComponent<const CEquippableComponent>(World, StackID);
			if (!pEquippable) continue;

			std::set<CStrID> LocalBodyParts;
			for (const auto& AppearanceRsrc : pEquippable->AppearanceAssets)
			{
				// TODO: skip ignored assets

				if (auto pAppearanceAsset = AppearanceRsrc->ValidateObject<CAppearanceAsset>())
				{
					for (const auto& VisualPart : pAppearanceAsset->Visuals)
					{
						// Skip if all body parts are already overridden by higher priority assets
						if (!VisualPart.BodyParts.empty() &&
							!std::any_of(VisualPart.BodyParts.cbegin(), VisualPart.BodyParts.cend(),
								[&FilledBodyParts](CStrID BodyPart) { return FilledBodyParts.find(BodyPart) == FilledBodyParts.cend(); }))
						{
							continue;
						}

						// TODO: skip ignored body parts (maybe only overrides, e.g. helmet)

						LocalBodyParts.insert(VisualPart.BodyParts.cbegin(), VisualPart.BodyParts.cend());

						// Apply first matching variant, if any
						for (const auto& Variant : VisualPart.Variants)
						{
							// Check appearance parameter conditions
							if (Variant.Conditions && Variant.Conditions->GetCount())
							{
								if (!AppearanceComponent.Params) continue;

								bool Match = true;
								for (const auto& Condition : *Variant.Conditions)
								{
									auto pParam = AppearanceComponent.Params->Find(Condition.GetName());
									if (!pParam || pParam->GetRawValue() != Condition.GetRawValue())
									{
										Match = false;
										break;
									}
								}

								if (!Match) continue;
							}

							// Match found, remember this scene asset for instantiation
							NewLook.emplace_back(Variant.Asset, VisualPart.RootBonePath);

							//!!!???TODO:
							// detect target bone from SlotID if no explicit path specified?
							// how to handle sheathed and unsheathed weapons? a bool flag in a character appearance component? just as 'hide helmet'
							//!!!can patch empty RootBonePath by SlotID in a postprocessing pass, to reuse main part of the loop for the base look!
						}
					}
				}
			}

			//???or insert directly into FilledBodyParts in a separate loop? Allows reusing the first loop for base look!
			FilledBodyParts.merge(std::move(LocalBodyParts));
		}
	}

	//!!!TODO: REMOVE CODE DUPLICATION!
	for (const auto& AppearanceRsrc : AppearanceComponent.AppearanceAssets)
	{
		// TODO: skip ignored assets

		if (auto pAppearanceAsset = AppearanceRsrc->ValidateObject<CAppearanceAsset>())
		{
			for (const auto& VisualPart : pAppearanceAsset->Visuals)
			{
				// Skip if all body parts are already overridden by higher priority assets
				if (!VisualPart.BodyParts.empty() &&
					!std::any_of(VisualPart.BodyParts.cbegin(), VisualPart.BodyParts.cend(),
						[&FilledBodyParts](CStrID BodyPart) { return FilledBodyParts.find(BodyPart) == FilledBodyParts.cend(); }))
				{
					continue;
				}

				// TODO: skip ignored body parts (maybe only overrides, e.g. helmet), then no need in this for base look. Pass empty ignore list there!

				//LocalBodyParts.insert(VisualPart.BodyParts.cbegin(), VisualPart.BodyParts.cend());

				// Apply first matching variant, if any
				for (const auto& Variant : VisualPart.Variants)
				{
					// Check appearance parameter conditions
					if (Variant.Conditions && Variant.Conditions->GetCount())
					{
						if (!AppearanceComponent.Params) continue;

						bool Match = true;
						for (const auto& Condition : *Variant.Conditions)
						{
							auto pParam = AppearanceComponent.Params->Find(Condition.GetName());
							if (!pParam || pParam->GetRawValue() != Condition.GetRawValue())
							{
								Match = false;
								break;
							}
						}

						if (!Match) continue;
					}

					//!!!???TODO:
					// detect target bone from SlotID if no explicit path specified?
					//!!!use main slot ID for the item, not just the first met slot!
					// how to handle sheathed and unsheathed weapons? a bool flag in a character appearance component? just as 'hide helmet'
					//!!!can patch empty RootBonePath by SlotID in a postprocessing pass, to reuse main part of the loop for the base look!

					// Match found, remember this scene asset for instantiation
					NewLook.emplace_back(Variant.Asset, VisualPart.RootBonePath);
					break;
				}
			}
		}
	}

	// now we must have a vector<pair<PResource<SCNAsset>, TargetBone>>
	// detach attachments parented to altered parts
	// delete parts not needed anymore
	// reparent parts that are instantiated but now changed their parent bone (see below for attachment, can generalize this)
	// create missing parts
	// destroy detached attachments that are not longer needed
	// for each of remaining detached attachments, reattach if target bone exists, destroy otherwise
	// create missing attachments
	// record a new current state to AppearanceComponent

	//!!!TODO: apply material overrides/constants to newly instantiated parts

	int xxx = 0;
}
//---------------------------------------------------------------------

void UpdateCharacterModelEquipment(Game::CGameWorld& World, Game::HEntity OwnerID, CStrID SlotID, bool ForceHide)
{
	if (!OwnerID || !SlotID) return;

	//!!!FIXME: where to place?! Or require bones to be named as slots!
	static const std::map<CStrID, const char*> EEquipmentSlot_Bone
	{
		{ CStrID("Torso"), "torso"},
		{ CStrID("Shoulders"), "shoulders_cloak"},
		{ CStrID("Head"), "head"},
		{ CStrID("Arms"), "arms"},
		{ CStrID("Hands"), "hands"},
		{ CStrID("Legs"), "legs"},
		{ CStrID("Feet"), "feet"},
		{ CStrID("Belt"), "belt"},
		{ CStrID("Backpack"), "backpack"},
		{ CStrID("Neck"), "neck"},
		{ CStrID("Bracelet1"), "bracelet"},
		{ CStrID("Bracelet2"), "bracelet"},
		{ CStrID("Ring1"), "ring"},
		{ CStrID("Ring2"), "ring"},
		{ CStrID("Ring3"), "ring"},
		{ CStrID("Ring4"), "ring"},
		{ CStrID("ItemInHand1"), "mixamorig_RightHandMiddle1"},
		{ CStrID("ItemInHand2"), "mixamorig_LeftHandMiddle1"},
		{ CStrID("ItemInHand3"), "mixamorig_RightHandMiddle1"},
		{ CStrID("ItemInHand4"), "mixamorig_LeftHandMiddle1"}
	};

	//!!!FIXME: constexpr CStrID?! Or at least pre-init once!
	const auto ItBoneName = EEquipmentSlot_Bone.find(SlotID);
	if (ItBoneName == EEquipmentSlot_Bone.cend() || !ItBoneName->second || !*ItBoneName->second) return;

	auto pEquipment = World.FindComponent<const CEquipmentComponent>(OwnerID);
	if (!pEquipment) return;

	auto It = pEquipment->Equipment.find(SlotID);
	const auto StackID = (It == pEquipment->Equipment.cend()) ? Game::HEntity{} : It->second;

	auto pOwnerScene = World.FindComponent<const Game::CSceneComponent>(OwnerID);
	if (!pOwnerScene || !pOwnerScene->RootNode) return;

	auto pBone = pOwnerScene->RootNode->GetChildRecursively(CStrID(ItBoneName->second));
	if (!pBone) return;

	if (StackID && !ForceHide)
	{
		const CItemComponent* pItem = FindItemComponent<const CItemComponent>(World, StackID);
		if (!pItem || !pItem->WorldModelID) return;

		if (pBone->IsWorldTransformDirty()) pBone->UpdateTransform();

		// Undo scaling
		Math::CTransformSRT Tfm(pBone->GetWorldMatrix());
		Tfm.Scale.ReciprocalInplace();
		Tfm.Rotation = quaternion::Identity;
		Tfm.Translation = vector3::Zero;

		//!!!TODO: store desired attachment local rotation and translation (and scaling?) somewhere?

		//!!!DBG TMP!
		Tfm.Rotation.set_rotate_x(HALF_PI);

		auto pSceneComponent = World.AddComponent<Game::CSceneComponent>(StackID);
		pBone->AddChild(CStrID("Equipment"), pSceneComponent->RootNode, true);
		pSceneComponent->AssetID = pItem->WorldModelID;
		pSceneComponent->SetLocalTransform(Tfm);
	}
	else
	{
		pBone->RemoveChild(CStrID("Equipment"));
	}
}
//---------------------------------------------------------------------

void ProcessEquipmentChanges(Game::CGameWorld& World, Game::CGameSession& Session)
{
	World.ForEachComponent<const CEquipmentChangesComponent>([&World, &Session](auto EntityID, const CEquipmentChangesComponent& Changes)
	{
		n_assert2_dbg(!Changes.Records.empty(), "DEM::RPG::ProcessEquipmentChanges() > CEquipmentChangesComponent must be removed when empty!");

		// TODO: reusable set outside ForEachComponent? std::vector with unique check on insert?
		std::set<CStrID> SlotsToUpdate;

		// Apply and remove equipment effects
		for (auto& [StackID, Rec] : Changes.Records)
		{
			const bool IsReequipped = (Rec.NewStorage == Rec.PrevStorage && Rec.NewSlot == Rec.PrevSlot);

			// Remember affected slots for owner 3D model updating
			if (!IsReequipped)
			{
				if (Rec.PrevSlot) SlotsToUpdate.insert(Rec.PrevSlot);
				if (Rec.NewSlot) SlotsToUpdate.insert(Rec.NewSlot);
			}

			//!!!DBG TMP!
			{
				std::string Log = "Item ";
				Log += DEM::Game::EntityToString(StackID);
				Log += " moved from ";
				Log += Rec.PrevSlot.CStr() ? Rec.PrevSlot.CStr() : "outside";
				Log += " to ";
				Log += Rec.NewSlot.CStr() ? Rec.NewSlot.CStr() : "outside";
				Log += "\n";
				::Sys::Log(Log.c_str());
			}

			// Check for equipment or re-equipment request
			if (Rec.NewStorage != EItemStorage::None || IsReequipped)
			{
				auto pEquipped = World.FindComponent<CEquippedComponent>(StackID);
				if (pEquipped)
				{
					n_assert_dbg(!IsReequipped || pEquipped->OwnerID == EntityID);

					// Discard all prevoius modifiers and effects, we will recreate them
					pEquipped->Modifiers.clear();

					// Apply custom logic from the script
					if (!IsReequipped)
					{
						if (auto pEquippable = FindItemComponent<const CEquippableComponent>(World, StackID))
						{
							if (auto ScriptObject = Session.GetScript(pEquippable->ScriptAssetID))
							{
								auto FnProxy = ScriptObject["OnUnequipped"];
								if (FnProxy.get_type() == sol::type::function)
									FnProxy(StackID, EntityID, Rec.PrevStorage, Rec.PrevSlot);
							}
						}
					}
				}
				else
				{
					pEquipped = World.AddComponent<CEquippedComponent>(StackID);
					n_assert_dbg(pEquipped);
					if (!pEquipped) continue; // Should never happen
				}

				pEquipped->OwnerID = EntityID;

				// Apply equipment modifiers and effects

				if (Rec.NewStorage == EItemStorage::Equipment)
				{
					if (auto pArmor = FindItemComponent<const CArmorComponent>(World, StackID))
						if (auto pDestructible = FindItemComponent<CDestructibleComponent>(World, EntityID))
							AddEquipmentModifier<CArmorAbsorptionModifier>(*pEquipped, pDestructible->DamageAbsorption, World, StackID);

					// ...
				}

				// ...

				// Apply custom logic from the script
				if (auto pEquippable = FindItemComponent<const CEquippableComponent>(World, StackID))
				{
					if (auto ScriptObject = Session.GetScript(pEquippable->ScriptAssetID))
					{
						auto FnProxy = ScriptObject["OnEquipped"];
						if (FnProxy.get_type() == sol::type::function)
							FnProxy(StackID, EntityID, Rec.NewStorage, Rec.NewSlot);
					}
				}
			}
			else
			{
				// Modifiers are disconnected when the component is destroyed
				if (World.RemoveComponent<CEquippedComponent>(StackID))
				{
					::Sys::Log("Item unequipped\n");

					// Apply custom logic from the script
					if (auto pEquippable = FindItemComponent<const CEquippableComponent>(World, StackID))
					{
						if (auto ScriptObject = Session.GetScript(pEquippable->ScriptAssetID))
						{
							auto FnProxy = ScriptObject["OnUnequipped"];
							if (FnProxy.get_type() == sol::type::function)
								FnProxy(StackID, EntityID, Rec.PrevStorage, Rec.PrevSlot);
						}
					}

					// TODO: update UI outside loop, need to recalculate stats displayed in screens
					// UI can subscribe on displayed parameter's OnChanged! But then need to trigger their UpdateFinalValue from here!
					// Again, a single 'Update all' event fired to UI seems to be the best way to do that.
				}
			}
		}

		for (CStrID SlotID : SlotsToUpdate)
			UpdateCharacterModelEquipment(World, EntityID, SlotID, false);

		if (auto pAppearance = FindItemComponent<CAppearanceComponent>(World, EntityID))
			RebuildCharacterAppearance(World, EntityID, *pAppearance);
	});

	World.RemoveAllComponents<CEquipmentChangesComponent>();
}
//---------------------------------------------------------------------

}
