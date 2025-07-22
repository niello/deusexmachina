#pragma once
#include <IO/Stream.h>

// Stream that accesses data from a file stored in some file system

namespace IO
{
typedef Ptr<class IFileSystem> PFileSystem;

class CFileStream: public IStream
{
protected:

	std::string          FileName;
	PFileSystem          FS;
	void*                hFile = nullptr;
	U64                  TruncatedAt = std::numeric_limits<U64>().max();

public:

	CFileStream(const char* pPath, IFileSystem* pFS, EStreamAccessMode Mode, EStreamAccessPattern Pattern = SAP_DEFAULT);
	virtual ~CFileStream() override;

	virtual void	Close() override;
	virtual UPTR	Read(void* pData, UPTR Size) override;
	virtual UPTR	Write(const void* pData, UPTR Size) override;
	virtual bool	Seek(I64 Offset, ESeekOrigin Origin) override;
	virtual U64		Tell() const override;
	virtual bool    Truncate() override { TruncatedAt = Tell(); return true; } // Truncation in FS can be slow, postpone to closing and do once
	virtual void	Flush() override;
	virtual void*	Map() override;
	virtual void	Unmap() override;

	void			SetFileName(const char* pPath) { n_assert(!IsOpened()); FileName = pPath; }
	const auto&     GetFileName() const { return FileName; }
	virtual U64		GetSize() const override;
	virtual bool	IsOpened() const override { return !!hFile; }
	virtual bool    IsMapped() const override;
	virtual bool	IsEOF() const override;
	virtual bool	CanRead() const override { OK; }
	virtual bool	CanWrite() const override { OK; }
	virtual bool	CanSeek() const override { OK; }
	virtual bool	CanBeMapped() const override { OK; }

	virtual Data::PBuffer ReadAll() override;
};

typedef Ptr<CFileStream> PFileStream;

}
