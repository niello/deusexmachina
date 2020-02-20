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

	virtual bool	Open(EStreamAccessMode Mode, EStreamAccessPattern Pattern = SAP_DEFAULT) override;
	virtual void	Close() override;
	virtual UPTR	Read(void* pData, UPTR Size) override;
	virtual UPTR	Write(const void* pData, UPTR Size) override;
	virtual bool	Seek(I64 Offset, ESeekOrigin Origin) override;
	virtual U64		Tell() const override;
	virtual void	Flush() override;
	virtual void*	Map() override;
	virtual void	Unmap() override;

	void			SetFileName(const char* pPath) { n_assert(!IsOpen()); FileName = pPath; }
	const CString&	GetFileName() const { return FileName; }
	virtual U64		GetSize() const override;
	virtual bool	IsEOF() const override;
	virtual bool	CanRead() const override { OK; }
	virtual bool	CanWrite() const override { OK; }
	virtual bool	CanSeek() const override { OK; }
	virtual bool	CanBeMapped() const override { OK; }
};

typedef Ptr<CFileStream> PFileStream;

}

#endif