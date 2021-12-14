#include "ItemUtils.h"
#include <Items/ItemComponent.h>
#include <Items/ItemContainerComponent.h>
#include <Scene/SceneComponent.h>
#include <Physics/RigidBodyComponent.h>

namespace DEM::RPG
{

Game::HEntity AddItemsIntoContainer(Game::CGameWorld& World, Game::HEntity Container, Game::HEntity StackID, bool Merge)
{
	auto pContainer = World.FindComponent<CItemContainerComponent>(Container);
	if (!pContainer) return {};

	// Check that we don't insert already contained stack
	if (std::find(pContainer->Items.cbegin(), pContainer->Items.cend(), StackID) != pContainer->Items.cend())
		return {};

	auto pItemStack = World.FindComponent<CItemStackComponent>(StackID);
	n_assert_dbg(pItemStack->Count);
	if (!pItemStack || pItemStack->Count) return {};

	// Check volume limits. Weight doesn't block adding items, only carrying them.
	// TODO: split stack, fill available container space!
	// bool flag in args to enable this? return actually added count / remaining stack ID?
	if (pContainer->MaxVolume >= 0.f)
	{
		if (auto pItem = FindItemComponent<const CItemComponent>(World, StackID, *pItemStack))
		{
			if (pItem->Volume > 0.f)
			{
				const float RequiredVolume = pItemStack->Count * pItem->Volume;
				float FreeVolume = pContainer->MaxVolume;
				for (auto StackID : pContainer->Items)
				{
					FreeVolume -= GetItemStackVolume(World, StackID);
					if (FreeVolume < RequiredVolume) return {};
				}
			}
		}
	}

	// Items inside a container are always hidden from the view
	RemoveItemsFromLocation(World, StackID);

	return AddStackIntoCollection(World, pContainer->Items, StackID, Merge);
}
//---------------------------------------------------------------------

Game::HEntity AddStackIntoCollection(Game::CGameWorld& World, std::vector<Game::HEntity>& Collection, Game::HEntity StackID, bool Merge)
{
	// Try to merge new items into existing stack
	if (Merge)
	{
		// TODO: maybe will need to remove !Modified condition in the future. See CanMergeStacks() logic!
		//???don't merge items with different owners?!
		auto pStack = World.FindComponent<const CItemStackComponent>(StackID);
		if (pStack && !pStack->Modified)
		{
			for (auto MergeAcceptorID : Collection)
			{
				auto pMergeTo = World.FindComponent<CItemStackComponent>(MergeAcceptorID);
				if (CanMergeStacks(*pStack, pMergeTo))
				{
					pMergeTo->Count += pStack->Count;
					World.DeleteEntity(StackID);
					return MergeAcceptorID;
				}
			}
		}
	}

	// If not merged, transfer a stack into the container
	auto It = std::find(Collection.begin(), Collection.end(), DEM::Game::HEntity{});
	if (It == Collection.cend())
		Collection.push_back(StackID);
	else
		(*It) = StackID;

	return StackID;
}
//---------------------------------------------------------------------

void ShrinkItemCollection(std::vector<Game::HEntity>& Collection)
{
	// Trim the tail to the last busy slot
	auto RIt = ++Collection.rbegin();
	for (; RIt != Collection.rend(); ++RIt)
		if (*RIt) break;
	Collection.erase(RIt.base(), Collection.end());
}
//---------------------------------------------------------------------

//!!!check dropping near another item stack or pile (temporary container)! bool flag 'allow merging'?
bool DropItemsToLocation(Game::CGameWorld& World, Game::HEntity ItemStackEntity, const Math::CTransformSRT& Tfm)
{
	auto pItemStack = World.FindComponent<CItemStackComponent>(ItemStackEntity);
	if (!pItemStack) return false;

	const CItemComponent* pItem = FindItemComponent<const CItemComponent>(World, ItemStackEntity, *pItemStack);
	if (!pItem) return false;

	// TODO:
	// If merging enabled, query item stacks and tmp item containers in accessible range, not owned by someone else
	// If tmp item containers are found, add the stack to the closest one (take lookat dir into account?)
	// Else if stacks are found, try to merge into the closest one (take lookat dir into account?)
	// If not merged, create tmp item container and add found stack and our stack to it
	// If nothing found, drop a new item object into location (create scene and RB)

	if (pItem->WorldModelID)
	{
		auto pSceneComponent = World.AddComponent<Game::CSceneComponent>(ItemStackEntity);
		pSceneComponent->RootNode->RemoveFromParent();
		pSceneComponent->AssetID = pItem->WorldModelID;
		pSceneComponent->SetLocalTransform(Tfm);
	}

	if (pItem->WorldPhysicsID)
	{
		auto pPhysicsComponent = World.AddComponent<Game::CRigidBodyComponent>(ItemStackEntity);
		pPhysicsComponent->ShapeAssetID = pItem->WorldPhysicsID;
		pPhysicsComponent->Mass = pItemStack->Count * pItem->Weight;
		pPhysicsComponent->CollisionGroupID = CStrID("PhysicalDynamic|Interactable");
		// TODO: physics material?
	}

	return true;
}
//---------------------------------------------------------------------

void RemoveItemsFromLocation(Game::CGameWorld& World, Game::HEntity StackID)
{
	World.RemoveComponent<Game::CSceneComponent>(StackID);
	World.RemoveComponent<Game::CRigidBodyComponent>(StackID);
}
//---------------------------------------------------------------------

// TODO: use container as an std::optional hint? or handle this in user API and call RemoveItemsFromContainer only when needed?
void RemoveItemsFromContainer(Game::CGameWorld& World, Game::HEntity ItemStackEntity, Game::HEntity Container)
{
	auto pContainer = World.FindComponent<CItemContainerComponent>(Container);
	if (!pContainer) return;

	pContainer->Items.erase(std::remove(pContainer->Items.begin(), pContainer->Items.end(), ItemStackEntity));
}
//---------------------------------------------------------------------

void CalcContainerStats(Game::CGameWorld& World, const CItemContainerComponent& Container, CContainerStats& OutStats)
{
	OutStats.UsedWeight = 0.f;
	OutStats.UsedVolume = 0.f;
	OutStats.Price = 0;
	for (auto ItemEntityID : Container.Items)
	{
		auto pStack = World.FindComponent<const CItemStackComponent>(ItemEntityID);
		if (!pStack) continue;

		if (auto pItem = FindItemComponent<const CItemComponent>(World, ItemEntityID, *pStack))
		{
			OutStats.UsedWeight += pStack->Count * pItem->Weight;
			OutStats.UsedVolume += pStack->Count * pItem->Volume;
			OutStats.Price += pStack->Count * pItem->Price; //???what to count? only valuable or money-like items?
		}
	}

	OutStats.FreeWeight = (Container.MaxWeight <= 0.f) ? FLT_MAX : (Container.MaxWeight - OutStats.UsedWeight);
	OutStats.FreeVolume = (Container.MaxVolume <= 0.f) ? FLT_MAX : (Container.MaxVolume - OutStats.UsedVolume);
}
//---------------------------------------------------------------------

bool CanMergeStacks(const CItemStackComponent& SrcStack, const CItemStackComponent* pDestStack)
{
	//???TODO: can add more possibilities for merging? deep comparison of components?
	return pDestStack && pDestStack->Prototype == SrcStack.Prototype && !SrcStack.Modified && !pDestStack->Modified;
}
//---------------------------------------------------------------------

float GetItemStackVolume(const Game::CGameWorld& World, Game::HEntity StackID)
{
	if (auto pStack = World.FindComponent<const CItemStackComponent>(StackID))
		if (auto pItem = FindItemComponent<const CItemComponent>(World, StackID, *pStack))
			return pStack->Count * pItem->Volume;
	return 0.f;
}
//---------------------------------------------------------------------

Game::HEntity TransferItems(Game::CGameWorld& World, U32 Count, Game::HEntity& SrcSlot, Game::HEntity& DestSlot)
{
	const auto SrcStackID = SrcSlot;
	const auto DestStackID = DestSlot;

	if (!Count || !SrcStackID || SrcStackID == DestStackID) return SrcStackID;

	auto pSrcStack = World.FindComponent<CItemStackComponent>(SrcStackID);
	if (!pSrcStack) return {};

	const bool TransferWholeStack = (pSrcStack->Count == Count);
	if (TransferWholeStack)
		SrcSlot = {};
	else
		pSrcStack->Count -= Count;

	auto pDestStack = World.FindComponent<CItemStackComponent>(DestStackID);
	if (CanMergeStacks(*pSrcStack, pDestStack))
	{
		pDestStack->Count += Count;
		if (TransferWholeStack) World.DeleteEntity(SrcStackID);
	}
	else
	{
		if (TransferWholeStack)
		{
			DestSlot = SrcStackID;
		}
		else
		{
			const auto NewStackID =
				World.CloneEntityExcluding<Game::CSceneComponent, Game::CRigidBodyComponent>(SrcStackID);
			if (auto pNewStack = World.FindComponent<CItemStackComponent>(NewStackID))
				pNewStack->Count = Count;
			DestSlot = NewStackID;
		}

		// If we replaced some stack, start dragging it instead of the source stack
		if (DestStackID) return DestStackID;
	}

	return SrcStackID;
}
//---------------------------------------------------------------------

}
