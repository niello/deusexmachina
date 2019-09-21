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

bool LoadParamsFromPRM(const char* pFileName, Data::CParams& OutParams)
{
	IO::PStream File = IOSrv->CreateStream(pFileName);
	if (!File || !File->Open(IO::SAM_READ)) return nullptr;
	IO::CBinaryReader Reader(*File);

	Data::PParams Params = n_new(Data::CParams);
	if (!Reader.ReadParams(*Params)) FAIL;
	OutParams = Params;
	OK;
}
//---------------------------------------------------------------------

bool LoadDescFromPRM(const char* pRootPath, const char* pRelativeFileName, Data::CParams& OutParams)
{
	Data::PParams Main;
	if (!ParamsUtils::LoadParamsFromPRM(std::string(pRootPath) + pRelativeFileName, Main)) FAIL;
	if (Main.IsNullPtr()) FAIL;

	std::string BaseName;
	if (Main->Get(BaseName, CStrID("_Base_")))
	{
		n_assert(BaseName != pRelativeFileName);
		if (!LoadDescFromPRM(pRootPath, BaseName + ".prm", OutParams)) FAIL;
		OutParams->Merge(*Main, Data::Merge_AddNew | Data::Merge_Replace | Data::Merge_Deep); //!!!can specify merge flags in Desc!
	}
	else OutParams = n_new(Data::CParams(*Main));

	OK;
}
//---------------------------------------------------------------------

bool LoadDataSerializationSchemesFromDSS(const char* pFileName, std::map<CStrID, Data::PDataScheme>& OutSchemes)
{
	Data::PParams SchemeDescs;
	if (!ParamsUtils::LoadParamsFromHRD(pFileName, SchemeDescs)) FAIL;
	if (SchemeDescs.IsNullPtr()) FAIL;

	for (size_t i = 0; i < SchemeDescs->GetCount(); ++i)
	{
		const Data::CParam& Prm = SchemeDescs->Get(i);
		if (!Prm.IsA<Data::PParams>()) FAIL;

		IPTR Idx = OutSchemes.FindIndex(Prm.GetName());
		if (Idx != INVALID_INDEX) OutSchemes.RemoveAt(Idx);

		Data::PDataScheme Scheme = n_new(Data::CDataScheme);
		if (!Scheme->Init(*Prm.GetValue<Data::PParams>())) FAIL;
		OutSchemes.Add(Prm.GetName(), Scheme);
	}

	OK;
}
//---------------------------------------------------------------------

bool SaveParamsToHRD(const char* pFileName, const Data::CParams& Params)
{
	IO::PStream File = IOSrv->CreateStream(pFileName);
	if (!File || !File->Open(IO::SAM_WRITE)) FAIL;
	IO::CHRDWriter Writer(*File);
	return Writer.WriteParams(Params);
}
//---------------------------------------------------------------------

bool SaveParamsToPRM(const char* pFileName, const Data::CParams& Params)
{
	IO::PStream File = IOSrv->CreateStream(pFileName);
	if (!File || !File->Open(IO::SAM_WRITE)) FAIL;
	IO::CBinaryWriter Writer(*File);
	return Writer.WriteParams(Params);
}
//---------------------------------------------------------------------

}