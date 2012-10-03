#pragma once
#ifndef __DEM_L1_MEM_STREAM_H__
#define __DEM_L1_MEM_STREAM_H__

#include <Data/Stream.h>

// RAM access stream
// Partially based on Nebula 3 (c) IO::MemoryStream class

namespace Data
{

class CMemStream: public CStream
{
protected:

	char*	pBuffer;
	DWORD	Pos;
	DWORD	DataSize;
	DWORD	AllocSize;
	bool	SelfAlloc;

public:

	CMemStream(): pBuffer(NULL), Pos(0), DataSize(0), AllocSize(0), SelfAlloc(false) {}
	virtual ~CMemStream() { if (IsOpen()) Close(); }

	bool			Open(void* pData, DWORD Size, EStreamAccessMode Mode, EStreamAccessPattern Pattern = SAP_DEFAULT);
	virtual bool	Open(EStreamAccessMode Mode, EStreamAccessPattern Pattern = SAP_DEFAULT);
	virtual void	Close();
	virtual DWORD	Read(void* pData, DWORD Size);
	virtual DWORD	Write(const void* pData, DWORD Size);
	virtual bool	Seek(int Offset, ESeekOrigin Origin);
	virtual void	Flush() {}
	virtual void*	Map();

	const char*		GetPtr() const { return pBuffer; }
	virtual DWORD	GetSize() const { return DataSize; }
	virtual DWORD	GetPosition() const { return Pos; }
	virtual bool	IsEOF() const;
	virtual bool	CanRead() const { OK; }
	virtual bool	CanWrite() const { OK; }
	virtual bool	CanSeek() const { OK; }
	virtual bool	CanBeMapped() const { OK; }
};

inline bool CMemStream::Open(void* pData, DWORD Size, EStreamAccessMode Mode, EStreamAccessPattern Pattern)
{
	pBuffer = (char*)pData;
	DataSize = Size;
	return Open(Mode, Pattern);
}
//---------------------------------------------------------------------

}

#endif