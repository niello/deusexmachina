#include "ShaderLibrary.h"

#include <IO/Streams/ScopedStream.h>
#include <Core/Factory.h>

namespace Render
{
__ImplementClass(Render::CShaderLibrary, 'SLIB', Resources::CResourceObject);

CShaderLibrary::CShaderLibrary() {}
CShaderLibrary::~CShaderLibrary() {}

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

IO::PStream CShaderLibrary::GetElementStream(U32 ID)
{
	//???return positioned stream + size OR RAM stream over storage stream OR RAM stream over a buffer?
	//???return buffer instead of all this?
	//???what about file mapping? GetRawDataByID is good for this.
	NOT_IMPLEMENTED;
	return nullptr;
}
//---------------------------------------------------------------------

}