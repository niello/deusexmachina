#pragma once
#include <Data/MemberAccess.h>
#include <Data/Hash.h> // for auto-generated field codes
#include <cstdint>
#include <string>

// Provides type-safe introspection and serialization info for arbitrary objects

// TODO: CMetadata is a static object itself. Members, Name, IsRegistered etc are fields. Register(){ return Meta<T>("name").Members(a, b, c, ...) }
// TODO: CMember template deduction guides to replace multiple trivial Member(...) factory functions
// TODO: nicer Extras. CMember(n, g, s, Ex<T>().Min().Max())? or CMemberBase + per-type overloads with embedded extras?
// TODO: mark empty exras with [[no_unique_address]] in C++20?
// TODO: avoid duplicating Member() functions for Code+Name and Name-only? One constructor? Code is optional.
// TODO: add free function accessors for non-class types? is really needed? CMetadata<int64_t> for example, but what for?
// TODO: compile-time member access by name, like constexpr size_t CMetadata<T>::MemberName = MemberIndex or like this. Is possible?
// TODO: implicit support for class hierarchies (or use composition)? Now each meta must register its members even if its base class meta already declared.

namespace DEM::Meta
{

constexpr std::uint32_t NO_MEMBER_CODE = std::numeric_limits<std::uint32_t>::max();

// Specialize this for your types
template<typename T> constexpr auto RegisterMembers() { return false; } // return std::tuple instead for your types
template<typename T> constexpr auto RegisterClassName() { return "<no class name specified>"; } // typeid(T).name()

// Specialize this to add more meta info to your types
template<typename T> struct CTypeMetadata {};

// TODO: move to more appropriate header?
template<typename T, typename U> inline bool IsEqualByValue(const T& a, const U& b)
{
	if constexpr (std::is_same_v<T, U> && std::is_array_v<T>)
		return !std::memcmp(a, b, sizeof(T));
	else
		return a == b;
}

// Access this to work with registered types
template<typename T>
class CMetadata final
{
private:

	static constexpr auto _Members = RegisterMembers<T>();

public:

	// Template sets _Members to const bool false, specializations must use std::tuple
	static constexpr bool IsRegistered = !std::is_same_v<decltype(_Members), const bool>;

	static constexpr auto   GetClassName() { return RegisterClassName<T>(); }
	template<size_t Index>
	static constexpr auto   GetMember() { return std::get<Index>(_Members); }
	static constexpr size_t GetMemberCount() { return std::tuple_size_v<decltype(_Members)>; }

	template<typename TCallback>
	static constexpr void ForEachMember(TCallback Callback)
	{
		std::apply([Callback](auto& ...Members) { (..., Callback(Members)); }, _Members);
	}

	static constexpr bool HasMember(std::string_view Name)
	{
		return std::apply([Name](auto& ...Members) { return (... || (Members.GetName() == Name)); }, _Members);
	}

	static constexpr bool HasMember(std::uint32_t Code)
	{
		return std::apply([Code](auto& ...Members) { return (... || (Members.GetCode() == Code)); }, _Members);
	}

	static constexpr bool IsEqual(const T& a, const T& b)
	{
		return std::apply([&a, &b](auto& ...Members) { return (... && IsEqualByValue(Members.GetConstValue(a), Members.GetConstValue(b))); }, _Members);
	}

	static constexpr void Copy(const T& From, T& To)
	{
		std::apply([&From, &To](auto& ...Members) { (..., Members.SetValue(To, Members.GetConstValue(From))); }, _Members);
	}

	// TODO: TSetter doesn't allow to get mutable pointer and alter the source this way. Need to improve.
	//static constexpr void Move(T&& From, T& To)
	//{
	//	std::apply([&From, &To](auto& ...Members) { (..., Members.SetValue(To, std::move(Members.GetValueRef(From)))); }, _Members);
	//}

	template<typename TCallback>
	static constexpr bool WithMember(std::string_view Name, TCallback Callback)
	{
		bool Found = false;
		ForEachMember([Name, Callback, &Found](const auto& Member)
		{
			if (!Found && Member.GetName() == Name)
			{
				Callback(Member);
				Found = true;
			}
		});
		return Found;
	}

	template<typename TCallback>
	static constexpr bool WithMember(std::uint32_t Code, TCallback Callback)
	{
		bool Found = false;
		ForEachMember([Code, Callback, &Found](const auto& Member)
		{
			if (!Found && Member.GetCode() == Code)
			{
				Callback(Member);
				Found = true;
			}
		});
		return Found;
	}

	// TODO: how to invoke automatically on _Members init?
	static constexpr bool ValidateMembers()
	{
		// Check that field codes are unique. If uniqueness is broken by auto-generation via Hash,
		// change the field name or provide the code explicitly via DEM_META_MEMBER_FIELD.
		bool Result = true;
		ForEachMember([&Result](const auto& MemberA)
		{
			ForEachMember([&Result, &MemberA](const auto& MemberB)
			{
				// Early exit if already failed or comparing with itself
				if (!Result || (void*)&MemberA == (void*)&MemberB) return;

				if (MemberA.GetCode() != NO_MEMBER_CODE && MemberA.GetCode() == MemberB.GetCode())
					Result = false;
				else if (MemberA.GetName() == MemberB.GetName())
					Result = false;
			});
		});
		return Result;
	}
};

template<typename TMember>
using TMemberValue = typename std::decay_t<TMember>::TValue;

// Interface to a registered member metadata
template<typename TClass, typename T, typename TGetter, typename TSetter>
class CMember final
{
	static_assert(!std::is_null_pointer_v<TGetter> || !std::is_null_pointer_v<TSetter>, "Inacessible members are not allowed, please add setter or getter");

public:

	using TValue = T;

	constexpr CMember(const char* pName, TGetter pGetter = nullptr, TSetter pSetter = nullptr) noexcept
		: _pName(pName), _pGetter(pGetter), _pSetter(pSetter)
	{
	}

	constexpr CMember(std::uint32_t Code, const char* pName, TGetter pGetter = nullptr, TSetter pSetter = nullptr) noexcept
		: _Code(Code), _pName(pName), _pGetter(pGetter), _pSetter(pSetter)
	{
	}

	constexpr CMember&    Extras(CTypeMetadata<T>&& Value) { _Extras = std::move(Value); return *this; }
	constexpr auto&       Extras() const { return _Extras; }
	static constexpr bool CanRead() { return !std::is_same_v<TGetter, std::nullptr_t>; }
	static constexpr bool CanWrite() { return !std::is_same_v<TSetter, std::nullptr_t>; }
	static constexpr bool CanGetWritableRef() { return !is_setter_v<TSetter, TClass, T>; }
	template<typename U>
	static constexpr bool Is() { return std::is_same_v<T, U>; }

	constexpr const T* GetConstValuePtr(const TClass& Instance) const
	{
		static_assert(CanRead(), "Member is write-only");
		return MemberAccess<TClass, T, TGetter>::ConstPtr(_pGetter, Instance);
	}

	constexpr T* GetValuePtr(TClass& Instance) const
	{
		static_assert(CanWrite(), "Member is read-only");
		return MemberAccess<TClass, T, TSetter>::Ptr(_pSetter, Instance);
	}

	constexpr const T& GetConstValueRef(const TClass& Instance) const
	{
		static_assert(CanRead(), "Member is write-only");
		return MemberAccess<TClass, T, TGetter>::ConstRef(_pGetter, Instance);
	}

	constexpr T& GetValueRef(TClass& Instance) const
	{
		static_assert(CanWrite(), "Member is read-only");
		return MemberAccess<TClass, T, TSetter>::Ref(_pSetter, Instance);
	}

	// TODO: need to disable for plain C arrays (std::is_array_v)
	//template<typename TT = T>
	//constexpr typename std::enable_if_t<!std::is_array_v<TT>, T> GetValueCopy(const TClass& Instance) const
	constexpr T GetValueCopy(const TClass& Instance) const
	{
		static_assert(CanRead(), "Member is write-only");
		return MemberAccess<TClass, T, TGetter>::Copy(_pGetter, Instance);
	}

	constexpr decltype(auto) GetConstValue(const TClass& Instance) const
	{
		static_assert(CanRead(), "Member is write-only");
		return MemberAccess<TClass, T, TGetter>::BestGetConst(_pGetter, Instance);
	}

	constexpr decltype(auto) GetValue(TClass& Instance) const
	{
		static_assert(CanWrite(), "Member is read-only");
		return MemberAccess<TClass, T, TSetter>::BestGet(_pSetter, Instance);
	}

	template<typename U> //, typename = std::enable_if_t<std::is_constructible_v<T, U>>
	void SetValue(TClass& Instance, U&& Value) const
	{
		static_assert(CanWrite(), "Member is read-only");
		return MemberAccess<TClass, T, TSetter>::Set(_pSetter, Instance, std::forward<U>(Value));
	}

	constexpr const char*   GetName() const { return _pName; }
	constexpr std::uint32_t GetCode() const { return _Code; }

	// TODO: in addition could have per-member constexpr bool "need serialization/need diff/...?"
	constexpr bool          CanSerialize() const { return _Code != DEM::Meta::NO_MEMBER_CODE; }

private:

	std::uint32_t    _Code = NO_MEMBER_CODE;
	const char*      _pName = nullptr;
	TGetter          _pGetter = nullptr;
	TSetter          _pSetter = nullptr;
	CTypeMetadata<T> _Extras;
};

// FIXME: not usable without std::enable_if_t<is_setter_v<TAccessor, TClass, T>>> etc
//template<typename TClass, typename T>
//CMember(const char*, T TClass::*, T TClass::*) -> CMember<TClass, T, T TClass::*, T TClass::*>;
//template<typename TClass, typename T>
//CMember(const char*, T TClass::*) -> CMember<TClass, T, T TClass::*, std::nullptr_t>;
//template<typename TClass, typename T>
//CMember(const char*, std::nullptr_t, T TClass::*) -> CMember<TClass, T, std::nullptr_t, T TClass::*>;
//template<typename TClass, typename T>
//CMember(const char*, const T& (TClass::*)() const) -> CMember<TClass, T, const T& (TClass::*)() const, std::nullptr_t>;

template<typename TClass, typename T, typename TGetter, typename TSetter>
constexpr auto Member(const char* pName, TGetter pGetter = nullptr, TSetter pSetter = nullptr)
{
	return CMember<TClass, T, TGetter, TSetter>(pName, pGetter, pSetter);
}

template<typename TClass, typename T>
constexpr auto Member(const char* pName, T TClass::* pGetter)
{
	return CMember<TClass, T, T TClass::*, std::nullptr_t>(pName, pGetter, nullptr);
}

template<typename TClass, typename T>
constexpr auto Member(const char* pName, T TClass::* pGetter, T TClass::* pSetter)
{
	return CMember<TClass, T, T TClass::*, T TClass::*>(pName, pGetter, pSetter);
}

template<typename TClass, typename T, typename TGetter, typename TSetter>
constexpr auto Member(std::uint32_t Code, const char* pName, TGetter pGetter = nullptr, TSetter pSetter = nullptr)
{
	return CMember<TClass, T, TGetter, TSetter>(Code, pName, pGetter, pSetter);
}

template<typename TClass, typename T>
constexpr auto Member(std::uint32_t Code, const char* pName, T TClass::* pGetter)
{
	return CMember<TClass, T, T TClass::*, std::nullptr_t>(Code, pName, pGetter, nullptr);
}

template<typename TClass, typename T>
constexpr auto Member(std::uint32_t Code, const char* pName, T TClass::* pGetter, T TClass::* pSetter)
{
	return CMember<TClass, T, T TClass::*, T TClass::*>(Code, pName, pGetter, pSetter);
}

}

#define DEM_META_REGISTER_CLASS_NAME(Class) template<> constexpr auto RegisterClassName<Class>() { return #Class; }
#define DEM_META_MEMBER_FIELD_CODE(Class, Code, Name) Member(Code, #Name, &Class::Name, &Class::Name)
#define DEM_META_MEMBER_FIELD(Class, Name) Member(DEM::Utils::Hash(#Name), #Name, &Class::Name, &Class::Name)
#define DEM_META_MEMBER_FIELD_NOSAVE(Class, Name) Member(#Name, &Class::Name, &Class::Name)

// Default equality comparison for objects with registered metadata.
// Must be in the global namespace in order to be available everywhere on operator resolution.
template<typename T>
constexpr typename std::enable_if_t<DEM::Meta::CMetadata<T>::IsRegistered, bool> operator ==(const T& a, const T& b)
{
	return DEM::Meta::CMetadata<T>::IsEqual(a, b);
}

template<typename T>
constexpr typename std::enable_if_t<DEM::Meta::CMetadata<T>::IsRegistered, bool> operator !=(const T& a, const T& b)
{
	return !DEM::Meta::CMetadata<T>::IsEqual(a, b);
}
