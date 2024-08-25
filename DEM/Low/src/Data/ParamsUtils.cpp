#include "ParamsUtils.h"

#include <Data/HRDParser.h>
#include <Data/Buffer.h>
#include <Data/DataScheme.h>
#include <IO/IOServer.h>
#include <IO/HRDWriter.h>
#include <IO/BinaryReader.h>
#include <IO/BinaryWriter.h>
#include <IO/Streams/MemStream.h>

namespace ParamsUtils
{

Data::PParams LoadParamsFromHRD(const char* pFileName)
{
	IO::PStream File = IOSrv->CreateStream(pFileName, IO::SAM_READ, IO::SAP_SEQUENTIAL);
	if (!File || !File->IsOpened()) return nullptr;
	auto Buffer = File->ReadAll();
	if (!Buffer) return nullptr;

	Data::PParams Params(n_new(Data::CParams()));
	Data::CHRDParser Parser;
	//CString Errors;
	return Parser.ParseBuffer(static_cast<const char*>(Buffer->GetConstPtr()), Buffer->GetSize(), *Params/*, &Errors*/) ?
		Params :
		nullptr;
}
//---------------------------------------------------------------------

Data::PParams LoadParamsFromPRM(const char* pFileName)
{
	IO::PStream File = IOSrv->CreateStream(pFileName, IO::SAM_READ, IO::SAP_SEQUENTIAL);
	if (!File || !File->IsOpened()) return nullptr;
	IO::CBinaryReader Reader(*File);

	Data::PParams Params = n_new(Data::CParams);
	return Reader.ReadParams(*Params) ? Params : nullptr;
}
//---------------------------------------------------------------------

Data::PParams LoadDescFromPRM(const char* pRootPath, const char* pRelativeFileName)
{
	Data::PParams Main = ParamsUtils::LoadParamsFromPRM(CString(pRootPath) + pRelativeFileName);
	if (!Main) return nullptr;

	CString BaseName;
	if (!Main->TryGet(BaseName, CStrID("_Base_"))) return Main;

	if (BaseName == pRelativeFileName)
	{
		::Sys::Error("LoadDescFromPRM() > _Base_ can't be self!");
		return nullptr;
	}

	Data::PParams Params = LoadDescFromPRM(pRootPath, BaseName + ".prm");
	if (!Params) return nullptr;

	Params->Merge(*Main, Data::Merge_AddNew | Data::Merge_Replace | Data::Merge_Deep); //!!!can specify merge flags in Desc!

	return Params;
}
//---------------------------------------------------------------------

bool LoadDataSerializationSchemesFromDSS(const char* pFileName, CDict<CStrID, Data::PDataScheme>& OutSchemes)
{
	Data::PParams SchemeDescs = ParamsUtils::LoadParamsFromHRD(pFileName);
	if (!SchemeDescs) FAIL;

	for (UPTR i = 0; i < SchemeDescs->GetCount(); ++i)
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
	IO::PStream File = IOSrv->CreateStream(pFileName, IO::SAM_WRITE, IO::SAP_SEQUENTIAL);
	if (!File || !File->IsOpened()) FAIL;
	IO::CHRDWriter Writer(*File);
	return Writer.WriteParams(Params);
}
//---------------------------------------------------------------------

bool SaveParamsToPRM(const char* pFileName, const Data::CParams& Params)
{
	IO::PStream File = IOSrv->CreateStream(pFileName, IO::SAM_WRITE, IO::SAP_SEQUENTIAL);
	if (!File || !File->IsOpened()) FAIL;
	IO::CBinaryWriter Writer(*File);
	return Writer.WriteParams(Params);
}
//---------------------------------------------------------------------

}

namespace StringUtils
{

// TODO: need to improve API for writing into memory (string)! Use std::string as a dest buffer directly?!
std::string ToString(const Data::CParams& Value)
{
	std::string Result;
	IO::CMemStream Stream;
	IO::CHRDWriter Writer(Stream);
	if (Writer.WriteParams(Value))
	{
		auto Buf = Stream.Detach();
		Result.assign(static_cast<const char*>(Buf->GetConstPtr()), Buf->GetSize());
	}
	return Result;
}
//---------------------------------------------------------------------

}
