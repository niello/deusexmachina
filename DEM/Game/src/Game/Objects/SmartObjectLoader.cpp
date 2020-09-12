#include "SmartObjectLoader.h"
#include <Game/Objects/SmartObject.h>
#include <Resources/ResourceManager.h>
#include <IO/Stream.h>
#include <Data/Params.h>
#include <Data/Buffer.h>
#include <Data/DataArray.h>
#include <Data/HRDParser.h>

namespace Resources
{

const Core::CRTTI& CSmartObjectLoader::GetResultType() const
{
	return DEM::Game::CSmartObject::RTTI;
}
//---------------------------------------------------------------------

PResourceObject CSmartObjectLoader::CreateResource(CStrID UID)
{
	// TODO: can support optional multiple templates in one file through sub-ID
	const char* pOutSubId;
	Data::PBuffer Buffer;
	{
		IO::PStream Stream = _ResMgr.CreateResourceStream(UID, pOutSubId, IO::SAP_SEQUENTIAL);
		if (!Stream || !Stream->IsOpened()) return nullptr;
		Buffer = Stream->ReadAll();
	}
	if (!Buffer) return nullptr;

	// Read params from resource HRD
	Data::CParams Params;
	Data::CHRDParser Parser;
	if (!Parser.ParseBuffer(static_cast<const char*>(Buffer->GetConstPtr()), Buffer->GetSize(), Params)) return nullptr;

	Data::PParams DescSection;
	if (Params.TryGet<Data::PParams>(DescSection, CStrID("States")))
	{
		for (const auto& StateParam : *DescSection)
		{
			const CStrID ID = StateParam.GetName();
		}
	}

	CStrID DefaultState = Params.Get(CStrID("DefaultState"), CStrID::Empty);
	std::string Script = Params.Get(CStrID("Script"), CString::Empty); //???CStrID of ScriptObject resource? Loader requires Lua.

	Data::PDataArray Actions;
	if (Params.TryGet<Data::PDataArray>(Actions, CStrID("Actions")))
	{
		for (const auto& ActionData : *Actions)
		{
			if (auto pActionStr = ActionData.As<CString>())
			{
				//Interactions.emplace_back(CStrID(pActionStr->CStr()), sol::function());
			}
			else if (auto pActionDesc = ActionData.As<Data::PParams>())
			{
				CString ActID;
				if ((*pActionDesc)->TryGet<CString>(ActID, CStrID("ID")))
				{
					// TODO: pushes the compiled chunk as a Lua function on top of the stack,
					// need to save anywhere in object? Or use object's function name here as condition?
					//???or force condition to be Can<action name>? when caching, find once.
					// so don't load them here at all and fill once on init script
					sol::function ConditionFunc;
					const std::string Condition = (*pActionDesc)->Get(CStrID("Condition"), CString::Empty);
					if (!Condition.empty())
					{
						//auto LoadedCondition = _Lua.load("local Target = ...; return " + Condition, (ID.CStr() + ActID).CStr());
						//if (LoadedCondition.valid()) ConditionFunc = LoadedCondition;
					}

					//Interactions.emplace_back(CStrID(ActID.CStr()), std::move(ConditionFunc));
				}
			}
		}
	}

	// load TL assets (at least read them as PResource?)? Or force loading? Probably force to make asset ready!

	// load script, compile conditions(?), cache functions - InitScript(sol::state[&/_view] Lua)
	// conditions may be functions with predefined args?

	return n_new(DEM::Game::CSmartObject());
}
//---------------------------------------------------------------------

}
