#pragma once
#include <Data/Ptr.h>

// Class with simple refcounting mechanism.
// Paired with Ptr<> template class, which implements a smart pointer over this class.

namespace Data
{

class CRefCounted
{
private:

	U32 RefCount = 0; // volatile for threading?

public:

	virtual ~CRefCounted() { n_assert_dbg(!RefCount); }

	void	AddRef() { ++RefCount; } //!!!interlocked for threading!
	void	Release() { n_assert_dbg(RefCount > 0); if (--RefCount == 0) n_delete(this); } //!!!interlocked for threading!
	U32		GetRefCount() const { return RefCount; }
};

typedef Ptr<CRefCounted> PRefCounted;

}

inline void DEMPtrAddRef(Data::CRefCounted* p) noexcept { p->AddRef(); }
inline void DEMPtrRelease(Data::CRefCounted* p) noexcept { p->Release(); }
