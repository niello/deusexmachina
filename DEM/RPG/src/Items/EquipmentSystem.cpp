#include <Game/ECS/GameWorld.h>
#include <Game/GameSession.h>
#include <Items/ArmorComponent.h>
#include <Items/EquipmentChangesComponent.h>
#include <Items/EquippableComponent.h>
#include <Combat/DestructibleComponent.h>

// A set of ECS systems required for functioning of the equipment logic

namespace DEM::RPG
{

//!!!DBG TMP! Must be not here, maybe in Damage.h?
class CArmorAbsorptionModifier : public CParameterModifier<CDamageAbsorption>
{
public:

	DEM::Game::CGameWorld& _World;
	DEM::Game::HEntity _SourceID;

	CArmorAbsorptionModifier(DEM::Game::CGameWorld& World, DEM::Game::HEntity SourceID) : _World(World), _SourceID(SourceID) {}

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
				ScheduleReequipment(World, StackID);
			}
		}

		// Request application of equipment effects for already equipped stacks in quick slots
		for (const auto StackID : Component.QuickSlots)
		{
			if (auto pEquipped = World.AddComponent<CEquippedComponent>(StackID))
			{
				pEquipped->OwnerID = EntityID;
				ScheduleReequipment(World, StackID);
			}
		}
	});
}
//---------------------------------------------------------------------

void ProcessEquipmentChanges(Game::CGameWorld& World, Game::CGameSession& Session)
{
	World.ForEachComponent<const CEquipmentChangesComponent>([&World, &Session](auto EntityID, const CEquipmentChangesComponent& Changes)
	{
		// TODO: reusable set outside ForEachComponent? std::vector with unique check on insert?
		std::set<CStrID> SlotsToUpdate;

		for (auto& [StackID, Rec] : Changes.Records)
		{
			// Remember affected slots for owner 3D model updating
			if (Rec.PrevSlot) SlotsToUpdate.insert(Rec.PrevSlot);
			if (Rec.NewSlot) SlotsToUpdate.insert(Rec.NewSlot);

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
			const bool IsReequipped = (Rec.NewStorage == Rec.PrevStorage && Rec.NewSlot == Rec.PrevSlot);
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
						if (auto pEquippable = FindItemComponent<CEquippableComponent>(World, StackID))
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
				if (auto pEquippable = FindItemComponent<CEquippableComponent>(World, StackID))
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
					if (auto pEquippable = FindItemComponent<CEquippableComponent>(World, StackID))
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
			UpdateCharacterModelEquipment(World, EntityID, SlotID);
	});

	World.RemoveAllComponents<CEquipmentChangesComponent>();
}
//---------------------------------------------------------------------

}
