#include "DataServer.h"

#include <Data/HRDParser.h>
#include <Data/XMLDocument.h>
#include <Data/Buffer.h>
#include <IO/IOServer.h>
#include <IO/HRDWriter.h>
#include <IO/BinaryReader.h>
#include <IO/BinaryWriter.h>
#include <IO/Streams/FileStream.h>

namespace Data
{
__ImplementClassNoFactory(Data::CDataServer, Core::CRefCounted);
__ImplementSingleton(Data::CDataServer);

CDataServer::CDataServer(): HRDCache(PParams())
{
	__ConstructSingleton;

#ifdef _EDITOR
	DataPathCB = NULL;
#endif
}
//---------------------------------------------------------------------

PParams CDataServer::LoadHRD(const nString& FileName, bool Cache)
{
	PParams P;
	if (HRDCache.Get(FileName.CStr(), P)) return P;
	else return ReloadHRD(FileName, Cache);
}
//---------------------------------------------------------------------

PParams CDataServer::ReloadHRD(const nString& FileName, bool Cache)
{
	CBuffer Buffer;
	if (!IOSrv->LoadFileToBuffer(FileName, Buffer)) return NULL;

	PParams Params;
	CHRDParser Parser; //???static?
	if (Parser.ParseBuffer((LPCSTR)Buffer.GetPtr(), Buffer.GetSize(), Params))
	{
		if (Cache) HRDCache.Add(FileName.CStr(), Params); //!!!???mangle/unmangle path to avoid duplicates?
	}
	else n_printf("FileIO: HRD parsing of \"%s\" failed\n", FileName.CStr());

	return Params;
}
//---------------------------------------------------------------------

//???remove from here? make user use readers/writers directly?
void CDataServer::SaveHRD(const nString& FileName, PParams Content)
{
	if (!Content.IsValid()) return;

	IO::CFileStream File;
	if (!File.Open(FileName, IO::SAM_WRITE)) return;
	IO::CHRDWriter Writer(File);
	Writer.WriteParams(Content);
}
//---------------------------------------------------------------------

void CDataServer::UnloadHRD(const nString& FileName)
{
	HRDCache.Remove(FileName.CStr());
}
//---------------------------------------------------------------------

PParams CDataServer::LoadPRM(const nString& FileName, bool Cache)
{
	PParams P;
	if (HRDCache.Get(FileName.CStr(), P)) return P;
	else return ReloadPRM(FileName, Cache);
}
//---------------------------------------------------------------------

PParams CDataServer::ReloadPRM(const nString& FileName, bool Cache)
{
	IO::CFileStream File;
	if (!File.Open(FileName, IO::SAM_READ)) return NULL;
	IO::CBinaryReader Reader(File);

	PParams Params = CParams::CreateInstance();
	if (Reader.ReadParams(*Params))
	{
		if (Cache) HRDCache.Add(FileName.CStr(), Params); //!!!???mangle path to avoid duplicates?
	}
	else
	{
		Params = NULL;
		n_printf("FileIO: PRM loading from \"%s\" failed\n", FileName.CStr());
	}

	return Params;
}
//---------------------------------------------------------------------

//???remove from here? make user use readers/writers directly?
void CDataServer::SavePRM(const nString& FileName, PParams Content)
{
	if (!Content.IsValid()) return;

	IO::CFileStream File;
	if (!File.Open(FileName, IO::SAM_WRITE)) return;
	IO::CBinaryWriter Writer(File);
	Writer.WriteParams(*Content);
}
//---------------------------------------------------------------------

PXMLDocument CDataServer::LoadXML(const nString& FileName) //, bool Cache)
{
	CBuffer Buffer;
	if (!IOSrv->LoadFileToBuffer(FileName, Buffer)) FAIL;

	PXMLDocument XML = n_new(CXMLDocument);
	if (XML->Parse((LPCSTR)Buffer.GetPtr(), Buffer.GetSize()) == tinyxml2::XML_SUCCESS)
	{
		//if (Cache) XMLCache.Add(FileName.CStr(), XML); //!!!???mangle/unmangle path to avoid duplicates?
	}
	else
	{
		n_printf("FileIO: XML parsing of \"%s\" failed: %s. %s.\n", FileName.CStr(), XML->GetErrorStr1(), XML->GetErrorStr2());
		XML = NULL;
	}

	return XML;
}
//---------------------------------------------------------------------

//!!!need desc cache! (independent from HRD cache)
bool CDataServer::LoadDesc(PParams& Out, const nString& FileName, bool Cache)
{
	PParams Main = LoadPRM(FileName, Cache);

	if (!Main.IsValid()) FAIL;

	nString BaseName;
	if (Main->Get(BaseName, CStrID("_Base_")))
	{
		BaseName = "actors:" + BaseName + ".prm";
		n_assert(BaseName != FileName);
		if (!LoadDesc(Out, BaseName, Cache)) FAIL;
		Out->Merge(*Main, Merge_AddNew | Merge_Replace | Merge_Deep); //!!!can specify merge flags in Desc!
	}
	else Out = n_new(CParams(*Main));

	OK;
}
//---------------------------------------------------------------------

bool CDataServer::LoadDataSchemes(const nString& FileName)
{
	PParams SchemeDescs = LoadHRD(FileName, false);
	for (int i = 0; i < SchemeDescs->GetCount(); ++i)
	{
		const CParam& Prm = SchemeDescs->Get(i);
		if (!Prm.IsA<PParams>()) FAIL;

		int Idx = DataSchemes.FindIndex(Prm.GetName());
		if (Idx != INVALID_INDEX) DataSchemes.EraseAt(Idx);

		PDataScheme Scheme = n_new(CDataScheme);
		if (!Scheme->Init(*Prm.GetValue<PParams>())) FAIL;
		DataSchemes.Add(Prm.GetName(), Scheme);
	}
	OK;
}
//---------------------------------------------------------------------

} //namespace Data
