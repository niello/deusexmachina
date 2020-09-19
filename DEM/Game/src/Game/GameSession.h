#pragma once
#include <Core/RTTIBaseClass.h>
#include <Data/RefCounted.h>
#include <Data/StringID.h>
#include <sol/sol.hpp>
#include <map>

// Central object of the game logic and state. Registers and gives
// access to game features, i.e. different pieces of game logic.
// Only one feature of each type can be registered in a session.

// TODO: Lua binding! Bind registered systems as named fields + GetSystem(ClassName)!

namespace DEM::Game
{
using PGameSession = Ptr<class CGameSession>;

class CGameSession : public Data::CRefCounted
{
protected:

	// Zero-based type index for fast feature access
	inline static uint32_t FeatureTypeCount = 0;
	template<typename T> inline static const uint32_t FeatureTypeIndex = FeatureTypeCount++;

	using PFeature = std::unique_ptr<::Core::CRTTIBaseClass>;

	std::vector<PFeature>                     _Features;
	std::map<CStrID, ::Core::CRTTIBaseClass*> _FeaturesByName;
	sol::state                                _ScriptState;

public:

	CGameSession();

	sol::state& GetScriptState() { return _ScriptState; }

	template<class T, typename... TArgs> T* RegisterFeature(CStrID Name, TArgs&&... Args)
	{
		static_assert(std::is_base_of_v<::Core::CRTTIBaseClass, T>,
			"CGameSession::RegisterFeature() > Feature must be a subclass of CRTTIBaseClass");

		const auto TypeIndex = FeatureTypeIndex<T>;
		if (_Features.size() <= TypeIndex)
			_Features.resize(TypeIndex + 1);

		_Features[TypeIndex] = std::make_unique<T>(std::forward<TArgs>(Args)...);
		auto pFeature = _Features[TypeIndex].get();
		_FeaturesByName.emplace(Name, pFeature);
		return static_cast<T*>(pFeature);

		// TODO: register in Lua right now? or remember name for delayed Lua state init?
		//???or hack indexing instead of providing real Lua fields? Will require ID->Feature map here.
		//May be it is the best way, indexing speed may be comparable.
		//index will work only if field is not in a table itself! But there will be (almost?) nothing in a table!
		//sol::readonly( &my_class::my_member_variable )
		//lua.new_usertype<Component>("Component"
		//	"paint", &Component::Paint,
		//	sol::meta_function::new_index, &Component::Setter
		//	);

		//!!!can also call some InitScript<T> routine for these classes (if exists), to register types!
		//need to register HEntity, for example!
	}
	//---------------------------------------------------------------------

	template<class T> T* FindFeature() const
	{
		const auto TypeIndex = FeatureTypeIndex<T>;
		if (_Features.size() <= TypeIndex) return nullptr;
		auto pFeature = _Features[TypeIndex].get();
		return pFeature ? static_cast<T*>(pFeature) : nullptr;
	}
	//---------------------------------------------------------------------

	//???extend tpl key type to std::string_view? transparent comparator must handle CStrID, fix if not yet.
	//???add expected type and return nullptr if RTTI check failed?
	auto FindFeature(CStrID Name) const
	{
		auto It = _FeaturesByName.find(Name);
		return (It == _FeaturesByName.cend()) ? nullptr : It->second;
	}
	//---------------------------------------------------------------------

	//???!!!add FindByClassName? for Lua at least!
};

}
