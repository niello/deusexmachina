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

	//???or use index on stack instead of promise? Find will return index and will accept a starting index.
	//???or use some kind of iterator? or pointer? need to access command args with Find result but then use it back as an iterator.
	void PopCommand(CCommandStackHandle CmdHandle, ECommandStatus Status)
	{
		const auto StartIdx = CmdHandle._Index;

		n_assert_dbg(CmdHandle._pStack == &_CommandStack && StartIdx < _CommandStack.size() && IsTerminalCommandStatus(Status));

		for (size_t i = StartIdx; i < _CommandStack.size(); ++i)
			_CommandStack[i].SetStatus(Status); //???is cmd status is already terminal, should keep old value or set new?

		_CommandStack.resize(StartIdx);

		// TODO: pop root logging
		//if (_CommandStack.empty())
		//::Sys::Log((EntityToString(EntityID) + ": " + RootAction.Get()->GetClassName() + " action succeeded\n").c_str());
		//::Sys::Log((EntityToString(EntityID) + ": " + RootAction.Get()->GetClassName() + " action failed or was cancelled\n").c_str());
	}

	CCommandStackHandle GetTop()
	{
		return _CommandStack.empty() ? CCommandStackHandle{} : CCommandStackHandle(&_CommandStack, _CommandStack.size() - 1);
	}

	template<typename... T>
	CCommandStackHandle FindTopmostCommand(CCommandStackHandle BelowThis = {})
	{
		auto RIt = _CommandStack.crbegin();
		if (BelowThis && BelowThis._Index > 0)
		{
			n_assert_dbg(CmdHandle._pStack == &_CommandStack && BelowThis._Index < _CommandStack.size());
			RIt = std::reverse_iterator(std::next(_CommandStack.begin(), BelowThis._Index));
			if (RIt == _CommandStack.crend()) return {};
		}

		// Walk from nested sub-actions to the stack root
		for (; RIt != _CommandStack.crend(); ++RIt)
			if ((*RIt)->IsAnyOf<T...>())
				return CCommandStackHandle(&_CommandStack, static_cast<size_t>(std::distance(_CommandStack.begin(), std::prev(RIt.base()))));

		return {};
	}

	CCommandStackHandle Find(const CCommandPromise& Cmd)
	{
		auto It = std::find(_CommandStack.begin(), _CommandStack.end(), Cmd);
		return (It == _CommandStack.end()) ? CCommandStackHandle{} : CCommandStackHandle(&_CommandStack, (std::distance(_CommandStack.begin(), It)));
	}

	//???should be able to get child or parent of an action by its future? need it? maybe yes, to control decomposition from the system.
	//???or should store future of sub-action it fired?

	bool IsEmpty() const { return _CommandStack.empty(); }
};

}

namespace DEM::Meta
{

template<> constexpr auto RegisterClassName<AI::CCommandStackComponent>() { return "DEM::AI::CCommandStackComponent"; }
template<> constexpr auto RegisterMembers<AI::CCommandStackComponent>() { return std::make_tuple(); }

}
