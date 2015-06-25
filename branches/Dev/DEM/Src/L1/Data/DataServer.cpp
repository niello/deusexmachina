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
__ImplementSingleton(Data::CDataServer);

CDataServer::CDataServer(): HRDCache(256)
{
	__ConstructSingleton;
}
//---------------------------------------------------------------------

PParams CDataServer::LoadHRD(const CString& FileName, bool Cache)
{
	PParams P;
	if (HRDCache.Get(FileName.CStr(), P)) return P;
	else return ReloadHRD(FileName, Cache);
}
//---------------------------------------------------------------------

PParams CDataServer::ReloadHRD(const CString& FileName, bool Cache)
{
	CBuffer Buffer;
	if (!IOSrv->LoadFileToBuffer(FileName, Buffer)) return NULL;

	PParams Params;
	CHRDParser Parser; //???static?
	if (Parser.ParseBuffer((LPCSTR)Buffer.GetPtr(), Buffer.GetSize(), Params))
	{
		if (Cache) HRDCache.Add(FileName.CStr(), Params); //!!!???mangle/unmangle path to avoid duplicates?
	}
	//else Sys::Log("FileIO: HRD parsing of \"%s\" failed\n", FileName.CStr());

	return Params;
}
//---------------------------------------------------------------------

//???remove from here? make user use readers/writers directly?
void CDataServer::SaveHRD(const CString& FileName, const CParams* pContent)
{
	if (!pContent) return;

	IO::CFileStream File;
	if (!File.Open(FileName, IO::SAM_WRITE)) return;
	IO::CHRDWriter Writer(File);
	Writer.WriteParams(*pContent);
}
//---------------------------------------------------------------------

void CDataServer::UnloadHRD(const CString& FileName)
{
	HRDCache.Remove(FileName.CStr());
}
//---------------------------------------------------------------------

PParams CDataServer::LoadPRM(const CString& FileName, bool Cache)
{
	PParams P;
	if (HRDCache.Get(FileName.CStr(), P)) return P;
	else return ReloadPRM(FileName, Cache);
}
//---------------------------------------------------------------------

PParams CDataServer::ReloadPRM(const CString& FileName, bool Cache)
{
	IO::CFileStream File;
	if (!File.Open(FileName, IO::SAM_READ)) return NULL;
	IO::CBinaryReader Reader(File);

	PParams Params = n_new(CParams);
	if (Reader.ReadParams(*Params))
	{
		if (Cache) HRDCache.Add(FileName.CStr(), Params); //!!!???mangle path to avoid duplicates?
	}
	else
	{
		Params = NULL;
		//Sys::Log("FileIO: PRM loading from \"%s\" failed\n", FileName.CStr());
	}

	return Params;
}
//---------------------------------------------------------------------

//???remove from here? make user use readers/writers directly?
bool CDataServer::SavePRM(const CString& FileName, const CParams* pContent)
{
	if (!pContent) FAIL;

	IO::CFileStream File;
	if (!File.Open(FileName, IO::SAM_WRITE)) FAIL;
	IO::CBinaryWriter Writer(File);
	return Writer.WriteParams(*pContent);
}
//---------------------------------------------------------------------

PXMLDocument CDataServer::LoadXML(const CString& FileName) //, bool Cache)
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
		//Sys::Log("FileIO: XML parsing of \"%s\" failed: %s. %s.\n", FileName.CStr(), XML->GetErrorStr1(), XML->GetErrorStr2());
		XML = NULL;
	}

	return XML;
}
//---------------------------------------------------------------------

//!!!need desc cache! (independent from HRD cache) OR pre-unwind descs on export!
bool CDataServer::LoadDesc(PParams& Out, const CString& Context, const CString& Name, bool Cache)
{
	PParams Main = LoadPRM(Context + Name + ".prm", Cache);

	if (Main.IsNullPtr()) FAIL;

	CString BaseName;
	if (Main->Get(BaseName, CStrID("_Base_")))
	{
		n_assert(BaseName != Name);
		if (!LoadDesc(Out, Context, BaseName, Cache)) FAIL;
		Out->Merge(*Main, Merge_AddNew | Merge_Replace | Merge_Deep); //!!!can specify merge flags in Desc!
	}
	else Out = n_new(CParams(*Main));

	OK;
}
//---------------------------------------------------------------------

bool CDataServer::LoadDataSchemes(const CString& FileName)
{
	PParams SchemeDescs = LoadHRD(FileName, false);
	if (SchemeDescs.IsNullPtr()) FAIL;

	for (int i = 0; i < SchemeDescs->GetCount(); ++i)
	{
		const CParam& Prm = SchemeDescs->Get(i);
		if (!Prm.IsA<PParams>()) FAIL;

		int Idx = DataSchemes.FindIndex(Prm.GetName());
		if (Idx != INVALID_INDEX) DataSchemes.RemoveAt(Idx);

		PDataScheme Scheme = n_new(CDataScheme);
		if (!Scheme->Init(*Prm.GetValue<PParams>())) FAIL;
		DataSchemes.Add(Prm.GetName(), Scheme);
	}

	OK;
}
//---------------------------------------------------------------------

} //namespace Data
