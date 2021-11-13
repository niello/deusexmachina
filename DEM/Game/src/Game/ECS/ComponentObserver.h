#pragma once
#include <Game/ECS/GameWorld.h>

// Tracks a list of entities who started or stopped having a set of components

namespace DEM::Game
{

template<typename... TComponents>
class CComponentObserver
{
protected:

	static_assert(sizeof...(TComponents) > 0, "CComponentObserver must observe at least one component type");

	Events::CConnection  _OnAddConn[sizeof...(TComponents)];
	Events::CConnection  _OnRemoveConn[sizeof...(TComponents)];

	std::vector<HEntity> _Started;
	std::vector<HEntity> _Stopped;

public:

	CComponentObserver(CGameWorld& World)
	{
		if (auto pStorage = World.FindComponentStorage<TComponents...>())
		{
			_OnAddConn[0] = pStorage->OnAdd.Subscribe([](HEntity EntityID, TComponents...* pComponent)
			{
				// check existence of all other components
				// if so, add to the list
			});
			_OnRemoveConn[0] = pStorage->OnRemove.Subscribe([](HEntity EntityID, TComponents...* pComponent)
			{
				// remove from the list (fast erase by swapping)
				// if was in the list, add to removed list
				//???need template bool flags TrackStart, TrackStop? if so, may not need one of vectors!
				//can use conditional type - vector or empty struct, to save memory
				//static assert at least one of 2 flags is set
			});
		}
	}

	// bool IsConnected() const { return OnAddConn[0].IsConnected(); }
};

}
