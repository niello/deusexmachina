#pragma once
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
typedef std::unique_ptr<class IBuffer> PBuffer; // Can add refcount

class IBuffer
{
public:

	virtual ~IBuffer() = default;

	virtual void* GetPtr() = 0;
	virtual const void* GetConstPtr() const = 0;
	virtual UPTR GetSize() const = 0;
	virtual bool Resize(UPTR NewSize) { FAIL; }
	virtual bool IsOwning() const = 0;
};

class CBufferNotOwned : public IBuffer
{
private:

	void* _pData = nullptr;

public:

	CBufferNotOwned(void* pData) : _pData(pData) {}

	virtual ~CBufferNotOwned() override = default;

	virtual void* GetPtr() override { return _pData; }
	virtual const void* GetConstPtr() const override { return _pData; }
	virtual UPTR GetSize() const { return 0; } // FIXME: size?
	virtual bool IsOwning() const { return false; }
};

class CBufferNotOwnedImmutable : public IBuffer
{
private:

	const void* _pData = nullptr;

public:

	CBufferNotOwnedImmutable(const void* pData) : _pData(pData) {}

	virtual ~CBufferNotOwnedImmutable() override = default;

	virtual void* GetPtr() override { return nullptr; }
	virtual const void* GetConstPtr() const override { return _pData; }
	virtual UPTR GetSize() const { return 0; } // FIXME: size?
	virtual bool IsOwning() const { return false; }
};

class CBufferMalloc : public IBuffer
{
private:

	void* _pData = nullptr;
	UPTR _Size = 0;

public:

	CBufferMalloc(UPTR Size) : _pData(Size ? n_malloc(Size) : nullptr), _Size(Size) {}

	virtual ~CBufferMalloc() override { if (_pData) n_free(_pData); }

	virtual void* GetPtr() override { return _pData; }
	virtual const void* GetConstPtr() const override { return _pData; }
	virtual UPTR GetSize() const { return _Size; }
	virtual bool IsOwning() const { return true; }
};

class CBufferMallocAligned : public IBuffer
{
private:

	void* _pData = nullptr;
	UPTR _Size = 0;

public:

	CBufferMallocAligned(UPTR Size, UPTR Alignment) : _pData((Size && Alignment) ? n_malloc_aligned(Size, Alignment) : nullptr), _Size(Size) {}

	virtual ~CBufferMallocAligned() override { if (_pData) n_free_aligned(_pData); }

	virtual void* GetPtr() override { return _pData; }
	virtual const void* GetConstPtr() const override { return _pData; }
	virtual UPTR GetSize() const { return _Size; }
	virtual bool IsOwning() const { return true; }
};

class CBufferMappedStream : public IBuffer
{
private:

	IO::PStream	_Stream;
	void*		_pData = nullptr;

public:

	CBufferMappedStream(IO::PStream Stream);

	virtual ~CBufferMappedStream() override;

	virtual void* GetPtr() override { return _pData; }
	virtual const void* GetConstPtr() const override { return _pData; }
	virtual UPTR GetSize() const;
	virtual bool IsOwning() const { return true; } // Because it holds the stream
};

}
