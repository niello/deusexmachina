#pragma once
#include <Core/Object.h>

// Character AI command. Subclass CCommand to create specific commands. You can store command params inside a subclass.

namespace DEM::AI
{

enum class ECommandStatus : U8
{
	NotStarted = 0,
	Running,
	Succeeded,
	Failed,
	Cancelled
};

class CCommand : public Core::CObject
{
private:

	friend class CCommandFuture;
	friend class CCommandPromise;

	ECommandStatus _Status = ECommandStatus::NotStarted;
	bool           _Changed = false;
};

using PCommand = Ptr<CCommand>;

class CCommandFuture final
{
private:

	PCommand _Command;

public:

	CCommandFuture(CCommand& Command) : _Command(&Command) {}
	CCommandFuture(const CCommandFuture&) = delete;
	CCommandFuture(CCommandFuture&&) noexcept = default;
	CCommandFuture& operator =(const CCommandFuture&) = delete;
	CCommandFuture& operator =(CCommandFuture&&) noexcept = default;

	ECommandStatus GetStatus() const { return _Command->_Status; }

	// Use non-const to update parameters of a fired command
	template<class T>
	T* As() const
	{
		using TCleaned = std::remove_const_t<T>;
		static_assert(std::is_base_of_v<CCommand, TCleaned>, "All AI commands must be derived from DEM::AI::CCommand");
		static_assert(!std::is_same_v<CCommand, TCleaned>, "You should request only a desired subtype, not the CCommand itself");

		T* pTypedCmd = _Command->As<T>();

		if constexpr (!std::is_const_v<T>)
			if (pTypedCmd)
				_Command->_Changed = true;

		return pTypedCmd;
	}

	bool RequestCancellation() const
	{
		if (_Command->_Status == ECommandStatus::Succeeded || _Command->_Status == ECommandStatus::Failed)
			return false;

		_Command->_Status = ECommandStatus::Cancelled;
		return true;
	}

	bool IsAbandoned() const { return _Command->GetRefCount() == 1; }
};

class CCommandPromise final
{
private:

	PCommand _Command;

public:

	CCommandPromise(CCommand& Command) : _Command(&Command) {}
	CCommandPromise(const CCommandPromise&) = delete;
	CCommandPromise(CCommandPromise&&) noexcept = default;
	CCommandPromise& operator =(const CCommandPromise&) = delete;
	CCommandPromise& operator =(CCommandPromise&&) noexcept = default;

	// Use non-const to enable executing side to store intermediate values right in the command.
	// NB: it is not architecturally correct to give a write access to the command here, but it is needed for possible optimizations.
	template<class T>
	T* As() const
	{
		using TCleaned = std::remove_const_t<T>;
		static_assert(std::is_base_of_v<CCommand, TCleaned>, "All AI commands must be derived from DEM::AI::CCommand");
		static_assert(!std::is_same_v<CCommand, TCleaned>, "You should request only a desired subtype, not the CCommand itself");
		return _Command->As<T>();
	}

	void SetStatus(ECommandStatus Status) { _Command->_Status = Status; }
	void AcceptChanges() { _Command->_Changed = false; }
	bool IsChanged() const { return _Command->_Changed; }
	bool IsCancelled() const { return _Command->_Status == ECommandStatus::Cancelled; }
	bool IsAbandoned() const { return _Command->GetRefCount() == 1; }
};

template<typename T, typename... TArgs>
[[nodiscard]] std::pair<CCommandFuture, CCommandPromise> CreateCommand(TArgs&&... Args)
{
	static_assert(std::is_base_of_v<CCommand, T>, "All AI commands must be derived from DEM::AI::CCommand");

	PCommand Cmd(new T(std::forward<TArgs>(Args)...));
	return std::make_pair(CCommandFuture(*Cmd), CCommandPromise(*Cmd));
}
//---------------------------------------------------------------------

}
