#pragma once
#include <Game/GameVarStorage.h>
#include <Events/Signal.h>

// An AI agent's blackboard. Stores heterogeneous values and notifies listeners about changes.

namespace DEM::AI
{

class CBlackboard
{
protected:

	Game::CGameVarStorage _Storage;

public:

	mutable Events::CSignal<void(HVar)> OnChanged;

	void Clear()
	{
		_Storage.clear();
		OnChanged({}); // a special case for clearing
	}

	template<typename T>
	void Set(HVar Handle, T&& Value)
	{
		_Storage.Set(Handle, std::forward<T>(Value));
		OnChanged(Handle);
	}

	// NB: this is probably too big to be inlined and microoptimization with T instead of T&& yields better assembly
	template<typename T, typename std::enable_if_t<DEM::Meta::should_pass_by_value<T>>* = nullptr>
	HVar Set(CStrID ID, T Value)
	{
		const auto Handle = _Storage.Set(ID, Value);
		OnChanged(Handle);
		return Handle;
	}

	template<typename T, typename std::enable_if_t<!DEM::Meta::should_pass_by_value<T>>* = nullptr>
	HVar Set(CStrID ID, T&& Value)
	{
		const auto Handle = _Storage.Set(ID, std::forward<T>(Value));
		OnChanged(Handle);
		return Handle;
	}

	template<typename T>
	HVar TrySet(CStrID ID, T&& Value)
	{
		const auto Handle = _Storage.TrySet(ID, std::forward<T>(Value));
		if (Handle) OnChanged(Handle);
		return Handle;
	}

	HVar TrySet(CStrID ID, Data::CData& Value) { return TrySet(ID, std::as_const(Value)); }

	HVar TrySet(CStrID ID, const Data::CData& Value)
	{
		const auto Handle = _Storage.TrySet(ID, Value);
		if (Handle) OnChanged(Handle);
		return Handle;
	}

	// All non-modifying operations can be done directly
	const auto& GetStorage() const { return _Storage; }
};

}
