#pragma once
#include <StdDEM.h>

// A signal that notifies an arbitrary number of callable handlers.
// Implements an Observer pattern. Use it for system decoupling, e.g. for an event system.

namespace DEM
{
template<typename T>
class CSignal;

template<typename TRet, typename... TArgs>
class CSignal<TRet(TArgs...)> final
{
protected:

	using THandler = std::function<TRet(TArgs...)>;

	std::vector<THandler> _Handlers; //???use list or custom data structure to support unsubscribe while iterating?

	//!!!can store free node pool if using a custom list!

public:

	CSignal() = default;
	CSignal(const CSignal&) = delete;
	CSignal(CSignal&&) noexcept = default;
	CSignal& operator =(const CSignal&) = delete;
	CSignal& operator =(CSignal&&) noexcept = default;
	~CSignal() = default;

	// connection [handle] = subscription
	//!!!return connection handle, not scoped. Scoped as a wrapper!
	//connection must store disconnect function, signal ptr and slot handle

	//!!!if returned handle is copied, all copies must know if any of them unsubscribed the connection from the signal!
	//or forbid connection copying, allow only moving!!!

	//all subscriptions may be inherited from an abstract base class (refcounted) to simplify storing them as one type

	//???subscription smart pointer IS a handle?
	//would be a problem to destroy it - external destroy = unsubscribe, internal destroy happens WHEN unsubscribing

	// Connection API: IsConnected, Disconnect - need to have a signal reference to disconnect
	// Scoped connection API: transparent creation from a simple connection handle

	//Subscribe(THandler&& Handler) //???return std::list iterator as a handle? make own list to allow checking iterator validity? CHandleList?
	//Unsubscribe

	void UnsubscribeAll() { _Handlers.clear(); }

	void Fire(TArgs... Args) const
	{
		for (auto&& Handler : std::as_const(Handlers))
			Handler(Args...);
	}

	void operator()(TArgs... Args) const
	{
		for (auto&& Handler : std::as_const(Handlers))
			Handler(Args...);
	}

	bool Empty() const { return _Handlers.empty(); }
	UPTR GetHandlerCount() const { return _Handlers.size(); }
};

}
