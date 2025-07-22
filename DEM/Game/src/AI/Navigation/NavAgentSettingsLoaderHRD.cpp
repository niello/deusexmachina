#include "NavAgentSettingsLoaderHRD.h"
#include <AI/Navigation/NavAgentSettings.h>
#include <AI/Navigation/TraversalAction.h>
#include <Resources/ResourceManager.h>
#include <IO/Stream.h>
#include <Data/Buffer.h>
#include <Data/HRDParser.h>
#include <Data/Params.h>
#include <Data/DataArray.h>
#include <Math/Math.h>
#include <Core/Factory.h>
#include <map>

namespace Resources
{

const DEM::Core::CRTTI& CNavAgentSettingsLoaderHRD::GetResultType() const
{
	return DEM::AI::CNavAgentSettings::RTTI;
}
//---------------------------------------------------------------------

DEM::Core::PObject CNavAgentSettingsLoaderHRD::CreateResource(CStrID UID)
{
	const char* pOutSubId;
	Data::PBuffer Buffer;
	{
		IO::PStream Stream = _ResMgr.CreateResourceStream(UID.CStr(), pOutSubId, IO::SAP_SEQUENTIAL);
		if (!Stream || !Stream->IsOpened()) return nullptr;
		Buffer = Stream->ReadAll();
	}
	if (!Buffer) return nullptr;

	// Read params from resource HRD
	Data::CParams Params;
	Data::CHRDParser Parser;
	if (!Parser.ParseBuffer(static_cast<const char*>(Buffer->GetConstPtr()), Buffer->GetSize(), Params)) return nullptr;

	std::map<U8, float> Costs;
	std::vector<DEM::AI::PTraversalAction> Actions;
	std::vector<bool> UseControllers;

	// Load area settings
	Data::PParams SubParams;
	if (Params.TryGet(SubParams, CStrID("Areas")))
	{
		for (const auto& Param : *SubParams)
		{
			if (!Param.IsA<Data::PParams>()) continue;

			const auto& AreaDesc = *Param.GetValue<Data::PParams>();

			Data::PDataArray AreaTypes;
			if (!AreaDesc.TryGet(AreaTypes, CStrID("AreaTypes")) || AreaTypes->empty()) continue;

			DEM::AI::PTraversalAction Action;
			{
				std::string ActionID;
				if (AreaDesc.TryGet(ActionID, CStrID("Action")))
					Action = DEM::Core::CFactory::Instance().Create<DEM::AI::CTraversalAction>(ActionID.c_str());
			}

			const bool UseControllersInArea = AreaDesc.Get(CStrID("UseControllers"), false);

			if (!UseControllersInArea && !Action) continue;

			const float Cost = AreaDesc.Get(CStrID("Cost"), 1.f);
			const bool IsCostOverridden = !n_fequal(Cost, 1.f);

			for (const auto& AreaTypeVal : *AreaTypes)
			{
				if (!AreaTypeVal.IsA<int>()) continue;

				const int AreaType = AreaTypeVal.GetValue<int>();
				if (AreaType < 0 || AreaType > 63) continue;

				const auto AreaTypeU8 = static_cast<U8>(AreaType);
				if (Actions.size() <= AreaTypeU8)
				{
					Actions.resize(AreaTypeU8 + 1);
					UseControllers.resize(AreaTypeU8 + 1);
				}
				Actions[AreaTypeU8] = Action;
				UseControllers[AreaTypeU8] = UseControllersInArea;
				if (IsCostOverridden) Costs.emplace(AreaTypeU8, Cost);
			}
		}
	}

	// TODO: read included/excluded flags

	return n_new(DEM::AI::CNavAgentSettings(std::move(Costs), std::move(Actions), std::move(UseControllers)));
}
//---------------------------------------------------------------------

}
