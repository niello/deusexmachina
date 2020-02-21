#include "ScopedStream.h"

namespace IO
{

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

bool CScopedStream::Open()
{
	if (!HostStream) FAIL;
	if (!IsOpened())
	{
		if (!HostStream->IsOpened() && !HostStream->Open()) FAIL;

		U64 RealSize = HostStream->GetSize();
		if (ScopeOffset + ScopeSize > RealSize) ScopeSize = RealSize - ScopeOffset;

		if (!HostStream->Seek(ScopeOffset, Seek_Begin)) FAIL;
	}
	OK;
}
//---------------------------------------------------------------------

void CScopedStream::Close()
{
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

}