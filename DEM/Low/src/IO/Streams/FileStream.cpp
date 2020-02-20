#include "FileStream.h"

#include <IO/FileSystem.h>

namespace IO
{

CFileStream::CFileStream(const char* pPath, IFileSystem* pFS)
	: FileName(pPath)
	, FS(pFS)
{
}
//---------------------------------------------------------------------

CFileStream::~CFileStream()
{
	if (IsOpen()) Close();
}
//---------------------------------------------------------------------

bool CFileStream::Open(EStreamAccessMode Mode, EStreamAccessPattern Pattern)
{
	n_assert(!IsOpen() && !hFile);
	if (!FileName.IsValid() || FS.IsNullPtr()) FAIL;
	if (!CStream::Open(Mode, Pattern)) FAIL;
	hFile = FS->OpenFile(FileName.CStr(), Mode, Pattern);
	if (!hFile) FAIL;
	Flags.Set(IS_OPEN);
	OK;
}
//---------------------------------------------------------------------

void CFileStream::Close()
{
	n_assert(IsOpen() && hFile);
	if (IsMapped()) Unmap();
	FS->CloseFile(hFile);
	hFile = nullptr;
	Flags.Clear(IS_OPEN);
	//CStream::Close();
}
//---------------------------------------------------------------------

UPTR CFileStream::Read(void* pData, UPTR Size)
{
	n_assert(IsOpen() && !IsMapped() && hFile && (!Size || pData));
	return (Size > 0) ? FS->Read(hFile, pData, Size) : 0;
}
//---------------------------------------------------------------------

UPTR CFileStream::Write(const void* pData, UPTR Size)
{
	n_assert(IsOpen() && !IsMapped() && hFile && (!Size || pData));
	return (Size > 0) ? FS->Write(hFile, pData, Size) : 0;
}
//---------------------------------------------------------------------

bool CFileStream::Seek(I64 Offset, ESeekOrigin Origin)
{
	n_assert(!IsMapped() && hFile);
	return FS->Seek(hFile, Offset, Origin);
}
//---------------------------------------------------------------------

U64 CFileStream::Tell() const
{
	n_assert(hFile);
	return FS->Tell(hFile);
}
//---------------------------------------------------------------------

void CFileStream::Flush()
{
	n_assert(!IsMapped() && hFile);
	FS->Flush(hFile);
}
//---------------------------------------------------------------------

bool CFileStream::IsEOF() const
{
	n_assert(!IsMapped() && hFile);
	return FS->IsEOF(hFile);
}
//---------------------------------------------------------------------

void* CFileStream::Map()
{
	//n_assert(!mappedContent);

	//Size size = GetSize();
	//n_assert(size > 0);
	//mappedContent = Memory::Alloc(Memory::ScratchHeap, size);
	//Seek(0, Begin);
	//UPTR readSize = Read(mappedContent, size);
	//n_assert(readSize == size);
	//Stream::Map();
	//return mappedContent;
	return nullptr;
}
//---------------------------------------------------------------------

void CFileStream::Unmap()
{
	//n_assert(mappedContent);
	//Stream::Unmap();
	//Memory::Free(Memory::ScratchHeap, mappedContent);
	//mappedContent = 0;
}
//---------------------------------------------------------------------

U64 CFileStream::GetSize() const
{
	n_assert(hFile);
	return FS->GetFileSize(hFile);
}
//---------------------------------------------------------------------

}