#include "ShaderLibrary.h"

#include <IO/Streams/ScopedStream.h>
#include <Data/Buffer.h>
#include <Core/Factory.h>

namespace Render
{
FACTORY_CLASS_IMPL(Render::CShaderLibrary, 'SLIB', Resources::CResourceObject);

CShaderLibrary::CShaderLibrary() {}
CShaderLibrary::~CShaderLibrary() {}

Data::PBuffer CShaderLibrary::CopyRawData(U32 ID)
{
	if (!ID) return nullptr;

	//!!!PERF:!
	//!!!need find index sorted for fixed arrays! move to algorithm?
	IPTR Idx = INVALID_INDEX;
	for (UPTR i = 0; i < TOC.GetCount(); ++i)
		if (TOC[i].ID == ID)
		{
			Idx = i;
			break;
		}
	if (Idx == INVALID_INDEX) return nullptr;

	CRecord& Rec = TOC[Idx];

	// Store CurrOffset to enable nested loads
	U64 CurrOffset = Storage->Tell();
	if (!Storage->Seek(Rec.Offset, IO::Seek_Begin)) return nullptr;

	// Read data into the buffer
	Data::PBuffer Buffer(n_new(Data::CBuffer(Rec.Size)));
	const UPTR ReadSize = Storage->Read(Buffer->GetPtr(), Rec.Size);
	if (ReadSize != Rec.Size) return nullptr;

	// Restore offset
	Storage->Seek(CurrOffset, IO::Seek_Begin);

	return Buffer;
}
//---------------------------------------------------------------------

// For single threaded use only
IO::PStream CShaderLibrary::GetElementStream(U32 ID)
{
	if (!ID) return nullptr;

	//!!!PERF:!
	//!!!need find index sorted for fixed arrays! move to algorithm?
	IPTR Idx = INVALID_INDEX;
	for (UPTR i = 0; i < TOC.GetCount(); ++i)
		if (TOC[i].ID == ID)
		{
			Idx = i;
			break;
		}
	if (Idx == INVALID_INDEX) return nullptr;

	CRecord& Rec = TOC[Idx];

	IO::PScopedStream ElmStream = n_new(IO::CScopedStream(Storage));
	ElmStream->SetScope(Rec.Offset, Rec.Size);
	return ElmStream;
}
//---------------------------------------------------------------------

}