#pragma once
#ifndef __DEM_L1_DB_DATASET_H__
#define __DEM_L1_DB_DATASET_H__

#include "Table.h"
#include "ValueTable.h"
#include "Command.h"

// A dataset is an efficient in-memory-cache for relational database data. It 
// is optimized for read/modify/write operations. Usually you tell the dataset object
// what you want from the database, modify the queried data, and write
// the queried data back. Datasets can also be used to write new data into the database.
// Based on mangalore Db::Dataset (C) 2006 Radon Labs GmbH

namespace DB
{

class CDataset: public Core::CRefCounted
{
protected:

	friend class CTable;

	PTable		Table;
	PValueTable	VT;
	PCommand	CmdSelect;
	PCommand	CmdInsert;
	PCommand	CmdUpdate;
	PCommand	CmdDelete;

	int			RowIdx;
	nString		SelectSQL;
	nString		WhereSQL;

	// PK & Non-PK ValueTable column indices
	nArray<int>	PKVTColumns;
	nArray<int>	NonPKVTColumns;
	
	void InvalidateCommands();
	void CompileSelectCmd();
	void CompileInsertCmd();
	void CompileUpdateCmd();
	void CompileDeleteCmd();
	void ExecuteInsertCmd();
	void ExecuteUpdateCmd();
	void ExecuteDeleteCmd();

public:

	CDataset(const PTable& HostTable);

	// SELECT query interface
	void				AddColumn(CAttrID AttrID) { VT->AddColumn(AttrID); }
	void				AddColumns(const CAttrID* AttrIDs, DWORD Count);
	void				AddColumnsFromTable();
	void				SetSelectSQL(const nString& SQL);
	void				ClearSelectSQL();
	void				SetWhereClause(const nString& SQL);
	//void				SetWhereClause(CAttrID, ECmpOp, CData);
	//void				SetWhereClause(const PFilterSet& Filter);
	void				ClearWhereClause() { SetWhereClause(NULL); }
	void				PerformQuery(bool AppendResult = false);

	// INSERT, UPDATE, DELETE queries interface
	void				CommitChanges(bool UseTransaction = true);
	void				CommitDeletedRows(bool UseTransaction = true);
	void				DeleteWhere(const nString& WhereSQL) const { if (Table.IsValid()) Table->DeleteWhere(WhereSQL); }
	void				TruncateTable() const { if (Table.IsValid()) Table->Truncate(); }

	// Read interface
	const PValueTable&	GetValueTable() { return VT; }
	//bool HasAttr(CAttrID AttrID) const;
	template<class T>
	const T&			Get(int ColIdx) const;
	template<class T>
	const T&			Get(CAttrID AttrID) const;
	//template<class T> void Get(CAttrID AttrID, T& Out) const;
	void GetValue(CAttrID AttrID, CData& Out);

	// Write interface
	int					AddRow() { RowIdx = VT->AddRow(); return RowIdx; }
	template<class T>
	void				Set(int ColIdx, const T& Value);
	template<class T>
	void				Set(CAttrID AttrID, const T& Value);
	template<class T>
	void				ForceSet(CAttrID AttrID, const T& Value);
	void				SetValue(CAttrID AttrID, const CData& Value);
	void				SetValueTable(const PValueTable& NewVT);
	void				Clear() { if (VT.IsValid()) VT->Clear(); }

	void				SetRowIndex(int Idx) { n_assert(Idx > -1 && Idx < VT->GetRowCount()); RowIdx = Idx; }
	int					GetRowIndex() const { return RowIdx; }
	int					GetRowCount() const { return VT.IsValid() ? VT->GetRowCount() : 0; }

	const PTable&		GetTable() const { return Table; }
};
//---------------------------------------------------------------------

typedef Ptr<CDataset> PDataset;

inline void CDataset::SetSelectSQL(const nString& SQL)
{
	SelectSQL = SQL;
	VT = CValueTable::Create();
	if (CmdSelect.IsValid() && CmdSelect->IsValid()) CmdSelect->Clear();
}
//---------------------------------------------------------------------

inline void CDataset::ClearSelectSQL()
{
	SelectSQL = NULL;
	if (CmdSelect.IsValid() && CmdSelect->IsValid()) CmdSelect->Clear();
}
//---------------------------------------------------------------------

inline void CDataset::SetWhereClause(const nString& SQL)
{
	WhereSQL = SQL;
	if (CmdSelect.IsValid() && CmdSelect->IsValid()) CmdSelect->Clear();
}
//---------------------------------------------------------------------

template<class T> inline const T& CDataset::Get(int ColIdx) const
{
	return VT->Get<T>(ColIdx, RowIdx);
}
//---------------------------------------------------------------------

template<class T> inline const T& CDataset::Get(CAttrID AttrID) const
{
	return VT->Get<T>(AttrID, RowIdx);
}
//---------------------------------------------------------------------

template<class T> inline void CDataset::Set(int ColIdx, const T& Value)
{
	VT->Set<T>(ColIdx, RowIdx, Value);
}
//---------------------------------------------------------------------

template<class T> inline void CDataset::Set(CAttrID AttrID, const T& Value)
{
	VT->Set<T>(AttrID, RowIdx, Value);
}
//---------------------------------------------------------------------

// Creates column if it doesn't exist
template<class T> inline void CDataset::ForceSet(CAttrID AttrID, const T& Value)
{
	VT->AddColumn(AttrID);
	VT->Set<T>(AttrID, RowIdx, Value);
}
//---------------------------------------------------------------------

inline void CDataset::SetValue(CAttrID AttrID, const CData& Value)
{
	VT->SetValue(AttrID, RowIdx, Value);
}
//---------------------------------------------------------------------

inline void CDataset::SetValueTable(const PValueTable& NewVT)
{
	n_assert(NewVT.IsValid());
	if (NewVT != VT)
	{
		VT = NewVT;
		InvalidateCommands();
	}
}
//---------------------------------------------------------------------

}

#endif
