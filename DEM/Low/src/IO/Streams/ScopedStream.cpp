#include "ScopedStream.h"
#include <Data/Buffer.h>

namespace IO
{

CScopedStream::CScopedStream(PStream Host, U64 Offset, U64 Size)
	: HostStream(Host)
	, ScopeOffset(Offset)
{
	if (HostStream && HostStream->IsOpened())
	{
		const U64 RealSize = HostStream->GetSize();
		ScopeSize = std::min(Size, RealSize - ScopeOffset);

		if (!HostStream->CanSeek() || !HostStream->Seek(ScopeOffset, Seek_Begin))
			HostStream = nullptr;
	}
}
//---------------------------------------------------------------------

bool CScopedStream::SetScope(U64 Offset, U64 Size)
{
	if (HostStream->IsOpened())
	{
		U64 RealSize = HostStream->GetSize();
		if (Size)
		{
			if (Offset > RealSize || Offset + Size > RealSize) FAIL;
		}
		else Size = RealSize - Offset;
		if (!HostStream->Seek(Offset, Seek_Begin)) FAIL;
	}
	ScopeOffset = Offset;
	ScopeSize = Size;
	OK;
}
//---------------------------------------------------------------------

void CScopedStream::Close()
{
	HostStream = nullptr;
}
//---------------------------------------------------------------------

bool CScopedStream::Seek(I64 Offset, ESeekOrigin Origin)
{
	if (ScopeOffset)
	{
		switch (Origin)
		{
			case Seek_Begin:	Offset += ScopeOffset; break;
			case Seek_End:		Offset += HostStream->GetSize() - (ScopeOffset + ScopeSize); break;
		}
	}
	return HostStream->Seek(Offset, Origin);
}
//---------------------------------------------------------------------

bool CScopedStream::Truncate()
{
	return HostStream->Truncate();
}
//---------------------------------------------------------------------

bool CScopedStream::IsEOF() const
{
	return HostStream->IsEOF() || HostStream->Tell() >= ScopeOffset + ScopeSize;
}
//---------------------------------------------------------------------

Data::PBuffer CScopedStream::ReadAll()
{
	if (!IsOpened() || IsEOF()) return nullptr;

	const auto Size = (ScopeOffset + ScopeSize) - HostStream->Tell();
	auto Buffer = std::make_unique<Data::CBufferMalloc>(Size);
	if (Size) Read(Buffer->GetPtr(), Size);

	return Buffer;
}
//---------------------------------------------------------------------

}