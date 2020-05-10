#include "NavAgentSettingsLoaderHRD.h"
#include <AI/Navigation/NavAgentSettings.h>
#include <AI/Navigation/TraversalController.h>
#include <Resources/ResourceManager.h>
#include <IO/Stream.h>
#include <Data/Buffer.h>
#include <Data/HRDParser.h>
#include <Data/Params.h>
#include <Data/DataArray.h>
#include <Core/Factory.h>
#include <map>

namespace Resources
{

const Core::CRTTI& CNavAgentSettingsLoaderHRD::GetResultType() const
{
	return DEM::AI::CNavAgentSettings::RTTI;
}
//---------------------------------------------------------------------

PResourceObject CNavAgentSettingsLoaderHRD::CreateResource(CStrID UID)
{
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

	std::map<U8, float> Costs;
	std::vector<DEM::AI::PTraversalController> Controllers;

	// Load area settings
	Data::PParams SubParams;
	if (Params.TryGet(SubParams, CStrID("Areas")))
	{
		for (const auto& Param : *SubParams)
		{
			if (!Param.IsA<Data::PParams>()) continue;

			const auto& AreaDesc = *Param.GetValue<Data::PParams>();

			Data::PDataArray AreaTypes;
			if (!AreaDesc.TryGet(AreaTypes, CStrID("AreaTypes")) || AreaTypes->IsEmpty()) continue;

			CString ControllerID;
			if (!AreaDesc.TryGet(ControllerID, CStrID("Controller"))) continue;

			DEM::AI::PTraversalController Ctlr(Core::CFactory::Instance().Create<DEM::AI::CTraversalController>(ControllerID));
			if (!Ctlr) continue;

			const float Cost = AreaDesc.Get(CStrID("Cost"), 1.f);
			const bool IsCostOverridden = !n_fequal(Cost, 1.f);

			for (const auto& AreaTypeVal : *AreaTypes)
			{
				if (!AreaTypeVal.IsA<int>()) continue;

				const int AreaType = AreaTypeVal.GetValue<int>();
				if (AreaType < 0 || AreaType > 63) continue;

				const auto AreaTypeU8 = static_cast<U8>(AreaType);
				if (Controllers.size() <= AreaTypeU8)
					Controllers.resize(AreaTypeU8 + 1);
				Controllers[AreaTypeU8] = std::move(Ctlr);
				if (IsCostOverridden) Costs.emplace(AreaTypeU8, Cost);
			}
		}
	}

	// TODO: include/exclude flags

	return n_new(DEM::AI::CNavAgentSettings(std::move(Costs), std::move(Controllers)));
}
//---------------------------------------------------------------------

}
