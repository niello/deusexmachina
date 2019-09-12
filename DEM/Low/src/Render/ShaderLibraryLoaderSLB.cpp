#include "ShaderLibraryLoaderSLB.h"

#include <Render/Shader.h>
#include <Render/ShaderLibrary.h>
#include <Resources/ResourceManager.h>
#include <IO/BinaryReader.h>

namespace Resources
{

const Core::CRTTI& CShaderLibraryLoaderSLB::GetResultType() const
{
	return Render::CShaderLibrary::RTTI;
}
//---------------------------------------------------------------------

PResourceObject CShaderLibraryLoaderSLB::CreateResource(CStrID UID)
{
	if (!pResMgr) return nullptr;

	const char* pOutSubId;
	IO::PStream Stream = pResMgr->CreateResourceStream(UID, pOutSubId);
	if (!Stream || !Stream->Open(IO::SAM_READ, IO::SAP_SEQUENTIAL)) return nullptr;

	IO::CBinaryReader Reader(*Stream);

	U32 Magic;
	if (!Reader.Read(Magic) || Magic != 'SLIB') return nullptr;

	U32 FormatVersion;
	if (!Reader.Read(FormatVersion)) return nullptr;

	U32 ShaderCount;
	if (!Reader.Read(ShaderCount)) return nullptr;

	Render::PShaderLibrary ShaderLibrary = n_new(Render::CShaderLibrary);

	ShaderLibrary->TOC.SetSize(ShaderCount);
	for (U32 i = 0; i < ShaderCount; ++i)
	{
		Render::CShaderLibrary::CRecord& Rec = ShaderLibrary->TOC[i];
		if (!Reader.Read(Rec.ID)) return nullptr;
		if (!Reader.Read(Rec.Offset)) return nullptr;
		if (!Reader.Read(Rec.Size)) return nullptr;
	}
	
	ShaderLibrary->Storage = Stream;

	return ShaderLibrary.Get();
}
//---------------------------------------------------------------------

}
