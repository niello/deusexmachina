#pragma once
#ifndef __DEM_L1_FILE_STREAM_H__
#define __DEM_L1_FILE_STREAM_H__

#include <IO/Stream.h>
#include <Data/String.h>

// Stream that accesses data from a file stored in some file system

namespace IO
{
typedef Ptr<class IFileSystem> PFileSystem;

class CFileStream: public CStream
{
protected:

	CString		FileName;
	PFileSystem	FS;
	void*		hFile = nullptr;

public:

	CFileStream(const char* pPath, IFileSystem* pFS);
	virtual ~CFileStream() override;

	virtual bool	Open(EStreamAccessMode Mode, EStreamAccessPattern Pattern = SAP_DEFAULT);
	virtual void	Close();
	virtual UPTR	Read(void* pData, UPTR Size);
	virtual UPTR	Write(const void* pData, UPTR Size);
	virtual bool	Seek(I64 Offset, ESeekOrigin Origin);
	virtual void	Flush();
	virtual void*	Map();
	virtual void	Unmap();

	void			SetFileName(const char* pPath) { n_assert(!IsOpen()); FileName = pPath; }
	const CString&	GetFileName() const { return FileName; }
	virtual U64		GetSize() const;
	virtual U64		GetPosition() const;
	virtual bool	IsEOF() const;
	virtual bool	CanRead() const { OK; }
	virtual bool	CanWrite() const { OK; }
	virtual bool	CanSeek() const { OK; }
	virtual bool	CanBeMapped() const { OK; }
};

typedef Ptr<CFileStream> PFileStream;

}

#endif