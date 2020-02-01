#pragma once
#include <type_traits>

// Helpers for member read and write access

namespace DEM::Meta
{

template<typename TClass, typename T, typename TAccessor, typename SFINAE_Enabled = void>
struct MemberAccess
{
	static inline const T* ConstPtr(TAccessor, const TClass&) { static_assert(false, "Invalid getter"); }
	static inline T*       Ptr(TAccessor, TClass&) { static_assert(false, "Invalid setter"); }
	static inline const T& ConstRef(TAccessor, const TClass&) { static_assert(false, "Invalid getter"); }
	static inline T&       Ref(TAccessor, TClass&) { static_assert(false, "Invalid setter"); }
	static inline T        Copy(TAccessor, TClass&) { static_assert(false, "Invalid getter"); }

	template<typename U>
	static inline void     Set(TAccessor, TClass&, U&&) { static_assert(false, "Invalid setter"); }
};

// Pointer-to-member specialization
template<typename TClass, typename T>
struct MemberAccess<TClass, T, T TClass::*>
{
	using TAccessor = T TClass::*;

	static inline const T* ConstPtr(TAccessor pGetter, const TClass& Instance) { return &Instance.*pGetter; }
	static inline T*       Ptr(TAccessor pSetter, TClass& Instance) { return &Instance.*pSetter; }
	static inline const T& ConstRef(TAccessor pGetter, const TClass& Instance) { return Instance.*pGetter; }
	static inline T&       Ref(TAccessor pSetter, TClass& Instance) { return Instance.*pSetter; }
	static inline T        Copy(TAccessor pGetter, TClass& Instance) { return Instance.*pGetter; }

	template<typename U>
	static inline void     Set(TAccessor pSetter, TClass& Instance, U&& Value) { Instance.*pSetter = std::forward<U>(Value); }
};

// Setter specialization
template<typename TClass, typename T, typename TAccessor>
struct MemberAccess<TClass, T, TAccessor,
	typename std::enable_if_t<
	std::is_same_v<TAccessor, void (TClass::*)(T)> ||
	std::is_same_v<TAccessor, void (TClass::*)(const T&)> ||
	std::is_same_v<TAccessor, void (TClass::*)(T&&)> ||
	std::is_same_v<TAccessor, void (TClass::*)(T) noexcept> ||
	std::is_same_v<TAccessor, void (TClass::*)(const T&) noexcept> ||
	std::is_same_v<TAccessor, void (TClass::*)(T&&) noexcept>>>
{
	static inline const T* ConstPtr(TAccessor, const TClass&) { static_assert(false, "Can't get value with a setter function"); }
	static inline T*       Ptr(TAccessor, TClass&) { static_assert(false, "Can't get value with a setter function"); }
	static inline const T& ConstRef(TAccessor, const TClass&) { static_assert(false, "Can't get value with a setter function"); }
	static inline T&       Ref(TAccessor, TClass&) { static_assert(false, "Can't get value with a setter function"); }
	static inline T        Copy(TAccessor, TClass&) { static_assert(false, "Can't get value with a setter function"); }

	template<typename U>
	static inline void     Set(TAccessor pSetter, TClass& Instance, U&& Value) { (Instance.*pSetter)(std::forward<U>(Value)); }
};

// Value getter specialization
template<typename TClass, typename T, typename TAccessor>
struct MemberAccess<TClass, T, TAccessor,
	typename std::enable_if_t<
	std::is_same_v<TAccessor, T (TClass::*)() const> ||
	std::is_same_v<TAccessor, T (TClass::*)()> ||
	std::is_same_v<TAccessor, T (TClass::*)() const noexcept> ||
	std::is_same_v<TAccessor, T (TClass::*)() noexcept>>>
{
	static inline const T* ConstPtr(TAccessor, const TClass&) { static_assert(false, "Can't get pointer with a value getter"); }
	static inline T*       Ptr(TAccessor, TClass&) { static_assert(false, "Can't get pointer with a value getter"); }
	static inline const T& ConstRef(TAccessor, const TClass&) { static_assert(false, "Can't get reference with a value getter"); }
	static inline T&       Ref(TAccessor, TClass&) { static_assert(false, "Can't get reference with a value getter"); }
	static inline T        Copy(TAccessor pGetter, TClass& Instance) { return (Instance.*pGetter)(); }

	template<typename U>
	static inline void     Set(TAccessor, TClass&, U&&) { static_assert(false, "Can't set value with a getter function"); }
};

// Const ref getter specialization
template<typename TClass, typename T, typename TAccessor>
struct MemberAccess<TClass, T, TAccessor,
	typename std::enable_if_t<
	std::is_same_v<TAccessor, const T& (TClass::*)() const> ||
	std::is_same_v<TAccessor, const T& (TClass::*)()> ||
	std::is_same_v<TAccessor, const T& (TClass::*)() const noexcept> ||
	std::is_same_v<TAccessor, const T& (TClass::*)() noexcept>>>
{
	static inline const T* ConstPtr(TAccessor pGetter, const TClass& Instance) { return &(Instance.*pGetter)(); }
	static inline T*       Ptr(TAccessor, TClass&) { static_assert(false, "Can't get mutable pointer with a const getter"); }
	static inline const T& ConstRef(TAccessor pGetter, const TClass& Instance) { return (Instance.*pGetter)(); }
	static inline T&       Ref(TAccessor, TClass&) { static_assert(false, "Can't get mutable reference with a const getter"); }
	static inline T        Copy(TAccessor pGetter, TClass& Instance) { return (Instance.*pGetter)(); }

	template<typename U>
	static inline void     Set(TAccessor, TClass&, U&&) { static_assert(false, "Can't set value with a getter function"); }
};

// Mutable ref getter specialization
template<typename TClass, typename T>
struct MemberAccess<TClass, T, T& (TClass::*)()>
{
	using TAccessor = T& (TClass::*)();

	static inline const T* ConstPtr(TAccessor pGetter, const TClass& Instance) { return &(Instance.*pGetter)(); }
	static inline T*       Ptr(TAccessor pSetter, TClass& Instance) { return &(Instance.*pSetter)(); }
	static inline const T& ConstRef(TAccessor pGetter, const TClass& Instance) { return (Instance.*pGetter)(); }
	static inline T&       Ref(TAccessor pSetter, TClass& Instance) { return (Instance.*pSetter)(); }
	static inline T        Copy(TAccessor pGetter, TClass& Instance) { return (Instance.*pGetter)(); }

	template<typename U>
	static inline void     Set(TAccessor pSetter, TClass& Instance, U&& Value) { (Instance.*pSetter)() = std::forward<U>(Value); }
};

}
