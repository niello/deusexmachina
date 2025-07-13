#pragma once
#include <AI/Command.h>
#include <Data/Metadata.h>

// Character AI command stack of currently executing action and it's sub-actions.
// Produced by decision making or player input. Executed by different actuation systems (navigation etc).

namespace DEM::AI
{

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
	void PopCommand(const CCommandPromise& Cmd, ECommandStatus Status)
	{
		n_assert_dbg(IsTerminalCommandStatus(Status));

		// drop all children and then ourselves from the stack, setting the same status

		// root logging
		//::Sys::Log((EntityToString(EntityID) + ": " + RootAction.Get()->GetClassName() + " action succeeded\n").c_str());
		//::Sys::Log((EntityToString(EntityID) + ": " + RootAction.Get()->GetClassName() + " action failed or was cancelled\n").c_str());
	}

	// FindCurrent<T>? Or always work with top action? maybe not, because higher level
	// subsystem may change state, e.g. character became unable to navigate, but Steer doesn't know!
	//???only for cancellation? or for param updating too? params updating must happen only through a future held!!!

	//???should be able to get child or parent of an action by its future? need it? maybe yes, to control decomposition from the system.
	// GetCurrent needed? stack top.

	bool IsEmpty() const { return _CommandStack.empty(); }
};

}

namespace DEM::Meta
{

template<> constexpr auto RegisterClassName<AI::CCommandStackComponent>() { return "DEM::AI::CCommandStackComponent"; }
template<> constexpr auto RegisterMembers<AI::CCommandStackComponent>() { return std::make_tuple(); }

}
