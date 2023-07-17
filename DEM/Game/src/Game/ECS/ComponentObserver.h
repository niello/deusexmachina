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
			((std::is_same_v<TComponent, TComponents> || _World.FindComponent<const TComponents>(EntityID))
			&& ...);

		if (Collected)
		{
			_Started.push_back(EntityID);
			Algo::VectorFastErase(_Stopped, EntityID);
		}
	}

	template<typename TComponent>
	void OnComponentRemoved(HEntity EntityID)
	{
		// Check if this removal breaks a full set of observed components for this entity
		const bool HadAllComponents = Algo::VectorFastErase(_Started, EntityID) ||
			((std::is_same_v<TComponent, TComponents> || _World.FindComponent<const TComponents>(EntityID))
				&& ...);

		if (HadAllComponents) _Stopped.push_back(EntityID);
	}

	template<typename TComponent>
	void Init(size_t Index)
	{
		if (auto pStorage = _World.FindComponentStorage<TComponent>())
		{
			_OnAddConn[Index] = pStorage->OnAdd.Subscribe([this](HEntity EntityID, TComponent*)
			{
				OnComponentAdded<TComponent>(EntityID);
			});
			_OnRemoveConn[Index] = pStorage->OnRemove.Subscribe([this](HEntity EntityID, TComponent*)
			{
				OnComponentRemoved<TComponent>(EntityID);
			});
		}
	}

	template <std::size_t... I> void InitInternal(std::index_sequence<I...>) { (Init<TComponents>(I), ...); }

public:

	CComponentObserver(CGameWorld& World)
		: _World(World)
	{
		InitInternal(std::index_sequence_for<TComponents...>());
	}

	template<typename F> void ForEachStarted(F Callback) const { for (HEntity EntityID : _Started) Callback(EntityID); }
	template<typename F> void ForEachStopped(F Callback) const { for (HEntity EntityID : _Stopped) Callback(EntityID); }

	void Clear()
	{
		_Started.clear();
		_Stopped.clear();
	}

	bool IsConnected() const { return OnAddConn[0].IsConnected(); }
};

}
