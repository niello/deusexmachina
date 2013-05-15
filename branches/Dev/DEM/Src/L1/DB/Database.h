#pragma once
#ifndef __DEM_L1_DB_DATABASE_H__
#define __DEM_L1_DB_DATABASE_H__

#include <Core/RefCounted.h>
#include <util/narray.h>
#include <Data/Flags.h>
#include "Table.h"
#include "Command.h"

// Database connection wrapper

typedef struct sqlite3 sqlite3;

namespace DB
{

class CDatabase: public Core::CRefCounted
{
	__DeclareClassNoFactory;

protected:

	enum
	{
		_IsOpen				=0x01,
		IgnoreUnknownCols	=0x02,
		InMemoryDB			=0x04,
		ExclusiveMode		=0x08,
		SyncMode			=0x10
	};

	CFlags			Flags;

	nString			ErrorStr;
	nArray<PTable>	Tables;     //????? NOTE: not in a Dictionary, because name may change externally!
	int				CacheSize;
	PCommand		BeginTransactionCmd;
	PCommand		EndTransactionCmd;
	DWORD			TransactionDepth;

	sqlite3*		SQLiteHandle;

	void	SetError(const nString& Error) { ErrorStr = Error; }
	void	ReadTableLayouts();
	//void RegisterAttributes(PTable& AttrTable);

public:

	enum EAccessMode
    {
        DBAM_ReadOnly,
        DBAM_ReadWriteExisting,
        DBAM_ReadWriteCreate
    };

    enum TempStore
    {
        File,
        Memory
    };

	nString		URI;
	EAccessMode	AccessMode;
	TempStore	TmpStorageType;
	int			BusyTimeout;

	CDatabase();
	virtual ~CDatabase();

	bool			Open();
	void			Close();
	bool			IsOpen() const { return Flags.Is(_IsOpen); }

	bool			AttachDatabase(const nString& URI, const nString& DBName);
	void			DetachDatabase(const nString& DBName);
	void			BeginTransaction();
	void			EndTransaction();
	void			CopyInMemoryDatabaseToFile(const nString& FileURI);

	void			AddTable(const PTable& Table);
	void			DeleteTable(const nString& TableName);
	bool			HasTable(const nString& TableName) const;
	int				GetNumTables() const { n_assert(IsOpen()); return Tables.GetCount(); }
	const PTable&	GetTable(int Idx) const { n_assert(IsOpen()); return Tables[Idx]; }
	const PTable&	GetTable(const nString& TableName) const { return Tables[FindTableIndex(TableName)]; }
	int				FindTableIndex(const nString& TableName) const;

	void			SetExclusiveMode(bool Yes) { Flags.SetTo(ExclusiveMode, Yes); }
	bool			IsExclusiveMode() const { return Flags.Is(ExclusiveMode); }
	void			SetIgnoreUnknownColumns(bool Yes) { Flags.SetTo(IgnoreUnknownCols, Yes); }
	bool			DoesIgnoreUnknownColumns() const { return Flags.Is(IgnoreUnknownCols); }
	void			SetSyncMode(bool Yes) { Flags.SetTo(SyncMode, Yes); }
	bool			IsSyncMode() const { return Flags.Is(SyncMode); }
	void			SetInMemoryDatabase(bool Yes) { Flags.SetTo(InMemoryDB, Yes); }
	void			SetCacheSizeInPages(int Size);
	int				GetCacheSizeInPages() const { return CacheSize; }
	const nString&	GetError() const { return ErrorStr; }
	sqlite3*		GetSQLiteHandle() const { n_assert(SQLiteHandle); return SQLiteHandle; }
};
//---------------------------------------------------------------------

typedef PDatabase PDatabase;

inline void CDatabase::SetCacheSizeInPages(int Size)
{
	n_assert(!IsOpen() && Size > 0);
	CacheSize = Size;
}
//---------------------------------------------------------------------

inline bool CDatabase::HasTable(const nString& TableName) const
{
	return (INVALID_INDEX != FindTableIndex(TableName));
}
//---------------------------------------------------------------------

}

#endif