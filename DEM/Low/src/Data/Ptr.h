#pragma once
#include <System/System.h>

// Intrusive smart pointer class.
// Can be used like a normal C++ pointer in most cases.

template<class T>
class Ptr
{
private:

	T* pObj = nullptr;

public:

	template<class U> friend class Ptr;

	constexpr Ptr() noexcept = default;

	Ptr(T* pOther): pObj(pOther) { if (pObj) DEMPtrAddRef(pObj); }

	Ptr(Ptr const& Other): pObj(Other.pObj) { if (pObj) DEMPtrAddRef(pObj); }
	template<class U>
	Ptr(Ptr<U> const& Other) : pObj(Other.Get()) { if (pObj) DEMPtrAddRef(pObj); }

	Ptr(Ptr&& Other) noexcept : pObj(Other.pObj) { Other.pObj = nullptr; }
	template<class U>
	Ptr(Ptr<U>&& Other) : pObj(Other.pObj) { Other.pObj = nullptr; }

	~Ptr() { if (pObj) DEMPtrRelease(pObj); }

	bool	IsValidPtr() const noexcept { return !!pObj; }
	bool	IsNullPtr() const noexcept { return !pObj; }
	T*		Get() const noexcept { return pObj; }
	void	Reset() { Ptr().Swap(*this); }

	void	Swap(Ptr& Other) noexcept
	{
		T* pTmp = pObj;
		pObj = Other.pObj;
		Other.pObj = pTmp;
	}

	Ptr& operator =(T* pOther) { Ptr(pOther).Swap(*this); return *this; }

	Ptr& operator =(Ptr const& Other) { Ptr(Other).Swap(*this); return *this; }
	template<class U>
	Ptr& operator =(Ptr<U> const& Other) { Ptr(Other).Swap(*this); return *this; }

	Ptr& operator =(Ptr&& Other) noexcept { Ptr(static_cast<Ptr&&>(Other)).Swap(*this); return *this; }
	template<class U>
	Ptr& operator =(Ptr<U>&& Other) noexcept { Ptr(static_cast<Ptr<U>&&>(Other)).Swap(*this); return *this; }

	T*		operator ->() const { n_assert(pObj); return pObj; }
	T&		operator *() const { n_assert(pObj); return *pObj; }
			operator T*() const { n_assert(pObj); return pObj; }
			operator bool() const noexcept { return !!pObj; }
};

template<class T, class U> inline bool operator ==(Ptr<T> const& a, Ptr<U> const& b) noexcept
{
	return a.Get() == b.Get();
}

template<class T, class U> inline bool operator !=(Ptr<T> const& a, Ptr<U> const& b) noexcept
{
	return a.Get() != b.Get();
}

template<class T, class U> inline bool operator ==(Ptr<T> const& a, U* b) noexcept
{
	return a.Get() == b;
}

template<class T, class U> inline bool operator !=(Ptr<T> const& a, U* b) noexcept
{
	return a.Get() != b;
}

template<class T, class U> inline bool operator ==(T* a, Ptr<U> const& b) noexcept
{
	return a == b.Get();
}

template<class T, class U> inline bool operator !=(T* a, Ptr<U> const& b) noexcept
{
	return a != b.Get();
}

template<class T> inline bool operator ==(Ptr<T> const& p, std::nullptr_t) noexcept
{
	return p.Get() == 0;
}

template<class T> inline bool operator ==(std::nullptr_t, Ptr<T> const& p) noexcept
{
	return p.Get() == 0;
}

template<class T> inline bool operator !=(Ptr<T> const& p, std::nullptr_t) noexcept
{
	return p.Get() != 0;
}

template<class T> inline bool operator !=(std::nullptr_t, Ptr<T> const& p) noexcept
{
	return p.Get() != 0;
}

template<class T> inline bool operator <(Ptr<T> const& a, Ptr<T> const& b) noexcept
{
	return std::less<T*>()(a.Get(), b.Get());
}
