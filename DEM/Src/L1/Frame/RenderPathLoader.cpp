#include "RenderPathLoader.h"

#include <Frame/RenderPath.h>
#include <Frame/RenderPhase.h>
#include <IO/BinaryReader.h>
#include <Data/Buffer.h>
#include <Data/HRDParser.h>
#include <Core/Factory.h>

namespace Resources
{
__ImplementClassNoFactory(Resources::CRenderPathLoader, Resources::CResourceLoader);
__ImplementClass(Resources::CRenderPathLoaderHRD, 'RPLH', Resources::CRenderPathLoader);
__ImplementClass(Resources::CRenderPathLoaderPRM, 'RPLP', Resources::CRenderPathLoader);

const Core::CRTTI& CRenderPathLoader::GetResultType() const
{
	return Frame::CRenderPath::RTTI;
}
//---------------------------------------------------------------------

PResourceObject CRenderPathLoader::LoadImpl(Data::PParams Desc)
{
	if (Desc.IsNullPtr()) return NULL;

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

			if (!CurrPhase->Init(Prm.GetName(), PhaseDesc)) return NULL;

			RP->Phases[i] = CurrPhase;
		}
	}

	return RP.GetUnsafe();
}
//---------------------------------------------------------------------

PResourceObject CRenderPathLoaderHRD::Load(IO::CStream& Stream)
{
	UPTR DataSize = (UPTR)Stream.GetSize();

	void* pData = NULL;
	if (Stream.CanBeMapped()) pData = Stream.Map();
	bool Mapped = !!pData;
	if (!Mapped)
	{
		pData = n_malloc(DataSize);
		if (Stream.Read(pData, DataSize) != DataSize)
		{
			n_free(pData);
			return NULL;
		}
	}

	Data::PParams Desc;
	Data::CHRDParser Parser;
	bool Result = Parser.ParseBuffer((const char*)pData, DataSize, Desc);

	if (Mapped) Stream.Unmap();
	else n_free(pData);

	return Result ? LoadImpl(Desc) : PResourceObject();
}
//---------------------------------------------------------------------

PResourceObject CRenderPathLoaderPRM::Load(IO::CStream& Stream)
{
	IO::CBinaryReader Reader(Stream);
	Data::PParams Desc = n_new(Data::CParams);
	if (!Reader.ReadParams(*Desc)) return NULL;
	return LoadImpl(Desc);
}
//---------------------------------------------------------------------

}