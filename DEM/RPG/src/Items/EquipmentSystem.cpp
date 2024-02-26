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

//!!!DBG TMP!
#include <Frame/Lights/PointLightAttribute.h>

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
		{
			Component.Scheme = ResMgr.RegisterResource<CEquipmentScheme>(Component.SchemeID.CStr())->ValidateObject<CEquipmentScheme>();
			if (Component.Scheme && Component.Scheme->HandCount)
				Component.Hands.reset(new CHandSlot[Component.Scheme->HandCount]);
		}

		// Request application of equipment effects for already equipped stacks
		for (const auto [SlotID, StackID] : Component.Equipment)
		{
			if (auto pEquipped = World.AddComponent<CEquippedComponent>(StackID))
			{
				pEquipped->OwnerID = EntityID;
				ScheduleStackReequipment(World, StackID, EItemStorage::Equipment, FindMainOccupiedSlot(World, Component, StackID, EItemStorage::Equipment).first);
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
	const std::set<CStrID>& IgnoredBodyParts, const CEquipmentScheme* pEquipmentScheme, CStrID SlotID)
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
				//!!!TODO: explicit empty RootBonePath and deafult RootBonePath may need to be treated differently, but now can't distinguish between them!
				std::string Bone = VisualPart.RootBonePath;
				if (pEquipmentScheme && Bone.empty())
				{
					auto It = pEquipmentScheme->SlotBones.find(SlotID);
					if (It != pEquipmentScheme->SlotBones.cend())
						Bone = It->second;
				}

				// Match found, remember this scene asset for instantiation
				Look.emplace(std::make_pair(Variant.Asset, std::move(Bone)), Scene::PSceneNode{});
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

	auto pEquipment = World.FindComponent<const CEquipmentComponent>(EntityID);
	if (pEquipment && pEquipment->Scheme)
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
					//!!!TODO: use main slot ID for the item, not just the first met slot!
					ApplyAppearance(NewLook, pAppearanceAsset, AppearanceComponent.Params, IgnoredBodyParts, pEquipment->Scheme, SlotID);

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
		ApplyAppearance(NewLook, AppearanceRsrc->ValidateObject<CAppearanceAsset>(), AppearanceComponent.Params, FilledBodyParts, nullptr, CStrID::Empty);

	// Mark as detached all elements that do not match the new look
	CAppearanceComponent::CLookMap Detached;
	Algo::SortedDifference(AppearanceComponent.CurrentLook, NewLook, [&AppearanceComponent, &Detached](CAppearanceComponent::CLookMap::const_iterator It)
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
	Algo::SortedDifference(NewLook, AppearanceComponent.CurrentLook, [&NewLook, &AppearanceComponent, &Detached, pRootNode](CAppearanceComponent::CLookMap::const_iterator It)
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
		{
			//???resolve in ApplyAppearance and store node pointers instead of paths in a look map?
			const char* pBoneName = LookNode.key().second.c_str();
			auto pDestNode = pRootNode->FindNodeByPath(pBoneName);
			if (!pDestNode) pDestNode = pRootNode->GetChildRecursively(CStrID(pBoneName));
			if (pDestNode)
				pDestNode->AddChild(pSceneAsset->GetUID(), LookNode.mapped());
			else
				::Sys::Error("Can't find a bone for appearance attachment");
		}

		AppearanceComponent.CurrentLook.insert(std::move(LookNode));
	});

	// Discard not reused nodes
	Detached.clear();

	// Process attachments
	if (pEquipment && pEquipment->Scheme)
	{
		std::map<Game::HEntity, Scene::CSceneNode*> NewAttachments; // Item stack ID -> Target bone

		static const CStrID sidScabbard("Scabbard");
		static const CStrID sidBigScabbard("BigScabbard");
		for (auto [SlotID, StackID] : pEquipment->Equipment) // TODO: the same for Q slots!
		{
			// No item - no attachment
			if (!StackID) continue;

			// Ignore slots that do not produce attachments
			const auto SlotType = pEquipment->Scheme->Slots[SlotID];
			if (SlotType != sidScabbard && SlotType != sidBigScabbard) continue;

			const CItemComponent* pItemComponent = FindItemComponent<const CItemComponent>(World, StackID);
			if (!pItemComponent || !pItemComponent->WorldModelID) continue;

			CStrID BoneKey = SlotID;
			for (size_t HandIdx = 0; HandIdx < pEquipment->Scheme->HandCount; ++HandIdx)
			{
				const auto& Hand = pEquipment->Hands[HandIdx];
				if (Hand.ScabbardSlotID == SlotID && Hand.Unsheathed)
					BoneKey = CStrID("__Hand" + std::to_string(HandIdx));
			}

			// Find parent bone for the attachment
			auto It = pEquipment->Scheme->SlotBones.find(BoneKey);
			if (It != pEquipment->Scheme->SlotBones.cend())
			{
				const char* pBoneName = It->second.c_str();
				auto pDestNode = pRootNode->FindNodeByPath(pBoneName);
				if (!pDestNode) pDestNode = pRootNode->GetChildRecursively(CStrID(pBoneName));
				if (!pDestNode)
				{
					// No target bone - no attachment
					::Sys::Error("Can't find a bone for item attachment");
					continue;
				}

				NewAttachments.emplace(StackID, pDestNode);
			}
		}

		// Synchronize current attachments with desired list
		// TODO: need Algo::SortedUnion for map+set / set+map! Currently a single Less isn't capable of handling a < b and b < a at the same time. Need a trait to get key from iterator?!
		{
			auto ItCurrA = NewAttachments.begin();
			auto ItCurrB = AppearanceComponent.CurrentAttachments.begin();
			bool IsEndA = (ItCurrA == NewAttachments.cend());
			bool IsEndB = (ItCurrB == AppearanceComponent.CurrentAttachments.cend());
			while (!IsEndA || !IsEndB)
			{
				decltype(ItCurrA) ItNew;
				decltype(ItCurrB) ItOld;
				if (IsEndB || (!IsEndA && ItCurrA->first < *ItCurrB))
				{
					ItNew = ItCurrA++;
					ItOld = AppearanceComponent.CurrentAttachments.end();
					IsEndA = (ItCurrA == NewAttachments.cend());
				}
				else if (IsEndA || *ItCurrB < ItCurrA->first)
				{
					ItNew = NewAttachments.end();
					ItOld = ItCurrB++;
					IsEndB = (ItCurrB == AppearanceComponent.CurrentAttachments.cend());
				}
				else // equal
				{
					ItNew = ItCurrA++;
					ItOld = ItCurrB++;
					IsEndA = (ItCurrA == NewAttachments.cend());
					IsEndB = (ItCurrB == AppearanceComponent.CurrentAttachments.cend());
				}

				if (ItNew == NewAttachments.cend())
				{
					// Remove old attachment
					World.RemoveComponent<Game::CSceneComponent>(*ItOld);
					AppearanceComponent.CurrentAttachments.erase(ItOld);
				}
				else if (ItOld == AppearanceComponent.CurrentAttachments.cend())
				{
					// Add new attachment
					auto pItemSceneComponent = World.FindComponent<Game::CSceneComponent>(ItNew->first);
					if (!pItemSceneComponent)
						pItemSceneComponent = World.AddComponent<Game::CSceneComponent>(ItNew->first);

					// FIXME PERF: can cache CItemComponent in NewAttachments because has already found it!
					pItemSceneComponent->AssetID = FindItemComponent<const CItemComponent>(World, ItNew->first)->WorldModelID;

					Scene::CSceneNode* pDestNode = ItNew->second;
					if (pDestNode->IsWorldTransformDirty()) pDestNode->UpdateTransform();

					pItemSceneComponent->RootNode->SetLocalScale(rtm::vector_reciprocal(Math::matrix_extract_scale(pDestNode->GetWorldMatrix()))); // Undo scaling
					pDestNode->AddChild(CStrID("Equipment"), pItemSceneComponent->RootNode, true);

					AppearanceComponent.CurrentAttachments.insert(ItNew->first);
				}
				else
				{
					//!!!FIXME: duplicated code!!!
					auto pItemSceneComponent = World.FindComponent<Game::CSceneComponent>(ItNew->first);
					if (pItemSceneComponent)
					{
						// If already attached where needed, skip
						if (pItemSceneComponent->RootNode->GetParent() == ItNew->second) break;

						// Else detach from old parent
						pItemSceneComponent->RootNode->RemoveFromParent();
					}
					else
					{
						pItemSceneComponent = World.AddComponent<Game::CSceneComponent>(ItNew->first);

						// FIXME PERF: can cache CItemComponent in NewAttachments because has already found it!
						pItemSceneComponent->AssetID = FindItemComponent<const CItemComponent>(World, ItNew->first)->WorldModelID;
					}

					// Attach to new parent
					Scene::CSceneNode* pDestNode = ItNew->second;
					if (pDestNode->IsWorldTransformDirty()) pDestNode->UpdateTransform();

					pItemSceneComponent->RootNode->SetLocalScale(rtm::vector_reciprocal(Math::matrix_extract_scale(pDestNode->GetWorldMatrix()))); // Undo scaling
					pDestNode->AddChild(CStrID("Equipment"), pItemSceneComponent->RootNode, true);
				}
			}
		}
	}

	//!!!TODO PERF: only if new models created!
	// Validate resources
	pRootNode->Visit([&RsrcMgr](Scene::CSceneNode& Node)
	{
		for (UPTR i = 0; i < Node.GetAttributeCount(); ++i)
			Node.GetAttribute(i)->ValidateResources(RsrcMgr);
		return true;
	});
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

						FnProxy = ScriptObject["UpdateEquipped"];
						if (FnProxy.get_type() == sol::type::function)
							pEquipped->FnUpdateEquipped = FnProxy;
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

		// Process appearance parts and attachments
		if (auto pAppearance = World.FindComponent<CAppearanceComponent>(EntityID))
			RebuildCharacterAppearance(World, EntityID, *pAppearance, RsrcMgr);
	});

	World.RemoveAllComponents<CEquipmentChangesComponent>();
}
//---------------------------------------------------------------------

void UpdateEquipment(Game::CGameWorld& World, float dt)
{
	ZoneScoped;

	World.ForEachComponent<const CEquippedComponent>([&World, dt](auto StackID, const CEquippedComponent& Equipped)
	{
		if (Equipped.FnUpdateEquipped) Equipped.FnUpdateEquipped(StackID, dt);
	});
}
//---------------------------------------------------------------------

}
