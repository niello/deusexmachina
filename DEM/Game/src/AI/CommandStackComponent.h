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

public:

	CCommandStackComponent() = default;
	CCommandStackComponent(CCommandStackComponent&&) noexcept = default;
	CCommandStackComponent& operator =(CCommandStackComponent&&) noexcept = default;
	~CCommandStackComponent() = default;

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

		for (size_t i = StartIdx; i < _CommandStack.size(); ++i)
			if (ForceRewriteFinishedStatus || !_CommandStack[i].IsFinished())
				_CommandStack[i].SetStatus(Status);

		// Can't use resize because it requires a default empty promise constructor even though growing never happens
		_CommandStack.erase(std::next(_CommandStack.begin(), StartIdx), _CommandStack.end());

		// TODO: pop root logging
		//if (_CommandStack.empty())
		//::Sys::Log((EntityToString(EntityID) + ": " + RootAction.Get()->GetClassName() + " action succeeded\n").c_str());
		//::Sys::Log((EntityToString(EntityID) + ": " + RootAction.Get()->GetClassName() + " action failed or was cancelled\n").c_str());
	}

	void Reset(ECommandStatus Status, bool ForceRewriteFinishedStatus = false)
	{
		PopCommand(CCommandStackHandle(&_CommandStack, 0), Status, ForceRewriteFinishedStatus);
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

	CCommandStackHandle Find(const CCommandPromise& Cmd)
	{
		auto It = std::find(_CommandStack.cbegin(), _CommandStack.cend(), Cmd);
		return (It == _CommandStack.cend()) ? CCommandStackHandle{} : CCommandStackHandle(&_CommandStack, (std::distance(_CommandStack.cbegin(), It)));
	}

	//???should be able to get child or parent of an action by its future? need it? maybe yes, to control decomposition from the system.
	//???or should store future of sub-action it fired?

	bool IsEmpty() const { return _CommandStack.empty(); }
};

template<typename T, typename... TArgs>
void PushOrUpdateCommand(CCommandStackComponent& CmdStack, CCommandFuture& Cmd, TArgs&&... Args)
{
	if (!Cmd)
	{
		Cmd = CmdStack.PushCommand<Steer>(std::forward<TArgs>(Args)...);
	}
	else if (!UpdateCommand<Steer>(Cmd, std::forward<TArgs>(Args)...))
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
