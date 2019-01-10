#include "ShaderLibrary.h"

#include <Render/ShaderLoader.h>
#include <IO/Streams/ScopedStream.h>
#include <Core/Factory.h>

namespace Render
{
__ImplementClass(Render::CShaderLibrary, 'SLIB', Resources::CResourceObject);

CShaderLibrary::CShaderLibrary()
{
}
//---------------------------------------------------------------------

CShaderLibrary::~CShaderLibrary()
{
	//if (ShaderLoader.IsValidPtr()) ShaderLoader->ShaderLibrary = NULL; // Resolve cyclic dependency
}
//---------------------------------------------------------------------

void CShaderLibrary::SetLoader(Resources::PShaderLoader Loader)
{
	//if (ShaderLoader.IsValidPtr()) ShaderLoader->ShaderLibrary = NULL; // Resolve cyclic dependency
	ShaderLoader = (Resources::CShaderLoader*)Loader->Clone().GetUnsafe();
}
//---------------------------------------------------------------------

PShader CShaderLibrary::GetShaderByID(U32 ID)
{
	if (!ID || ShaderLoader.IsNullPtr()) return NULL;

	//!!!PERF:!
	//!!!need find index sorted for fixed arrays! move to algorithm?
	IPTR Idx = INVALID_INDEX;
	for (UPTR i = 0; i < TOC.GetCount(); ++i)
		if (TOC[i].ID == ID)
		{
			Idx = i;
			break;
		}
	if (Idx == INVALID_INDEX) return NULL;

	CRecord& Rec = TOC[Idx];
	if (Rec.LoadedShader.IsValidPtr()) return Rec.LoadedShader;

	// Allow nested loads
	U64 CurrOffset = Storage->GetPosition();

	IO::PScopedStream Stream = n_new(IO::CScopedStream)(Storage);
	Stream->SetScope(Rec.Offset, Rec.Size);

	// Introduce temporary cyclic dependency, loader requires access to library to load input signatures by ID
	ShaderLoader->ShaderLibrary = this;
	Rec.LoadedShader = (CShader*)ShaderLoader->Load(*Stream.GetUnsafe()).GetUnsafe();
	ShaderLoader->ShaderLibrary = NULL;

	Stream = NULL;

	Storage->Seek(CurrOffset, IO::Seek_Begin);

	return Rec.LoadedShader;
}
//---------------------------------------------------------------------

// Used primarily for input signatures of D3D11 vertex/geometry shaders
bool CShaderLibrary::GetRawDataByID(U32 ID, void*& pOutData, UPTR& OutSize)
{
	if (!ID) return NULL;

	//!!!PERF:!
	//!!!need find index sorted for fixed arrays! move to algorithm?
	IPTR Idx = INVALID_INDEX;
	for (UPTR i = 0; i < TOC.GetCount(); ++i)
		if (TOC[i].ID == ID)
		{
			Idx = i;
			break;
		}
	if (Idx == INVALID_INDEX) return NULL;

	CRecord& Rec = TOC[Idx];

	pOutData = n_malloc(Rec.Size);
	if (!pOutData) FAIL;

	// Allow nested loads
	U64 CurrOffset = Storage->GetPosition();

	if (!Storage->Seek(Rec.Offset, IO::Seek_Begin)) FAIL;

	UPTR ReadSize = Storage->Read(pOutData, Rec.Size);
	Storage->Seek(CurrOffset, IO::Seek_Begin);
	
	if (ReadSize != Rec.Size)
	{
		n_free(pOutData);
		FAIL;
	}

	OutSize = Rec.Size;

	OK;
}
//---------------------------------------------------------------------

}