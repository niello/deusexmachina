#pragma once
#include <Data/MemberAccess.h>
#include <cstdint>
#include <string>

// Provides type-safe introspection and serialization info for arbitrary objects
// Inspired by https://github.com/eliasdaler/MetaStuff

// TODO: CMetadata is a static object itself. Members, Name, IsRegistered etc are fields.
// TODO: CMember template deduction guides to replace multiple trivial Member(...) factory functions
// TODO: nicer Extras. CMember(n, g, s, Ex<T>().Min().Max())? or CMemberBase + per-type overloads with embedded extras?
// TODO: mark empty exras with [[no_unique_address]] in C++20?
// TODO: avoid duplicating Member() functions for Code+Name and Name-only? One constructor? Code is optional.
// TODO: better IsRegistered!

namespace DEM::Meta
{

// Specialize this for your types
template<typename T> inline constexpr auto RegisterMembers() { return std::make_tuple(); }
template<typename T> inline constexpr auto RegisterClassName() { return "<no class name specified>"; } // typeid(T).name()

// Specialize this to add more meta info to your types
template<typename T> struct CTypeMetadata {};

// Access this to work with registered types
template<typename T>
class CMetadata final
{
private:

	static inline constexpr auto _Members = RegisterMembers<T>();

public:

	static inline constexpr bool IsRegistered = (std::tuple_size_v<decltype(_Members)> != 0);

	static inline constexpr auto   GetClassName() { return RegisterClassName<T>(); }
	template<size_t Index>
	static inline constexpr auto   GetMember() { return std::get<Index>(_Members); }
	static inline constexpr size_t GetMemberCount() { return std::tuple_size_v<decltype(_Members)>; }

	template<typename TCallback>
	static inline void ForEachMember(TCallback Callback)
	{
		std::apply([Callback](auto& ...Members) { (..., Callback(Members)); }, _Members);
	}

	static inline constexpr bool HasMember(std::string_view Name)
	{
		return std::apply([Name](auto& ...Members) { return (... || (Members.GetName() == Name)); }, _Members);
	}

	static inline constexpr bool HasMember(std::uint32_t Code)
	{
		return std::apply([Code](auto& ...Members) { return (... || (Members.GetCode() == Code)); }, _Members);
	}

	template<typename TCallback>
	static inline bool WithMember(std::string_view Name, TCallback Callback)
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
	static inline bool WithMember(std::uint32_t Code, TCallback Callback)
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
};

// Interface to a registered member metadata
template<typename TClass, typename T, typename TGetter, typename TSetter>
class CMember final
{
	static_assert(!std::is_null_pointer_v<TGetter> || !std::is_null_pointer_v<TSetter>, "Inacessible members are not allowed, please add setter or getter");

public:

	constexpr CMember(const char* pName, TGetter pGetter = nullptr, TSetter pSetter = nullptr) noexcept
		: _pName(pName), _pGetter(pGetter), _pSetter(pSetter)
	{
	}

	constexpr CMember(std::uint32_t Code, const char* pName, TGetter pGetter = nullptr, TSetter pSetter = nullptr) noexcept
		: _Code(Code), _pName(pName), _pGetter(pGetter), _pSetter(pSetter)
	{
	}

	constexpr CMember& Extras(CTypeMetadata<T>&& Value) { _Extras = std::move(Value); return *this; }
	constexpr auto&    Extras() const { return _Extras; }

	template<typename U> static constexpr bool Is() { return std::is_same_v<T, U>; }

	constexpr const T* GetConstValuePtr(const TClass& Instance) const
	{
		static_assert(!std::is_same_v<TGetter, std::nullptr_t>, "Member is write-only");
		return MemberAccess<TClass, T, TGetter>::ConstPtr(_pGetter, Instance);
	}

	constexpr T* GetValuePtr(TClass& Instance) const
	{
		static_assert(!std::is_same_v<TSetter, std::nullptr_t>, "Member is read-only");
		return MemberAccess<TClass, T, TSetter>::Ptr(_pSetter, Instance);
	}

	constexpr const T& GetConstValueRef(const TClass& Instance) const
	{
		static_assert(!std::is_same_v<TGetter, std::nullptr_t>, "Member is write-only");
		return MemberAccess<TClass, T, TGetter>::ConstRef(_pGetter, Instance);
	}

	constexpr T& GetValueRef(TClass& Instance) const
	{
		static_assert(!std::is_same_v<TSetter, std::nullptr_t>, "Member is read-only");
		return MemberAccess<TClass, T, TSetter>::Ref(_pSetter, Instance);
	}

	constexpr T GetValueCopy(const TClass& Instance) const
	{
		static_assert(!std::is_same_v<TGetter, std::nullptr_t>, "Member is write-only");
		return MemberAccess<TClass, T, TGetter>::Copy(_pGetter, Instance);
	}

	constexpr decltype(auto) GetConstValue(const TClass& Instance) const
	{
		static_assert(!std::is_same_v<TGetter, std::nullptr_t>, "Member is write-only");
		return MemberAccess<TClass, T, TGetter>::BestGetConst(_pGetter, Instance);
	}

	constexpr decltype(auto) GetValue(TClass& Instance) const
	{
		static_assert(!std::is_same_v<TSetter, std::nullptr_t>, "Member is read-only");
		return MemberAccess<TClass, T, TSetter>::BestGet(_pSetter, Instance);
	}

	template<typename U> //, typename = std::enable_if_t<std::is_constructible_v<T, U>>
	void SetValue(TClass& Instance, U&& Value) const
	{
		static_assert(!std::is_same_v<TSetter, std::nullptr_t>, "Member is read-only");
		return MemberAccess<TClass, T, TSetter>::Set(_pSetter, Instance, std::forward<U>(Value));
	}

	const char*   GetName() const { return _pName; }
	std::uint32_t GetCode() const { return _Code; }

private:

	std::uint32_t    _Code = std::numeric_limits<std::uint32_t>::max();
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
inline constexpr auto Member(const char* pName, TGetter pGetter = nullptr, TSetter pSetter = nullptr)
{
	return CMember<TClass, T, TGetter, TSetter>(pName, pGetter, pSetter);
}

template<typename TClass, typename T>
inline constexpr auto Member(const char* pName, T TClass::* pGetter)
{
	return CMember<TClass, T, T TClass::*, std::nullptr_t>(pName, pGetter, nullptr);
}

template<typename TClass, typename T>
inline constexpr auto Member(const char* pName, T TClass::* pGetter, T TClass::* pSetter)
{
	return CMember<TClass, T, T TClass::*, T TClass::*>(pName, pGetter, pSetter);
}

template<typename TClass, typename T, typename TGetter, typename TSetter>
inline constexpr auto Member(std::uint32_t Code, const char* pName, TGetter pGetter = nullptr, TSetter pSetter = nullptr)
{
	return CMember<TClass, T, TGetter, TSetter>(Code, pName, pGetter, pSetter);
}

template<typename TClass, typename T>
inline constexpr auto Member(std::uint32_t Code, const char* pName, T TClass::* pGetter)
{
	return CMember<TClass, T, T TClass::*, std::nullptr_t>(Code, pName, pGetter, nullptr);
}

template<typename TClass, typename T>
inline constexpr auto Member(std::uint32_t Code, const char* pName, T TClass::* pGetter, T TClass::* pSetter)
{
	return CMember<TClass, T, T TClass::*, T TClass::*>(Code, pName, pGetter, pSetter);
}

}
