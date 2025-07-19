#pragma once
#include <AI/Command.h>
#include <Data/Metadata.h>

// Character AI command stack of currently executing action and it's sub-actions.
// Produced by decision making or player input. Executed by different actuation systems (navigation etc).

namespace DEM::AI
{
class CCommandStackComponent;

// An opaque handle to CCommandPromise stored in CCommandStackComponent. Required for stack manipulation.
class CCommandStackHandle final
{
private:

	friend class CCommandStackComponent;

	std::vector<CCommandPromise>* _pStack = nullptr;
	size_t                        _Index = 0;

	CCommandStackHandle(std::vector<CCommandPromise>* pStack, size_t Index) : _pStack(pStack), _Index(Index) {}

public:

	CCommandStackHandle() = default;

	CCommandPromise& CCommandStackHandle::operator *() const
	{
		n_assert_dbg(_pStack && _Index < _pStack->size());
		return (*_pStack)[_Index];
	}

	CCommandPromise* CCommandStackHandle::operator ->() const
	{
		return (_pStack && _Index < _pStack->size()) ? &(*_pStack)[_Index] : nullptr;
	}

	operator bool() const noexcept { return !!_pStack; }
};

class CCommandStackComponent final
{
protected:

	std::vector<CCommandPromise> _CommandStack;
	std::vector<CCommandPromise> _PoppedCommands; // Commands that were running and are pending finalization

public:

	// For command cleanup
	static constexpr bool Signals = true;

	CCommandStackComponent() = default;
	CCommandStackComponent(CCommandStackComponent&&) noexcept = default;
	CCommandStackComponent& operator =(CCommandStackComponent&&) noexcept = default;

	~CCommandStackComponent()
	{
		n_assert_dbg(_CommandStack.empty() && _PoppedCommands.empty());
	}

	template<typename T, typename... TArgs>
	CCommandFuture PushCommand(TArgs&&... Args)
	{
		auto [Future, Promise] = CreateCommand<T>(std::forward<TArgs>(Args)...);
		_CommandStack.push_back(std::move(Promise));
		return std::move(Future);
	}

	void PushCommand(CCommandPromise&& Cmd) { _CommandStack.push_back(std::move(Cmd)); }

	void PopCommand(CCommandStackHandle CmdHandle, ECommandStatus Status, bool ForceRewriteFinishedStatus = false)
	{
		const auto StartIdx = CmdHandle._Index;

		n_assert_dbg(CmdHandle._pStack == &_CommandStack && StartIdx < _CommandStack.size() && IsTerminalCommandStatus(Status));

		// Unwind a stack from the top down to fill _PoppedCommands in the right order
		auto REnd = std::make_reverse_iterator(std::next(_CommandStack.begin(), StartIdx));
		for (auto RIt = _CommandStack.rbegin(); RIt != REnd; ++RIt)
		{
			auto& Cmd = *RIt;
			const bool IsTerminated = IsTerminalCommandStatus(Cmd.GetStatus());

			// Normally we don't want to rewrite status of an already finished command
			if (ForceRewriteFinishedStatus || !IsTerminated)
				Cmd.SetStatus(Status);

			// Remember the command for finalization by its system
			if (!IsTerminated) _PoppedCommands.push_back(std::move(Cmd));
		}

		// Can't use resize because it requires a default empty promise constructor even though growing never happens
		_CommandStack.erase(std::next(_CommandStack.begin(), StartIdx), _CommandStack.end());
	}

	void Reset(ECommandStatus Status, bool ForceRewriteFinishedStatus = false)
	{
		if (!_CommandStack.empty()) PopCommand(CCommandStackHandle(&_CommandStack, 0), Status, ForceRewriteFinishedStatus);
	}

	template<typename... T>
	CCommandStackHandle FindTopmostCommand(CCommandStackHandle BelowThis = {})
	{
		auto RIt = _CommandStack.crbegin();
		if (BelowThis && BelowThis._Index > 0)
		{
			n_assert_dbg(BelowThis._pStack == &_CommandStack && BelowThis._Index < _CommandStack.size());
			RIt = std::reverse_iterator(std::next(_CommandStack.begin(), BelowThis._Index));
			if (RIt == _CommandStack.crend()) return {};
		}

		// Walk from nested sub-actions to the stack root
		for (; RIt != _CommandStack.crend(); ++RIt)
			if ((*RIt).IsAnyOf<T...>())
				return CCommandStackHandle(&_CommandStack, static_cast<size_t>(std::distance(_CommandStack.cbegin(), std::prev(RIt.base()))));

		return {};
	}

	template<typename T, typename F = std::nullptr_t>
	void FinalizePoppedCommands(F Finalizer = {})
	{
		for (auto It = _PoppedCommands.begin(); It != _PoppedCommands.end(); /**/)
		{
			if (auto* pTypedCmd = It->As<T>())
			{
				if constexpr (!std::is_same_v<F, std::nullptr_t>)
					Finalizer(*pTypedCmd, It->GetStatus());
				It = _PoppedCommands.erase(It);
			}
			else
			{
				++It;
			}
		}
	}

	bool IsEmpty() const { return _CommandStack.empty(); }
};

template<typename T, typename... TArgs>
void PushOrUpdateCommand(CCommandStackComponent& CmdStack, CCommandFuture& Cmd, TArgs&&... Args)
{
	if (!Cmd)
	{
		Cmd = CmdStack.PushCommand<T>(std::forward<TArgs>(Args)...);
	}
	else if (!UpdateCommand<T>(Cmd, std::forward<TArgs>(Args)...))
	{
		// Cmd is of different type, must cancel it before replacing.
		// External system can handle empty Cmd in a way it wants.
		Cmd.RequestCancellation();
		Cmd = {};
	}
}
//---------------------------------------------------------------------

}

namespace DEM::Meta
{

template<> constexpr auto RegisterClassName<AI::CCommandStackComponent>() { return "DEM::AI::CCommandStackComponent"; }
template<> constexpr auto RegisterMembers<AI::CCommandStackComponent>() { return std::make_tuple(); }

}
