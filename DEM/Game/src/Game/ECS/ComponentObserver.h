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

	CGameWorld&          _World;
	Events::CConnection  _OnAddConn[sizeof...(TComponents)];
	Events::CConnection  _OnRemoveConn[sizeof...(TComponents)];

	std::vector<HEntity> _Started;
	std::vector<HEntity> _Stopped;

	template<typename TComponent>
	void OnComponentAdded(HEntity EntityID)
	{
		// Check existence of all other components for this entity
		const bool Collected =
			((std::is_same_v<TComponent, TComponents> || _World.FindComponent<std::add_const_t<TComponent>>(EntityID))
			&& ...);

		if (Collected)
		{
			_Started.push_back(EntityID);
			VectorFastErase(_Stopped, std::find(_Stopped.begin(), _Stopped.end(), EntityID));
		}
	}

	template<typename TComponent>
	void Init(size_t Index)
	{
		if (auto pStorage = _World.FindComponentStorage<TComponent>())
		{
			_OnAddConn[Index] = pStorage->OnAdd.Subscribe([this](HEntity EntityID, TComponent* pComponent)
			{
				OnComponentAdded<TComponent>(EntityID);
			});
			//_OnAddConn[Index] = pStorage->OnAdd.Subscribe(std::bind(&CComponentObserver::OnComponentAdded<TComponent>, this, std::placeholders::_1));
			_OnRemoveConn[Index] = pStorage->OnRemove.Subscribe([](HEntity EntityID, TComponent* pComponent)
			{
				// remove from the list (fast erase by swapping)
				// if was in the list, add to removed list
				//???need template bool flags TrackStart, TrackStop? if so, may not need one of vectors!
				//can use conditional type - vector or empty struct, to save memory
				//static assert at least one of 2 flags is set
			});
		}
	}

	template <std::size_t... I>
	void InitInternal(std::index_sequence<I...>)
	{
		(Init<TComponents>(I), ...);
	}

public:

	CComponentObserver(CGameWorld& World)
		: _World(World)
	{
		InitInternal(std::index_sequence_for<TComponents...>());
	}

	bool IsConnected() const { return OnAddConn[0].IsConnected(); }
};

}
