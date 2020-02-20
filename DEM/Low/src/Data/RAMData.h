#pragma once
#include <StdDEM.h>
#include <Data/Ptr.h>

// Different RAM data holders. All store raw memory pointers, but differently
// manage memory ownership, lifetime and destruction. These holders aren't
// suited for storing typed data, they must store raw bytes only.

namespace IO
{
	typedef Ptr<class IStream> PStream;
}

namespace Data
{
typedef std::unique_ptr<class IRAMData> PRAMData; // Can add refcount

class IRAMData
{
public:

	virtual ~IRAMData() = default;

	virtual void* GetPtr() = 0;
	virtual const void* GetConstPtr() const = 0;
};

class CRAMDataNotOwned : public IRAMData
{
private:

	void* _pData = nullptr;

public:

	CRAMDataNotOwned(void* pData) : _pData(pData) {}

	virtual ~CRAMDataNotOwned() override = default;

	virtual void* GetPtr() override { return _pData; }
	virtual const void* GetConstPtr() const override { return _pData; }
};

class CRAMDataNotOwnedImmutable : public IRAMData
{
private:

	const void* _pData = nullptr;

public:

	CRAMDataNotOwnedImmutable(const void* pData) : _pData(pData) {}

	virtual ~CRAMDataNotOwnedImmutable() override = default;

	virtual void* GetPtr() override { return nullptr; }
	virtual const void* GetConstPtr() const override { return _pData; }
};

class CRAMDataNew : public IRAMData
{
private:

	void* _pData = nullptr;

public:

	CRAMDataNew(UPTR Size) : _pData(Size ? n_new_array(char, Size) : nullptr) {}

	virtual ~CRAMDataNew() override { if (_pData) n_delete_array(_pData); }

	virtual void* GetPtr() override { return _pData; }
	virtual const void* GetConstPtr() const override { return _pData; }
};

class CRAMDataMalloc : public IRAMData
{
private:

	void* _pData = nullptr;

public:

	CRAMDataMalloc(UPTR Size) : _pData(Size ? n_malloc(Size) : nullptr) {}

	virtual ~CRAMDataMalloc() override { if (_pData) n_free(_pData); }

	virtual void* GetPtr() override { return _pData; }
	virtual const void* GetConstPtr() const override { return _pData; }
};

class CRAMDataMallocAligned : public IRAMData
{
private:

	void* _pData = nullptr;

public:

	CRAMDataMallocAligned(UPTR Size, UPTR Alignment) : _pData((Size && Alignment) ? n_malloc_aligned(Size, Alignment) : nullptr) {}

	virtual ~CRAMDataMallocAligned() override { if (_pData) n_free_aligned(_pData); }

	virtual void* GetPtr() override { return _pData; }
	virtual const void* GetConstPtr() const override { return _pData; }
};

class CRAMDataMappedStream : public IRAMData
{
private:

	IO::PStream	_Stream;
	void*		_pData = nullptr;

public:

	CRAMDataMappedStream(IO::PStream Stream);

	virtual ~CRAMDataMappedStream() override;

	virtual void* GetPtr() override { return _pData; }
	virtual const void* GetConstPtr() const override { return _pData; }
};

}
