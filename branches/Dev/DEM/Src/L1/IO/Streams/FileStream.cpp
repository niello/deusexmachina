#include "FileStream.h"

#include <IO/IOServer.h>

namespace IO
{

bool CFileStream::Open(EStreamAccessMode Mode, EStreamAccessPattern Pattern)
{
	n_assert(!IsOpen() && !hFile);
	if (!FileName.IsValid()) FAIL;
	if (!CStream::Open(Mode, Pattern)) FAIL;
	hFile = IOSrv->OpenFile(FS, FileName, Mode, Pattern);
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
	hFile = NULL;
	Flags.Clear(IS_OPEN);
	//CStream::Close();
}
//---------------------------------------------------------------------

DWORD CFileStream::Read(void* pData, DWORD Size)
{
	n_assert(IsOpen() && !IsMapped() && hFile && (!Size || pData));
	return (Size > 0) ? FS->Read(hFile, pData, Size) : 0;
}
//---------------------------------------------------------------------

DWORD CFileStream::Write(const void* pData, DWORD Size)
{
	n_assert(IsOpen() && !IsMapped() && hFile && (!Size || pData));
	return (Size > 0) ? FS->Write(hFile, pData, Size) : 0;
}
//---------------------------------------------------------------------

bool CFileStream::Seek(int Offset, ESeekOrigin Origin)
{
	n_assert(!IsMapped() && hFile);
	return FS->Seek(hFile, Offset, Origin);
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
	//DWORD readSize = Read(mappedContent, size);
	//n_assert(readSize == size);
	//Stream::Map();
	//return mappedContent;
	return NULL;
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

DWORD CFileStream::GetSize() const
{
	n_assert(hFile);
	return FS->GetFileSize(hFile);
}
//---------------------------------------------------------------------

DWORD CFileStream::GetPosition() const
{
	n_assert(hFile);
	return FS->Tell(hFile);
}
//---------------------------------------------------------------------

}