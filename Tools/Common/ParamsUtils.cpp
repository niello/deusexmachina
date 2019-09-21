#include "ParamsUtils.h"
#include <Utils.h>
#include <HRDParser.h>

namespace ParamsUtils
{

bool LoadParamsFromHRD(const char* pFileName, Data::CParams& OutParams)
{
	std::vector<char> In;
	if (!ReadAllFile(pFileName, In)) return false;

	Data::CHRDParser Parser;
	return Parser.ParseBuffer(In.data(), In.size(), OutParams);
}
//---------------------------------------------------------------------

/*
bool LoadParamsFromPRM(const char* pFileName, Data::CParams& OutParams)
{
	IO::PStream File = IOSrv->CreateStream(pFileName);
	if (!File || !File->Open(IO::SAM_READ)) return nullptr;
	IO::CBinaryReader Reader(*File);

	Data::PParams Params = n_new(Data::CParams);
	if (!Reader.ReadParams(*Params)) return false;
	OutParams = Params;
	return true;
}
//---------------------------------------------------------------------

bool LoadDescFromPRM(const char* pRootPath, const char* pRelativeFileName, Data::CParams& OutParams)
{
	Data::PParams Main;
	if (!ParamsUtils::LoadParamsFromPRM(std::string(pRootPath) + pRelativeFileName, Main)) return false;
	if (Main.IsNullPtr()) return false;

	std::string BaseName;
	if (Main->Get(BaseName, CStrID("_Base_")))
	{
		n_assert(BaseName != pRelativeFileName);
		if (!LoadDescFromPRM(pRootPath, BaseName + ".prm", OutParams)) return false;
		OutParams->Merge(*Main, Data::Merge_AddNew | Data::Merge_Replace | Data::Merge_Deep); //!!!can specify merge flags in Desc!
	}
	else OutParams = n_new(Data::CParams(*Main));

	return true;
}
//---------------------------------------------------------------------
*/

bool LoadDataSerializationSchemesFromDSS(const char* pFileName, std::map<CStrID, Data::CDataScheme>& OutSchemes)
{
	Data::CParams SchemeDescs;
	if (!ParamsUtils::LoadParamsFromHRD(pFileName, SchemeDescs)) return false;
	if (SchemeDescs.empty()) return false;

	for (const auto& Prm : SchemeDescs)
	{
		if (!Prm.second.IsA<Data::CParams>()) return false;

		Data::CDataScheme Scheme;
		if (!Scheme.Init(Prm.second.GetValue<Data::CParams>())) return false;
		OutSchemes[Prm.first] = std::move(Scheme);
	}

	return true;
}
//---------------------------------------------------------------------

/*
bool SaveParamsToHRD(const char* pFileName, const Data::CParams& Params)
{
	IO::PStream File = IOSrv->CreateStream(pFileName);
	if (!File || !File->Open(IO::SAM_WRITE)) return false;
	IO::CHRDWriter Writer(*File);
	return Writer.WriteParams(Params);
}
//---------------------------------------------------------------------

bool SaveParamsToPRM(const char* pFileName, const Data::CParams& Params)
{
	IO::PStream File = IOSrv->CreateStream(pFileName);
	if (!File || !File->Open(IO::SAM_WRITE)) return false;
	IO::CBinaryWriter Writer(*File);
	return Writer.WriteParams(Params);
}
//---------------------------------------------------------------------
*/

}