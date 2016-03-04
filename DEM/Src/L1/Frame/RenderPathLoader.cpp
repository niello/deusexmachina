#include "RenderPathLoader.h"

#include <Frame/RenderPath.h>
#include <Frame/RenderPhase.h>
#include <Resources/Resource.h>
#include <IO/IOServer.h>
#include <IO/Streams/FileStream.h>
#include <IO/BinaryReader.h>
#include <IO/PathUtils.h>
#include <Data/Buffer.h>
#include <Data/HRDParser.h>
#include <Core/Factory.h>

namespace Resources
{
__ImplementClass(Resources::CRenderPathLoader, 'RPLD', Resources::CResourceLoader);

const Core::CRTTI& CRenderPathLoader::GetResultType() const
{
	return Frame::CRenderPath::RTTI;
}
//---------------------------------------------------------------------

bool CRenderPathLoader::Load(CResource& Resource)
{
	const char* pURI = Resource.GetUID().CStr();
	const char* pExt = PathUtils::GetExtension(pURI);
	Data::PParams Desc;
	if (!n_stricmp(pExt, "hrd"))
	{
		Data::CBuffer Buffer;
		if (!IOSrv->LoadFileToBuffer(pURI, Buffer)) FAIL;
		Data::CHRDParser Parser;
		if (!Parser.ParseBuffer((const char*)Buffer.GetPtr(), Buffer.GetSize(), Desc)) FAIL;
	}
	else if (!n_stricmp(pExt, "prm"))
	{
		IO::PStream File = IOSrv->CreateStream(pURI);
		if (!File->Open(IO::SAM_READ, IO::SAP_SEQUENTIAL)) FAIL;
		IO::CBinaryReader Reader(*File);
		Desc = n_new(Data::CParams);
		if (!Reader.ReadParams(*Desc)) FAIL;
	}
	else FAIL;

	Frame::PRenderPath RP = n_new(Frame::CRenderPath);

	Data::CParam* pSubDesc;
	if (Desc->Get(pSubDesc, CStrID("Phases")))
	{
		Data::CParams& PhaseList = *pSubDesc->GetValue<Data::PParams>();
		RP->Phases.SetSize(PhaseList.GetCount());
		for (UPTR i = 0; i < PhaseList.GetCount(); ++i)
		{
			const Data::CParam& Prm = PhaseList[i];
			Data::CParams& PhaseDesc = *Prm.GetValue<Data::PParams>();

			CString ClassBase("Frame::CRenderPhase");
			const CString& PhaseType = PhaseDesc.Get<CString>(CStrID("Type"), CString::Empty); //???use FourCC in PRM?
			CString ClassName = "Frame::CRenderPhase" + PhaseType;
			Frame::PRenderPhase CurrPhase = (Frame::CRenderPhase*)Factory->Create(ClassName.CStr());

			if (!CurrPhase->Init(Prm.GetName(), PhaseDesc)) FAIL;

			RP->Phases[i] = CurrPhase;
		}
	}

	Resource.Init(RP.GetUnsafe(), this);

	OK;
}
//---------------------------------------------------------------------

}