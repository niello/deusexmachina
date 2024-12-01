#pragma once
#include <memory>

// A base class for event or signal connections. Connection holds a subscription,
// tracks reference count and unsubscribes when the last reference is lost.

namespace DEM::Events
{

struct CConnectionRecordBase
{
	// TODO: strong and weak counters for intrusive?
	uint16_t ConnectionCount = 0; //???TODO: could check existense of weak_ptrs instead?

	virtual bool IsConnected() const noexcept = 0;
	virtual void Disconnect() = 0;
};

class CConnection
{
protected:

	std::weak_ptr<CConnectionRecordBase> _Record;

public:

	CConnection() noexcept = default;

	CConnection(std::weak_ptr<CConnectionRecordBase>&& Record) noexcept
		: _Record(std::move(Record))
	{
		if (auto SharedRecord = _Record.lock())
			++SharedRecord->ConnectionCount;
	}

	CConnection(const CConnection& Other) noexcept
		: _Record(Other._Record)
	{
		if (auto SharedRecord = _Record.lock())
			++SharedRecord->ConnectionCount;
	}

	CConnection(CConnection&& Other) noexcept
		: _Record(std::move(Other._Record))
	{
		Other._Record.reset();
	}

	~CConnection()
	{
		// Check if the last connection was disconnected
		//???only CScopedConnection must auto-disconnect?
		const auto SharedRecord = _Record.lock();
		if (SharedRecord && --SharedRecord->ConnectionCount == 0)
			SharedRecord->Disconnect();
	}

	CConnection& operator =(const CConnection& Other) noexcept
	{
		if (this == &Other) return *this;

		const auto OldSharedRecord = _Record.lock();
		const auto NewSharedRecord = Other._Record.lock();
		if (OldSharedRecord == NewSharedRecord) return *this;

		if (NewSharedRecord) ++NewSharedRecord->ConnectionCount;

		if (OldSharedRecord && --OldSharedRecord->ConnectionCount == 0)
			OldSharedRecord->Disconnect();

		_Record = Other._Record;

		return *this;
	}

	CConnection& operator =(CConnection&& Other) noexcept
	{
		if (this == &Other) return *this;

		const auto OldSharedRecord = _Record.lock();
		if (OldSharedRecord != Other._Record.lock())
		{
			if (OldSharedRecord && --OldSharedRecord->ConnectionCount == 0)
				OldSharedRecord->Disconnect();

			_Record = std::move(Other._Record);
		}

		Other._Record.reset();

		return *this;
	}

	bool IsConnected() const noexcept
	{
		const auto SharedRecord = _Record.lock();
		return SharedRecord && SharedRecord->IsConnected();
	}

	void Disconnect()
	{
		if (auto SharedRecord = _Record.lock())
		{
			--SharedRecord->ConnectionCount;
			SharedRecord->Disconnect();
		}
		_Record.reset();
	}

	operator bool() const noexcept { return IsConnected(); }
};

}
