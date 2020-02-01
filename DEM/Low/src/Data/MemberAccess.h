#pragma once
#include <type_traits>

// Helpers for member read and write access

namespace DEM::Meta
{

template<typename TAccessor, typename TClass, typename T>
constexpr bool is_pointer_to_member_v =
	std::is_same_v<TAccessor, T TClass::*> ||
	std::is_same_v<TAccessor, T TClass::* const>;

template<typename TAccessor, typename TClass, typename T>
constexpr bool is_setter_v =
	std::is_same_v<TAccessor, void (TClass::*)(T)> ||
	std::is_same_v<TAccessor, void (TClass::*)(const T&)> ||
	std::is_same_v<TAccessor, void (TClass::*)(T&&)> ||
	std::is_same_v<TAccessor, void (TClass::*)(T) noexcept> ||
	std::is_same_v<TAccessor, void (TClass::*)(const T&) noexcept> ||
	std::is_same_v<TAccessor, void (TClass::*)(T&&) noexcept>;

template<typename TAccessor, typename TClass, typename T>
constexpr bool is_value_getter_v =
	std::is_same_v<TAccessor, T (TClass::*)() const> ||
	std::is_same_v<TAccessor, T (TClass::*)()> ||
	std::is_same_v<TAccessor, T (TClass::*)() const noexcept> ||
	std::is_same_v<TAccessor, T (TClass::*)() noexcept>;

template<typename TAccessor, typename TClass, typename T>
constexpr bool is_const_ref_getter_v =
	std::is_same_v<TAccessor, const T& (TClass::*)() const> ||
	std::is_same_v<TAccessor, const T& (TClass::*)()> ||
	std::is_same_v<TAccessor, const T& (TClass::*)() const noexcept> ||
	std::is_same_v<TAccessor, const T& (TClass::*)() noexcept>;

template<typename TAccessor, typename TClass, typename T>
constexpr bool is_mutable_ref_getter_v =
	std::is_same_v<TAccessor, T& (TClass::*)()> ||
	std::is_same_v<TAccessor, T& (TClass::*)() noexcept>;

template<typename TClass, typename T, typename TAccessor, typename SFINAE_Enabled = void>
struct MemberAccess
{
	static inline const T* ConstPtr(TAccessor, const TClass&) { static_assert(false, "Unsupported getter type"); }
	static inline T*       Ptr(TAccessor, TClass&) { static_assert(false, "Unsupported setter type"); }
	static inline const T& ConstRef(TAccessor, const TClass&) { static_assert(false, "Unsupported getter type"); }
	static inline T&       Ref(TAccessor, TClass&) { static_assert(false, "Unsupported setter type"); }
	static inline T        Copy(TAccessor, TClass&) { static_assert(false, "Unsupported getter type"); }

	template<typename U>
	static inline void     Set(TAccessor, TClass&, U&&) { static_assert(false, "Unsupported setter type"); }

	static inline auto     BestGetConst(TAccessor, TClass&) { static_assert(false, "Unsupported getter type"); }
	static inline auto     BestGet(TAccessor, TClass&) { static_assert(false, "Unsupported setter type"); }
};

// Pointer-to-member specialization
template<typename TClass, typename T, typename TAccessor>
struct MemberAccess<TClass, T, TAccessor, typename std::enable_if_t<is_pointer_to_member_v<TAccessor, TClass, T>>>
{
	static inline const T* ConstPtr(TAccessor pGetter, const TClass& Instance) { return &Instance.*pGetter; }
	static inline T*       Ptr(TAccessor pSetter, TClass& Instance) { return &Instance.*pSetter; }
	static inline const T& ConstRef(TAccessor pGetter, const TClass& Instance) { return Instance.*pGetter; }
	static inline T&       Ref(TAccessor pSetter, TClass& Instance) { return Instance.*pSetter; }
	static inline T        Copy(TAccessor pGetter, TClass& Instance) { return Instance.*pGetter; }

	template<typename U>
	static inline void     Set(TAccessor pSetter, TClass& Instance, U&& Value) { Instance.*pSetter = std::forward<U>(Value); }

	static inline const T& BestGetConst(TAccessor pGetter, TClass& Instance) { return ConstRef(pGetter, Instance); }
	static inline T&       BestGet(TAccessor pSetter, TClass& Instance) { return Ref(pSetter, Instance); }
};

// Setter specialization
template<typename TClass, typename T, typename TAccessor>
struct MemberAccess<TClass, T, TAccessor, typename std::enable_if_t<is_setter_v<TAccessor, TClass, T>>>
{
	static inline const T* ConstPtr(TAccessor, const TClass&) { static_assert(false, "Can't get value with a setter function"); }
	static inline T*       Ptr(TAccessor, TClass&) { static_assert(false, "Can't get value with a setter function"); }
	static inline const T& ConstRef(TAccessor, const TClass&) { static_assert(false, "Can't get value with a setter function"); }
	static inline T&       Ref(TAccessor, TClass&) { static_assert(false, "Can't get value with a setter function"); }
	static inline T        Copy(TAccessor, TClass&) { static_assert(false, "Can't get value with a setter function"); }

	template<typename U>
	static inline void     Set(TAccessor pSetter, TClass& Instance, U&& Value) { (Instance.*pSetter)(std::forward<U>(Value)); }

	static inline auto     BestGetConst(TAccessor, TClass&) { static_assert(false, "Can't get value with a setter function"); }
	static inline auto     BestGet(TAccessor, TClass&) { static_assert(false, "Can't get value with a setter function"); }
};

// Value getter specialization
template<typename TClass, typename T, typename TAccessor>
struct MemberAccess<TClass, T, TAccessor, typename std::enable_if_t<is_value_getter_v<TAccessor, TClass, T>>>
{
	static inline const T* ConstPtr(TAccessor, const TClass&) { static_assert(false, "Can't get pointer with a value getter"); }
	static inline T*       Ptr(TAccessor, TClass&) { static_assert(false, "Can't get pointer with a value getter"); }
	static inline const T& ConstRef(TAccessor, const TClass&) { static_assert(false, "Can't get reference with a value getter"); }
	static inline T&       Ref(TAccessor, TClass&) { static_assert(false, "Can't get reference with a value getter"); }
	static inline T        Copy(TAccessor pGetter, TClass& Instance) { return (Instance.*pGetter)(); }

	template<typename U>
	static inline void     Set(TAccessor, TClass&, U&&) { static_assert(false, "Can't set value with a getter function"); }

	static inline T        BestGetConst(TAccessor pGetter, TClass& Instance) { return Copy(pGetter, Instance); }
	static inline auto     BestGet(TAccessor, TClass&) { static_assert(false, "Can't get reference with a value getter"); }
};

// Const ref getter specialization
template<typename TClass, typename T, typename TAccessor>
struct MemberAccess<TClass, T, TAccessor, typename std::enable_if_t<is_const_ref_getter_v<TAccessor, TClass, T>>>
{
	static inline const T* ConstPtr(TAccessor pGetter, const TClass& Instance) { return &(Instance.*pGetter)(); }
	static inline T*       Ptr(TAccessor, TClass&) { static_assert(false, "Can't get mutable pointer with a const getter"); }
	static inline const T& ConstRef(TAccessor pGetter, const TClass& Instance) { return (Instance.*pGetter)(); }
	static inline T&       Ref(TAccessor, TClass&) { static_assert(false, "Can't get mutable reference with a const getter"); }
	static inline T        Copy(TAccessor pGetter, TClass& Instance) { return (Instance.*pGetter)(); }

	template<typename U>
	static inline void     Set(TAccessor, TClass&, U&&) { static_assert(false, "Can't set value with a getter function"); }

	static inline const T& BestGetConst(TAccessor pGetter, TClass& Instance) { return ConstRef(pGetter, Instance); }
	static inline auto     BestGet(TAccessor, TClass&) { static_assert(false, "Can't get mutable reference with a const getter"); }
};

// Mutable ref getter specialization
template<typename TClass, typename T, typename TAccessor>
struct MemberAccess<TClass, T, TAccessor, typename std::enable_if_t<is_mutable_ref_getter_v<TAccessor, TClass, T>>>
{
	static inline const T* ConstPtr(TAccessor pGetter, const TClass& Instance) { return &(Instance.*pGetter)(); }
	static inline T*       Ptr(TAccessor pSetter, TClass& Instance) { return &(Instance.*pSetter)(); }
	static inline const T& ConstRef(TAccessor pGetter, const TClass& Instance) { return (Instance.*pGetter)(); }
	static inline T&       Ref(TAccessor pSetter, TClass& Instance) { return (Instance.*pSetter)(); }
	static inline T        Copy(TAccessor pGetter, TClass& Instance) { return (Instance.*pGetter)(); }

	template<typename U>
	static inline void     Set(TAccessor pSetter, TClass& Instance, U&& Value) { (Instance.*pSetter)() = std::forward<U>(Value); }

	static inline const T& BestGetConst(TAccessor pGetter, TClass& Instance) { return ConstRef(pGetter, Instance); }
	static inline T&       BestGet(TAccessor pSetter, TClass& Instance) { return Ref(pSetter, Instance); }
};

}
