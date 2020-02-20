#include "ScopedStream.h"

namespace IO
{

bool CScopedStream::SetScope(U64 Offset, U64 Size)
{
	if (HostStream->IsOpen())
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

bool CScopedStream::Open(EStreamAccessMode Mode, EStreamAccessPattern Pattern)
{
	if (HostStream.IsNullPtr()) FAIL;
	if (!IsOpen())
	{
		if (!HostStream->IsOpen() && !HostStream->Open(Mode, Pattern)) FAIL;

		U64 RealSize = HostStream->GetSize();
		if (ScopeOffset + ScopeSize > RealSize) ScopeSize = RealSize - ScopeOffset;

		if (!HostStream->Seek(ScopeOffset, Seek_Begin)) FAIL;
		Flags.Set(IS_OPEN);
	}
	OK;
}
//---------------------------------------------------------------------

void CScopedStream::Close()
{
	if (IsOpen()) Flags.Clear(IS_OPEN);
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

bool CScopedStream::IsEOF() const
{
	return HostStream->IsEOF() || HostStream->Tell() >= ScopeOffset + ScopeSize;
}
//---------------------------------------------------------------------

}