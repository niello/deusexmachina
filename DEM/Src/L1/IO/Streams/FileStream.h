#pragma once
#ifndef __DEM_L1_FILE_STREAM_H__
#define __DEM_L1_FILE_STREAM_H__

#include <IO/Stream.h>
#include <IO/FileSystem.h>
#include <util/nstring.h>

// File system file access stream
// Partially based on Nebula 3 (c) IO::FileStream class

namespace IO
{

class CFileStream: public CStream
{
protected:

	nString		FileName;
	PFileSystem	FS;
	void*		hFile;

public:

	CFileStream(): hFile(NULL) {}
	virtual ~CFileStream() { if (IsOpen()) Close(); }

	bool			Open(const nString& Path, EStreamAccessMode Mode, EStreamAccessPattern Pattern = SAP_DEFAULT);
	virtual bool	Open(EStreamAccessMode Mode, EStreamAccessPattern Pattern = SAP_DEFAULT);
	virtual void	Close();
	virtual DWORD	Read(void* pData, DWORD Size);
	virtual DWORD	Write(const void* pData, DWORD Size);
	virtual bool	Seek(int Offset, ESeekOrigin Origin);
	virtual void	Flush();
	virtual void*	Map();
	virtual void	Unmap();

	void			SetFileName(const nString& Path) { n_assert(!IsOpen()); FileName = Path; }
	const nString&	GetFileName() const { return FileName; }
	virtual DWORD	GetSize() const;
	virtual DWORD	GetPosition() const;
	virtual bool	IsEOF() const;
	virtual bool	CanRead() const { OK; }
	virtual bool	CanWrite() const { OK; }
	virtual bool	CanSeek() const { OK; }
	virtual bool	CanBeMapped() const { OK; }
};

inline bool CFileStream::Open(const nString& Path, EStreamAccessMode Mode, EStreamAccessPattern Pattern)
{
	FileName = Path;
	return Open(Mode, Pattern);
}
//---------------------------------------------------------------------

}

#endif