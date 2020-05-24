#pragma once
#include <Game/ECS/EntityMap.h>
#include <Data/SparseArray.hpp>

// Storage for removed components that await custom deinitialization

namespace DEM::Game
{
typedef std::unique_ptr<class IDeadComponentStorage> PDeadComponentStorage;

class IDeadComponentStorage
{
public:

	virtual ~IDeadComponentStorage() = default;
};

template<typename T>
class CDeadComponentStorage : public IDeadComponentStorage
{
	static_assert(!std::is_empty_v<T>, "Empty components have no data to deinitialize");

public:

	using TInnerStorage = Data::CSparseArray<std::pair<T, HEntity>, U32>;

	TInnerStorage   _Data;
	CEntityMap<U32> _IndexByEntity;

public:

	CDeadComponentStorage(size_t InitialCapacity = 0)
		: _Data(std::min<size_t>(InitialCapacity, TInnerStorage::MAX_CAPACITY))
		, _IndexByEntity(std::min<size_t>(InitialCapacity, TInnerStorage::MAX_CAPACITY))
	{
		n_assert_dbg(InitialCapacity <= TInnerStorage::MAX_CAPACITY);
	}

	bool Acquire(HEntity EntityID, T&& Dead)
	{
		if (_IndexByEntity.find(EntityID)) return false;
		const auto Index = _Data.insert(std::move(Dead));
		if (Index == TInnerStorage::INVALID_INDEX) return false;
		_IndexByEntity.emplace(EntityID, Index);
		return true;
	}

	bool Resurrect(HEntity EntityID, T& OutAlive)
	{
		if (auto pRecord = _IndexByEntity.find(EntityID))
		{
			OutAlive = std::move(_Data[pRecord->Value]);
			_Data.erase(pRecord->Value);
			_IndexByEntity.erase(pRecord);
			return true;
		}
		return false;
	}

	void Remove(HEntity EntityID)
	{
		if (auto pRecord = _IndexByEntity.find(EntityID))
		{
			_Data.erase(pRecord->Value);
			_IndexByEntity.erase(pRecord);
		}
	}

	void Clear()
	{
		_Data.clear();
		_IndexByEntity.clear();
	}

	auto begin() { return _Data.begin(); }
	auto begin() const { return _Data.begin(); }
	auto cbegin() const { return _Data.cbegin(); }
	auto end() { return _Data.end(); }
	auto end() const { return _Data.end(); }
	auto cend() const { return _Data.cend(); }

	size_t GetComponentCount() const { return _Data.size(); }
};

}
