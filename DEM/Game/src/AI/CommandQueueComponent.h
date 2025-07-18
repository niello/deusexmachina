#pragma once
#include <AI/Command.h>
#include <Data/Metadata.h>
#include <deque>

// A queue for AI commands that serves as a source for the CCommandStackComponent.
// Useful when there is a hard sequence of actions which can be executed one after another,
// like a queue built via user input (Ctrl+Click), a script or a cutscene.

namespace DEM::AI
{

class CCommandQueueComponent final
{
protected:

	std::deque<CCommandPromise> _Queue;

public:

	CCommandQueueComponent() = default;
	CCommandQueueComponent(CCommandQueueComponent&&) noexcept = default;
	CCommandQueueComponent& operator =(CCommandQueueComponent&&) noexcept = default;
	~CCommandQueueComponent() = default;

	template<typename T, typename... TArgs>
	CCommandFuture EnqueueCommand(TArgs&&... Args)
	{
		auto [Future, Promise] = CreateCommand<T>(std::forward<TArgs>(Args)...);
		_Queue.push_back(std::move(Promise));
		return std::move(Future);
	}

	CCommandPromise Pop()
	{
		CCommandPromise Front = std::move(_Queue.front());
		_Queue.pop_front();
		return Front;
	}

	void Reset()
	{
		for (auto& Cmd : _Queue)
			Cmd.SetStatus(ECommandStatus::Cancelled);
		_Queue.clear();
	}

	bool IsEmpty() const { return _Queue.empty(); }
};

}

namespace DEM::Meta
{

template<> constexpr auto RegisterClassName<AI::CCommandQueueComponent>() { return "DEM::AI::CCommandQueueComponent"; }
template<> constexpr auto RegisterMembers<AI::CCommandQueueComponent>() { return std::make_tuple(); }

}
