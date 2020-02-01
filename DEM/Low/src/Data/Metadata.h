#pragma once
//#include <StdDEM.h>
#include <Data/MemberAccess.h>

// Provides type-safe introspection and serialization info for arbitrary objects
// Inspired by https://github.com/eliasdaler/MetaStuff

namespace DEM::Meta
{

// Specialize this for your types
template<typename T> inline constexpr auto RegisterMetadata() { return std::make_tuple(); }

// Specialize this to add more meta info to your types
template<typename T> struct CTypeMetadata {};

// Access this to work with registered types
template<typename T>
class CMetadata final
{
private:

	// TODO: CMetadata is a static object itself, members, name, isregistered etc are fields
	// TODO: constexpr metadata? All is defined in a compile time, so why not?

	static inline constexpr auto _Members = RegisterMetadata<T>();
	//static const char* Name() { return registerName<T>(); }

public:

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

	//CMember& Get(TGetter pGetter) { _pGetter = pGetter; }
	//CMember& Set(TSetter pSetter) { _pSetter = pSetter; }
	// TODO: specials like SetRange for numerics

	constexpr CMember& Extras(CTypeMetadata<T>&& Value) { _Extras = std::move(Value); return *this; }
	constexpr auto&    Extras() const { return _Extras; }

	template<typename U> static constexpr bool Is() { return std::is_same_v<std::decay_t<T>, U>; }

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

	constexpr T GetValueCopy(TClass& Instance) const
	{
		static_assert(!std::is_same_v<TGetter, std::nullptr_t>, "Member is write-only");
		return MemberAccess<TClass, T, TGetter>::Copy(_pGetter, Instance);
	}

	constexpr decltype(auto) GetConstValue(TClass& Instance) const
	{
		static_assert(!std::is_same_v<TGetter, std::nullptr_t>, "Member is write-only");
		return MemberAccess<TClass, T, TGetter>::BestGetConst(_pGetter, Instance);
	}

	constexpr decltype(auto) GetValue(TClass& Instance) const
	{
		static_assert(!std::is_same_v<TSetter, std::nullptr_t>, "Member is read-only");
		return MemberAccess<TClass, T, TSetter>::BestGet(_pSetter, Instance);
	}

	template<typename U>
	void SetValue(TClass& Instance, U&& Value) const
	{
		static_assert(!std::is_same_v<TSetter, std::nullptr_t>, "Member is read-only");
		return MemberAccess<TClass, T, TSetter>::Set(_pSetter, Instance, std::forward<U>(Value));
	}

	const char* GetName() const { return _pName; }

private:

	const char*      _pName = nullptr;
	TGetter          _pGetter = nullptr;
	TSetter          _pSetter = nullptr;
	CTypeMetadata<T> _Extras;
};

template<typename TClass, typename T, typename TGetter = std::nullptr_t, typename TSetter = std::nullptr_t>
inline constexpr CMember<TClass, T, TGetter, TSetter> Member(const char* pName, TGetter pGetter = nullptr, TSetter pSetter = nullptr)
{
	return CMember<TClass, T, TGetter, TSetter>(pName, pGetter, pSetter);
}

template<typename TClass, typename T>
inline constexpr CMember<TClass, T, T TClass::*, std::nullptr_t> Member(const char* pName, T TClass::* pGetter)
{
	return CMember<TClass, T, T TClass::*, std::nullptr_t>(pName, pGetter, nullptr);
}

template<typename TClass, typename T>
inline constexpr CMember<TClass, T, T TClass::*, T TClass::*> Member(const char* pName, T TClass::* pGetter, T TClass::* pSetter)
{
	return CMember<TClass, T, T TClass::*, T TClass::*>(pName, pGetter, pSetter);
}

}
