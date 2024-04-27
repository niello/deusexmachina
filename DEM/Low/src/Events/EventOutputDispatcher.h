#pragma once
#include <Events/EventOutput.h>
#include <Events/EventDispatcher.h>

// IEventOutput implementation with immediate event firing

namespace DEM::Events
{

class CEventOutputDispatcher : public IEventOutput
{
protected:

	::Events::CEventDispatcher& _Dispatcher;

public:

	CEventOutputDispatcher(::Events::CEventDispatcher& Dispatcher) : _Dispatcher(Dispatcher) {}

	virtual void OnEvent(CStrID ID, const Data::CData& Data, float TimeShift) override
	{
		if (Data.IsA<Data::PParams>())
		{
			_Dispatcher.FireEvent(ID, Data.GetValue<Data::PParams>());
		}
		else
		{
			Data::PParams P = n_new(Data::CParams(2));
			P->Set(CStrID("Data"), Data);
			P->Set(CStrID("TimeShift"), TimeShift);
			_Dispatcher.FireEvent(ID, std::move(P));
		}
	}
};

}
