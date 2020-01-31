#pragma once
#include <StdDEM.h>

// Provides type-safe introspection and serialization info for arbitrary objects
// Inspired by https://github.com/eliasdaler/MetaStuff

namespace DEM::Meta
{

// Specialize this for your types
template<typename T> inline constexpr auto RegisterMetadata() { return std::make_tuple(); }

// Access this to work with registered types
template<typename T, typename TMembers>
struct CMetadata
{
	// TODO: CMetadata is a static object itself, members, name, isregistered etc are fields

	static inline TMembers Members = RegisterMetadata<T>();
	//static const char* Name() { return registerName<T>(); }
};

template<typename TClass, typename T, typename TAccessor>
struct MemberAccess
{
	static inline const T* ConstPtr(TAccessor, const TClass&) { static_assert(false, "Invalid getter"); }
	static inline T*       Ptr(TAccessor, TClass&) { static_assert(false, "Invalid setter"); }
	static inline const T& ConstRef(TAccessor, const TClass&) { static_assert(false, "Invalid getter"); }
	static inline T&       Ref(TAccessor, TClass&) { static_assert(false, "Invalid setter"); }
	static inline T        Copy(TAccessor, TClass&) { static_assert(false, "Invalid getter"); }
};

// FIXME:
//!!!void Set(const T& / T&& / T)!
template<typename TClass, typename T, typename TAccessor,
	typename std::enable_if<std::is_arithmetic<TAccessor>::value>::type* = nullptr> // Setter specialization
struct MemberAccess
{
	static inline const T* ConstPtr(TAccessor, const TClass&) { static_assert(false, "Invalid getter"); }
	static inline T*       Ptr(TAccessor, TClass&) { static_assert(false, "Invalid setter"); }
	static inline const T& ConstRef(TAccessor, const TClass&) { static_assert(false, "Invalid getter"); }
	static inline T&       Ref(TAccessor, TClass&) { static_assert(false, "Invalid setter"); }
	static inline T        Copy(TAccessor, TClass&) { static_assert(false, "Invalid getter"); }
};

template<typename TClass, typename T>
struct MemberAccess<TClass, T, T TClass::*> // Pointer-to-member specialization
{
	using TAccessor = T TClass::*;

	static inline const T* ConstPtr(TAccessor pGetter, const TClass& Instance) { return &Instance.*pGetter; }
	static inline T*       Ptr(TAccessor pSetter, TClass& Instance) { return &Instance.*pSetter; }
	static inline const T& ConstRef(TAccessor pGetter, const TClass& Instance) { return Instance.*pGetter; }
	static inline T&       Ref(TAccessor pSetter, TClass& Instance) { return Instance.*pSetter; }
	static inline T        Copy(TAccessor pGetter, TClass& Instance) { return Instance.*pGetter; }
};

template<typename TClass, typename T>
struct MemberAccess<TClass, T, const T& (TClass::*)() const> // Const ref getter specialization
{
	using TAccessor = const T& (TClass::*)() const;

	static inline const T* ConstPtr(TAccessor pGetter, const TClass& Instance) { return &(Instance.*pGetter)(); }
	static inline T*       Ptr(TAccessor, TClass&) { static_assert(false, "Invalid setter"); }
	static inline const T& ConstRef(TAccessor pGetter, const TClass& Instance) { return (Instance.*pGetter)(); }
	static inline T&       Ref(TAccessor, TClass&) { static_assert(false, "Invalid setter"); }
	static inline T        Copy(TAccessor pGetter, TClass& Instance) { return (Instance.*pGetter)(); }
};

template<typename TClass, typename T>
struct MemberAccess<TClass, T, T& (TClass::*)()> // Mutable ref getter specialization
{
	using TAccessor = T& (TClass::*)();

	static inline const T* ConstPtr(TAccessor pGetter, const TClass& Instance) { return &(Instance.*pGetter)(); }
	static inline T*       Ptr(TAccessor pSetter, TClass& Instance) { return &(Instance.*pSetter)(); }
	static inline const T& ConstRef(TAccessor pGetter, const TClass& Instance) { return (Instance.*pGetter)(); }
	static inline T&       Ref(TAccessor pSetter, TClass& Instance) { return (Instance.*pSetter)(); }
	static inline T        Copy(TAccessor pGetter, TClass& Instance) { return (Instance.*pGetter)(); }
};

template<typename TClass, typename T>
struct MemberAccess<TClass, T, T (TClass::*)() const> // Value getter specialization
{
	using TAccessor = T (TClass::*)() const;

	static inline const T* ConstPtr(TAccessor, const TClass&) { static_assert(false, "Invalid getter"); }
	static inline T*       Ptr(TAccessor, TClass&) { static_assert(false, "Invalid setter"); }
	static inline const T& ConstRef(TAccessor, const TClass&) { static_assert(false, "Invalid getter"); }
	static inline T&       Ref(TAccessor, TClass&) { static_assert(false, "Invalid setter"); }
	static inline T        Copy(TAccessor pGetter, TClass& Instance) { return (Instance.*pGetter)(); }
};

template<typename TClass, typename T, typename TGetter, typename TSetter>
class CMember final
{
	static_assert(!std::is_null_pointer_v<TGetter> || !std::is_null_pointer_v<TSetter>, "Inacessible members are not allowed, please add setter or getter");

public:

	constexpr CMember(const char* pName, TGetter pGetter = nullptr, TSetter pSetter = nullptr) noexcept
		: _pName(pName), _pGetter(pGetter), _pSetter(pSetter)
	{
	}

	//CMember& Get(TGetter pGetter) { _pGetter = pGetter; }
	//CMember& Set(TSetter pSetter) { _pSetter = pSetter; }
	// TODO: specials like SetRange for numerics

	constexpr const T* GetConstValuePtr(const TClass& Instance) const
	{
		static_assert(!std::is_same_v<TGetter, nullptr_t>, "Member is write-only");
		return MemberAccess<TClass, T, TGetter>::ConstPtr(_pGetter, Instance);
	}

	constexpr T* GetValuePtr(TClass& Instance) const
	{
		static_assert(!std::is_same_v<TSetter, nullptr_t>, "Member is read-only");
		return MemberAccess<TClass, T, TSetter>::Ptr(_pSetter, Instance);
	}

	constexpr const T& GetConstValueRef(const TClass& Instance) const
	{
		static_assert(!std::is_same_v<TGetter, nullptr_t>, "Member is write-only");
		return MemberAccess<TClass, T, TGetter>::ConstRef(_pGetter, Instance);
	}

	constexpr T& GetValueRef(TClass& Instance) const
	{
		static_assert(!std::is_same_v<TSetter, nullptr_t>, "Member is read-only");
		return MemberAccess<TClass, T, TSetter>::Ref(_pSetter, Instance);
	}

	constexpr T GetValueCopy(TClass& Instance) const
	{
		static_assert(!std::is_same_v<TGetter, nullptr_t>, "Member is write-only");
		return MemberAccess<TClass, T, TGetter>::Copy(_pGetter, Instance);
	}

	//set value &&

private:

	const char* _pName = nullptr;
	TGetter     _pGetter = nullptr;
	TSetter     _pSetter = nullptr;
};

template<typename TClass, typename T, typename TGetter = std::nullptr_t, typename TSetter = std::nullptr_t>
inline CMember<TClass, T, TGetter, TSetter> Member(const char* pName, TGetter pGetter = nullptr, TSetter pSetter = nullptr)
{
	return CMember<TClass, T, TGetter, TSetter>(pName, pGetter, pSetter);
}

template<typename TClass, typename T>
inline CMember<TClass, T, T TClass::*, std::nullptr_t> Member(const char* pName, T TClass::* pGetter)
{
	return CMember<TClass, T, T TClass::*, std::nullptr_t>(pName, pGetter, nullptr);
}

template<typename TClass, typename T>
inline CMember<TClass, T, T TClass::*, T TClass::*> Member(const char* pName, T TClass::* pGetter, T TClass::* pSetter)
{
	return CMember<TClass, T, T TClass::*, T TClass::*>(pName, pGetter, pSetter);
}

}
