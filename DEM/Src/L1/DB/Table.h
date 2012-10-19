#pragma once
#ifndef __DEM_L1_DB_TABLE_H__
#define __DEM_L1_DB_TABLE_H__

#include <Core/RefCounted.h>
#include <DB/Column.h>
#include <util/HashTable.h>
#include <util/ndictionary.h>

// DB table is a set of typed Columns grouped under a common Name. CTable is only layout
// descriptor and never contains data. If the table is attached to a Database, any
// changes to its structure, name etc will also lead to changes in the Database.

namespace DB
{
typedef Ptr<class CDataset> PDataset;
typedef Ptr<class CDatabase> PDatabase;

class CTable: public Core::CRefCounted
{
	__DeclareClass(CTable);

public:

	enum ConnectMode
	{
		ForceCreate,	// create new table, even if table exists
		AssumeExists,	// assume the table exists
		Default,		// check if table exist, create if it doesn't
	};

private:

	friend class CDatabase;

	bool						_IsConnected;
	PDatabase					Database;
	nString						Name;
	nArray<CColumn>				Columns;
	CHashTable<nString, int>		NameIdxMap; //!!!CStrID!
	nDictionary<CAttrID, int>	AttrIDIdxMap;
	nArray<int>					PKColumnIndices;

	bool			TableExists();
	void			DropTable();
	void			CreateTable();
	void			ReadTableLayout(bool IgnoreUnknownColumns);
	nString			BuildColumnDef(const CColumn& column);

public:

	CTable();
	virtual ~CTable();

	void			Connect(const PDatabase& db, ConnectMode connectMode, bool ignoreUnknownColumn = true);
	void			Disconnect(bool DropDBTable);
	
	PDataset		CreateDataset();
	void			CreateMultiColumnIndex(const nArray<CAttrID>& columnIds);

	void			CommitUncommittedColumns();
	void			DeleteWhere(const nString& WhereSQL);
	void			Truncate();

	void			SetName(const nString& n);
	const nString&	GetName() const { return Name; }
	bool			IsConnected() const { return _IsConnected; }
	void			AddColumn(const CColumn& c);
	int				GetNumColumns() const { return Columns.Size(); }
	const CColumn&	GetColumn(int i) const { return Columns[i]; }
	const CColumn&	GetColumn(CAttrID id) const { return Columns[AttrIDIdxMap[id]]; }
	const CColumn&	GetColumn(const nString& Name) const { return Columns[NameIdxMap[Name]]; }
	const CColumn&	GetPrimaryColumn(int Idx = 0) const { return Columns[PKColumnIndices[Idx]]; }
	bool			HasColumn(CAttrID id) const { return AttrIDIdxMap.Contains(id); }
	bool			HasColumn(const nString& Name) const { return NameIdxMap.Contains(Name); }
	bool			HasPrimaryColumn() const { return PKColumnIndices.Size() > 0; }
	bool			HasUncommittedColumns() const;
	
	const PDatabase&		GetDB() const { return Database; }
	const nArray<CColumn>&	GetColumns() const { return Columns; }
	const nArray<int>&		GetPKColumnIndices() const { return PKColumnIndices; }
};    
//---------------------------------------------------------------------

typedef Ptr<CTable> PTable;

inline bool CTable::HasUncommittedColumns() const
{
	// NOTE: Columns are always added at the end, so its ok if we just check the last column
	return Columns.Size() ? (!Columns.Back().Committed): false;
}
//---------------------------------------------------------------------

}

#endif

