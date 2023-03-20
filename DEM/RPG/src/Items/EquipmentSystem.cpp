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
#include <Scene/NodeAttribute.h>
#include <Data/Algorithms.h>

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

static size_t ApplyAppearance(CAppearanceComponent::CLookMap& Look, const CAppearanceAsset* pAppearanceAsset, const Data::PParams& AppearanceParams,
	const std::set<CStrID>& IgnoredBodyParts)
{
	if (!pAppearanceAsset) return 0;

	size_t OldCount = Look.size();
	for (const auto& VisualPart : pAppearanceAsset->Visuals)
	{
		// Skip explicitly ignored body parts and body parts already overridden by higher priority assets.
		// Asset is added either if it is not associated with body parts or it has at least one not ignored part.
		if (!VisualPart.BodyParts.empty() &&
			!std::any_of(VisualPart.BodyParts.cbegin(), VisualPart.BodyParts.cend(),
				[&IgnoredBodyParts](CStrID BodyPart) { return IgnoredBodyParts.find(BodyPart) == IgnoredBodyParts.cend(); }))
		{
			continue;
		}

		// Apply first matching variant, if any
		for (const auto& Variant : VisualPart.Variants)
		{
			// Check appearance parameter conditions
			if (Variant.Conditions && Variant.Conditions->GetCount())
			{
				if (!AppearanceParams) continue;

				bool Match = true;
				for (const auto& Condition : *Variant.Conditions)
				{
					auto pParam = AppearanceParams->Find(Condition.GetName());
					if (!pParam || pParam->GetRawValue() != Condition.GetRawValue())
					{
						Match = false;
						break;
					}
				}

				if (!Match) continue;
			}

			if (Variant.Asset)
			{
				//!!!???TODO:
				// detect target bone from SlotID if no explicit path specified?
				//!!!use main slot ID for the item, not just the first met slot!
				// how to handle sheathed and unsheathed weapons? a bool flag in a character appearance component? just as 'hide helmet'
				//!!!can patch empty RootBonePath by SlotID in a postprocessing pass, to reuse main part of the loop for the base look!

				// Match found, remember this scene asset for instantiation
				Look.emplace(std::make_pair(Variant.Asset, VisualPart.RootBonePath), Scene::PSceneNode{});
			}

			break;
		}
	}

	return Look.size() - OldCount;
}
//---------------------------------------------------------------------

void RebuildCharacterAppearance(Game::CGameWorld& World, Game::HEntity EntityID, CAppearanceComponent& AppearanceComponent, Resources::CResourceManager& RsrcMgr)
{
	auto pSceneComponent = World.FindComponent<const Game::CSceneComponent>(EntityID);
	if (!pSceneComponent || !pSceneComponent->RootNode) return;
	auto pRootNode = pSceneComponent->RootNode->FindNodeByPath("asset.f_hum_skeleton"); // FIXME: how to determine??? Some convention needed?!
	if (!pRootNode) return;

	CAppearanceComponent::CLookMap NewLook;
	std::set<CStrID> FilledBodyParts;

	if (auto pEquipment = FindItemComponent<const CEquipmentComponent>(World, EntityID))
	{
		// The same as FilledBodyParts but with explicitly ignored parts. Affects only equipment.
		std::set<CStrID> IgnoredBodyParts;
		// TODO: if (AppearanceComponent.HideHelmet) IgnoredBodyParts.insert('Head');

		std::set<Game::HEntity> ProcessedStacks;

		// TODO: process slots in order of priority!
		for (const auto& [SlotID, StackID] : pEquipment->Equipment)
		{
			if (ProcessedStacks.find(StackID) != ProcessedStacks.cend()) continue;

			ProcessedStacks.insert(StackID);

			auto pEquippable = FindItemComponent<const CEquippableComponent>(World, StackID);
			if (!pEquippable) continue;

			// TODO: skip ignored assets
			for (const auto& AppearanceRsrc : pEquippable->AppearanceAssets)
			{
				if (auto pAppearanceAsset = AppearanceRsrc->ValidateObject<CAppearanceAsset>())
				{
					ApplyAppearance(NewLook, pAppearanceAsset, AppearanceComponent.Params, IgnoredBodyParts);

					// Consider body parts filled even if no scene asset was added. Can change this by checking ApplyAppearance return value.
					for (const auto& VisualPart : pAppearanceAsset->Visuals)
					{
						FilledBodyParts.insert(VisualPart.BodyParts.cbegin(), VisualPart.BodyParts.cend());
						IgnoredBodyParts.insert(VisualPart.BodyParts.cbegin(), VisualPart.BodyParts.cend());
					}
				}
			}
		}
	}

	// Apply base look, ignoring only filled body parts but not explicit ignores
	// TODO: skip ignored assets
	for (const auto& AppearanceRsrc : AppearanceComponent.AppearanceAssets)
		ApplyAppearance(NewLook, AppearanceRsrc->ValidateObject<CAppearanceAsset>(), AppearanceComponent.Params, FilledBodyParts);

	// Mark as detached all elements that do not match the new look
	CAppearanceComponent::CLookMap Detached;
	SortedDifference(AppearanceComponent.CurrentLook, NewLook, [&AppearanceComponent, &Detached](CAppearanceComponent::CLookMap::const_iterator It)
	{
		auto LookNode = AppearanceComponent.CurrentLook.extract(It);
		if (LookNode.mapped()) Detached.insert(std::move(LookNode));
	});

	// Also mark as detached all elements that are attached to detached parts of the hierarchy
	for (auto It = AppearanceComponent.CurrentLook.cbegin(); It != AppearanceComponent.CurrentLook.cend(); /**/)
	{
		if (auto pNode = It->second.Get())
		{
			auto ItDetached = std::find_if(Detached.cbegin(), Detached.cend(), [pNode](const auto& Rec) { return pNode->IsChildOf(Rec.second); });
			if (ItDetached != Detached.cend())
			{
				Detached.insert(AppearanceComponent.CurrentLook.extract(It++));
				continue;
			}
		}

		++It;
	}

	// Effectively detach nodes
	for (const auto& LookNode : Detached)
		LookNode.second->RemoveFromParent();

	// Attach nodes for the new look that are not in the current look yet
	SortedDifference(NewLook, AppearanceComponent.CurrentLook, [&NewLook, &AppearanceComponent, &Detached, pRootNode](CAppearanceComponent::CLookMap::const_iterator It)
	{
		const auto pSceneAsset = It->first.first.Get();
		if (!pSceneAsset) return;

		auto LookNode = NewLook.extract(It);

		auto ItCache = std::find_if(Detached.begin(), Detached.end(), [pSceneAsset](const auto& CacheRec) { return CacheRec.first.first == pSceneAsset; });
		if (ItCache != Detached.cend())
		{
			// Reuse already instantiated visual part from the previous look
			LookNode.mapped() = std::move(ItCache->second);
			Detached.erase(ItCache);
		}
		else if (auto NodeTpl = pSceneAsset->ValidateObject<Scene::CSceneNode>())
		{
			// Instantiate a new visual part
			LookNode.mapped() = NodeTpl->Clone();
		}

		if (LookNode.mapped())
			pRootNode->FindNodeByPath(LookNode.key().second.c_str())->AddChild(pSceneAsset->GetUID(), LookNode.mapped());

		AppearanceComponent.CurrentLook.insert(std::move(LookNode));
	});

	// Discard not reused nodes
	Detached.clear();

	// Validate resources
	pRootNode->Visit([&RsrcMgr](Scene::CSceneNode& Node)
	{
		for (UPTR i = 0; i < Node.GetAttributeCount(); ++i)
			Node.GetAttribute(i)->ValidateResources(RsrcMgr);
		return true;
	});
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

void ProcessEquipmentChanges(Game::CGameWorld& World, Game::CGameSession& Session, Resources::CResourceManager& RsrcMgr)
{
	World.ForEachComponent<const CEquipmentChangesComponent>([&World, &Session, &RsrcMgr](auto EntityID, const CEquipmentChangesComponent& Changes)
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
			RebuildCharacterAppearance(World, EntityID, *pAppearance, RsrcMgr);
	});

	World.RemoveAllComponents<CEquipmentChangesComponent>();
}
//---------------------------------------------------------------------

}
