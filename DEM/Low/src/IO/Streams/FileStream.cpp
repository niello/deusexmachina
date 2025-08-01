#include "FileStream.h"
#include <IO/FileSystem.h>
#include <Data/Buffer.h>

namespace IO
{

CFileStream::CFileStream(const char* pPath, IFileSystem* pFS, EStreamAccessMode Mode, EStreamAccessPattern Pattern)
	: FileName(pPath)
	, FS(pFS)
{
	if (!FileName.empty() && FS)
		hFile = FS->OpenFile(FileName.c_str(), Mode, Pattern);
}
//---------------------------------------------------------------------

CFileStream::~CFileStream()
{
	if (IsOpened()) Close();
}
//---------------------------------------------------------------------

void CFileStream::Close()
{
	n_assert_dbg(IsOpened());
	if (IsMapped()) Unmap();

	// Truncate file once on close, if required
	if (TruncatedAt != std::numeric_limits<U64>().max())
	{
		FS->Seek(hFile, TruncatedAt, IO::Seek_Begin);
		FS->Truncate(hFile);
	}

	FS->CloseFile(hFile);
	hFile = nullptr;
}
//---------------------------------------------------------------------

UPTR CFileStream::Read(void* pData, UPTR Size)
{
	n_assert_dbg(IsOpened() && !IsMapped() && hFile && (!Size || pData));
	return (Size > 0) ? FS->Read(hFile, pData, Size) : 0;
}
//---------------------------------------------------------------------

UPTR CFileStream::Write(const void* pData, UPTR Size)
{
	n_assert_dbg(IsOpened() && !IsMapped() && hFile && (!Size || pData));

	// Reset cached truncation after every write operation
	TruncatedAt = std::numeric_limits<U64>().max();

	return (Size > 0) ? FS->Write(hFile, pData, Size) : 0;
}
//---------------------------------------------------------------------

bool CFileStream::Seek(I64 Offset, ESeekOrigin Origin)
{
	n_assert_dbg(!IsMapped() && hFile);
	return FS->Seek(hFile, Offset, Origin);
}
//---------------------------------------------------------------------

U64 CFileStream::Tell() const
{
	n_assert_dbg(hFile);
	return FS->Tell(hFile);
}
//---------------------------------------------------------------------

void CFileStream::Flush()
{
	n_assert_dbg(!IsMapped() && hFile);
	FS->Flush(hFile);
}
//---------------------------------------------------------------------

bool CFileStream::IsEOF() const
{
	n_assert_dbg(!IsMapped() && hFile);
	return FS->IsEOF(hFile);
}
//---------------------------------------------------------------------

void* CFileStream::Map()
{
	//n_assert_dbg(!mappedContent);

	//Size size = GetSize();
	//n_assert_dbg(size > 0);
	//mappedContent = Memory::Alloc(Memory::ScratchHeap, size);
	//Seek(0, Begin);
	//UPTR readSize = Read(mappedContent, size);
	//n_assert_dbg(readSize == size);
	//Stream::Map();
	//return mappedContent;
	return nullptr;
}
//---------------------------------------------------------------------

void CFileStream::Unmap()
{
	//n_assert_dbg(mappedContent);
	//Stream::Unmap();
	//Memory::Free(Memory::ScratchHeap, mappedContent);
	//mappedContent = 0;
}
//---------------------------------------------------------------------

bool CFileStream::IsMapped() const
{
	// TODO: IMPLEMENT MAPPING!
	return false;
}
//---------------------------------------------------------------------

U64 CFileStream::GetSize() const
{
	n_assert_dbg(hFile);
	return FS->GetFileSize(hFile);
}
//---------------------------------------------------------------------

Data::PBuffer CFileStream::ReadAll()
{
	if (!IsOpened() || IsEOF()) return nullptr;

	const auto Size = static_cast<UPTR>(GetSize() - Tell());
	auto Buffer = std::make_unique<Data::CBufferMalloc>(Size);
	if (Size) Read(Buffer->GetPtr(), Size);

	return Buffer;
}
//---------------------------------------------------------------------

}
