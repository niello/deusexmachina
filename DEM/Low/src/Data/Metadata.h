#pragma once
#include <StdDEM.h>

// Provides type-safe introspection and serialization info for arbitrary objects
// Inspired by https://github.com/eliasdaler/MetaStuff

namespace DEM::Meta
{

// Specialize this for your types
template<typename T> inline constexpr auto RegisterMetadata() { return std::make_tuple(); }

template <typename TClass, typename T>
using member_ptr_t = T TClass::*;

template<typename TClass, typename T, typename TAccessor>
struct MemberAccess
{
	static inline const T* ConstPtr(TAccessor, const TClass&) { static_assert(false, "Invalid getter"); }
	static inline T*       Ptr(TAccessor, TClass&) { static_assert(false, "Invalid setter"); }
	static inline const T& ConstRef(TAccessor, const TClass&) { static_assert(false, "Invalid getter"); }
	static inline T&       Ref(TAccessor, TClass&) { static_assert(false, "Invalid setter"); }
	static inline T        Copy(TAccessor, TClass&) { static_assert(false, "Invalid getter"); }
};

template<typename TClass, typename T>
struct MemberAccess<TClass, T, member_ptr_t<TClass, T>>
{
	static inline const T* ConstPtr(member_ptr_t<TClass, T> pGetter, const TClass& Instance) { return &Instance.*pGetter; }
	static inline T*       Ptr(member_ptr_t<TClass, T> pSetter, TClass& Instance) { return &Instance.*pSetter; }
	static inline const T& ConstRef(member_ptr_t<TClass, T> pGetter, const TClass& Instance) { return Instance.*pGetter; }
	static inline T&       Ref(member_ptr_t<TClass, T> pSetter, TClass& Instance) { return Instance.*pSetter; }
	static inline T        Copy(member_ptr_t<TClass, T> pGetter, TClass& Instance) { return Instance.*pGetter; }
};

//  = std::nullptr_t
template<typename TClass, typename T, typename TGetter, typename TSetter>
class CMember final
{
	static_assert(!std::is_null_pointer_v<TGetter> || !std::is_null_pointer_v<TSetter>);

public:

	constexpr CMember(const char* pName, TGetter pGetter = nullptr, TSetter pSetter = nullptr) noexcept
		: _pName(pName), _pGetter(pGetter), _pSetter(pSetter)
	{
	}

	//CMember& Get(TGetter pGetter) { _pGetter = pGetter; }
	//CMember& Set(TSetter pSetter) { _pSetter = pSetter; }

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

private:

	const char* _pName = nullptr;
	TGetter     _pGetter = nullptr;
	TSetter     _pSetter = nullptr;
};

template<typename TClass, typename T, typename TGetter = std::nullptr_t, typename TSetter = std::nullptr_t>
CMember<TClass, T, TGetter, TSetter> Member(const char* pName, TGetter pGetter = nullptr, TSetter pSetter = nullptr)
{
	return CMember<TClass, T, TGetter, TSetter>(pName, pGetter, pSetter);
}

template<typename T, typename TMembers>
struct CMetadata
{
	static inline TMembers Members = RegisterMetadata<T>();
	//static const char* Name() { return registerName<T>(); }
};

}
