#pragma once
#include <Core/RTTIBaseClass.h>
#include <Data/RefCounted.h>
#include <Data/StringID.h>
#include <Resources/ResourceManager.h> // FIXME: for script loading, may be moved to application later!
#include <Scripting/SolGame.h>
#include <map>

// Central object of the game logic and state. Registers and gives
// access to game features, i.e. different pieces of game logic.
// Only one feature of each type can be registered in a session.

namespace DEM::Game
{
using PGameSession = Ptr<class CGameSession>;

class CGameSession final : public Data::CRefCounted
{
protected:

	// Zero-based type index for fast feature access
	inline static uint32_t FeatureTypeCount = 0;
	template<typename T> inline static const uint32_t FeatureTypeIndex = FeatureTypeCount++;

	Resources::CResourceManager&                         _ResMgr;

	std::vector<std::unique_ptr<Core::CRTTIBaseClass>> _Features;
	std::map<CStrID, Core::CRTTIBaseClass*>            _FeaturesByName;
	sol::state                                           _ScriptState; //???or pass it as a constructor argument and reuse for multiple sessions?!
	sol::table                                           _ScriptFields;
	std::map<CStrID, sol::table>                         _LoadedScripts; // For faster lookup

public:

	CGameSession(Resources::CResourceManager& ResMgr);
	~CGameSession();

	sol::state& GetScriptState() { return _ScriptState; }
	auto& GetResourceManager() { return _ResMgr; } // FIXME: where to get properly?!

	sol::table GetScript(CStrID ID, bool ForceReload = false);

	template<class T, typename... TArgs> T* RegisterFeature(CStrID Name, TArgs&&... Args)
	{
		static_assert(std::is_base_of_v<Core::CRTTIBaseClass, T>,
			"CGameSession::RegisterFeature() > Feature must be a subclass of CRTTIBaseClass");

		const auto TypeIndex = FeatureTypeIndex<T>;
		if (_Features.size() <= TypeIndex)
			_Features.resize(TypeIndex + 1);

		_Features[TypeIndex] = std::make_unique<T>(std::forward<TArgs>(Args)...);
		auto pFeature = _Features[TypeIndex].get();
		_FeaturesByName.emplace(Name, pFeature);

		_ScriptFields[Name.CStr()] = static_cast<T*>(pFeature);

		return static_cast<T*>(pFeature);
	}
	//---------------------------------------------------------------------

	template<class T> T* FindFeature() const
	{
		const auto TypeIndex = FeatureTypeIndex<T>;
		return (TypeIndex < _Features.size()) ? static_cast<T*>(_Features[TypeIndex].get()) : nullptr;
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

	auto FindFeature(const char* pName) const
	{
		auto It = _FeaturesByName.find(CStrID(pName));
		return (It == _FeaturesByName.cend()) ? nullptr : It->second;
	}
	//---------------------------------------------------------------------

	//???!!!add FindByClassName? for Lua at least!
};

}
