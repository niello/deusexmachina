#pragma once
#include <Events/EventOutput.h>
#include <queue>

// IEventOutput implementation with a queue buffer

namespace DEM::Events
{

class CEventOutputBuffer : public IEventOutput
{
public:

	struct CEventRecord
	{
		CStrID      ID;
		Data::CData Data;
		float       TimeShift;
	};

protected:

	std::queue<CEventRecord> _Queue;

public:

	bool PopFront(CEventRecord& Out)
	{
		if (_Queue.empty()) return false;
		Out = std::move(_Queue.front());
		_Queue.pop();
		return true;
	}

	virtual void OnEvent(CStrID ID, const Data::CData& Data, float TimeShift) override
	{
		_Queue.push(CEventRecord{ ID, Data, TimeShift });
	}
};

}
