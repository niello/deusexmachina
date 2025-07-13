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

inline bool IsFinishedCommandStatus(ECommandStatus Status) { return Status == ECommandStatus::Succeeded || Status == ECommandStatus::Failed; }
inline bool IsTerminalCommandStatus(ECommandStatus Status) { return IsFinishedCommandStatus(Status) || Status == ECommandStatus::Cancelled; }

class CCommand : public Core::CObject
{
	RTTI_CLASS_DECL(DEM::AI::CCommand, Core::CObject);

private:

	friend class CCommandFuture;
	friend class CCommandPromise;

	ECommandStatus _Status = ECommandStatus::NotStarted;
	bool           _Changed = true; // Considered changed on creation because a processing subsystem didn't it yet
};

using PCommand = Ptr<CCommand>;

class CCommandFuture final
{
private:

	PCommand _Command;

public:

	CCommandFuture() = default;
	CCommandFuture(CCommand& Command) : _Command(&Command) {}
	CCommandFuture(const CCommandFuture&) = delete;
	CCommandFuture(CCommandFuture&&) noexcept = default;
	CCommandFuture& operator =(const CCommandFuture&) = delete;
	CCommandFuture& operator =(CCommandFuture&&) noexcept = default;

	// NB: an empty future must return a terminal status to ensure that there is no waiting on it
	ECommandStatus GetStatus() const { return _Command ? _Command->_Status : ECommandStatus::Failed; }

	template<typename... T> bool IsAnyOf() const { return _Command && _Command->IsAnyOfExactly<T...>(); }

	// Use non-const to update parameters of a fired command
	template<typename T>
	T* As() const
	{
		using TCleaned = std::remove_const_t<T>;
		static_assert(std::is_base_of_v<CCommand, TCleaned>, "All AI commands must be derived from DEM::AI::CCommand");
		static_assert(!std::is_same_v<CCommand, TCleaned>, "You should request only a desired subtype, not the CCommand itself");

		T* pTypedCmd = _Command ? _Command->As<T>() : nullptr;

		if constexpr (!std::is_const_v<T>)
			if (pTypedCmd)
				_Command->_Changed = true;

		return pTypedCmd;
	}

	bool RequestCancellation() const
	{
		if (!_Command || IsFinishedCommandStatus(_Command->_Status))
			return false;

		_Command->_Status = ECommandStatus::Cancelled;
		return true;
	}

	bool IsAbandoned() const { return !_Command || _Command->GetRefCount() == 1; }

	operator bool() const noexcept { return !!_Command; }
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

	template<typename... T> bool IsAnyOf() const { return _Command->IsAnyOfExactly<T...>(); }

	// Use non-const to enable executing side to store intermediate values right in the command.
	// NB: it is not architecturally correct to give a write access to the command here, but it is needed for possible optimizations.
	template<typename T>
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
	bool IsFinished() const { return IsFinishedCommandStatus(_Command->_Status); }
	bool IsAbandoned() const { return _Command->GetRefCount() == 1; }

	bool operator ==(const CCommandPromise& Other) const { return _Command == Other._Command; }
};

template<typename T, typename... TArgs>
[[nodiscard]] std::pair<CCommandFuture, CCommandPromise> CreateCommand(TArgs&&... Args)
{
	static_assert(std::is_base_of_v<CCommand, T>, "All AI commands must be derived from DEM::AI::CCommand");

	PCommand Cmd(new T());
	if constexpr (sizeof...(TArgs) > 0)
		static_cast<T*>(Cmd.Get())->SetPayload(std::forward<TArgs>(Args)...);
	return std::make_pair(CCommandFuture(*Cmd), CCommandPromise(*Cmd));
}
//---------------------------------------------------------------------

template<typename T, typename... TArgs>
bool UpdateCommand(CCommandFuture& Cmd, TArgs&&... Args)
{
	static_assert(sizeof...(TArgs) > 0, "There is no meaning in updating a command without payload params");

	if (auto* pTypedCmd = Cmd.As<T>())
	{
		pTypedCmd->SetPayload(std::forward<TArgs>(Args)...);
		return true;
	}

	return false;
}
//---------------------------------------------------------------------

}
