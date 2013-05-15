#pragma once
#ifndef __DEM_L1_DB_VALUE_TABLE_H__
#define __DEM_L1_DB_VALUE_TABLE_H__

#include <Core/RefCounted.h>
#include <Data/Buffer.h>
#include <Data/Flags.h>
#include <DB/AttrID.h>
#include <util/ndictionary.h>

/**
    @class DB::CValueTable
    
    A table of database values. This is the basic data container of the
    database subsystem. ValueTables are used to store the result of a
    query or to define the data which should be written back into the database.
    
    (C) 2006 Radon Labs GmbH

	@class CValueTable

    A table of attributes with a compact memory footprint and
    fast random access. Table Columns are defined by attribute ids
    which associate a name, a fourcc code, a datatype and an access mode
    (ReadWrite, ReadOnly) to the table. CAttr values are stored in
    one big chunk of memory without additional overhead. Table cells can
    have the NULL status, which means the cell contains no value.

    The table's value buffer consists of 4-byte aligned rows, each
    row consists of a bitfield with 2 bits per row (one bit is set
    if a column/row value is valid, the other is used as modified-marker).

    The header-bitfield is padded to 4-byte. After the header field follow 
    the value fields, one field for each column. The size of the field 
    depends on the datatype of the column, the minimum field size of 4 bytes for
    data alignment reasons:

    Bool:       sizeof(int)         usually 4 bytes
    Int:        sizeof(int)         usually 4 bytes
    Float:      sizeof(float)       usually 4 bytes
    Float4:     sizeof(float4)      usually 16 bytes
    Matrix44:   sizeof(matrix44)    usually 48 bytes
    String:     sizeof(char*)       usually 4 bytes

    The CValueTable object keeps track of all changes (added Columns,
    added rows, modified rows, modified values).
    
    Based on nebula 3 AttributeTable_(C) 2006 Radon Labs GmbH
*/

namespace DB
{

class CValueTable: public Core::CRefCounted //!!!rename Core to Core or even import Core from N3!
{
	__DeclareClass(CValueTable);

private:

	struct ColumnInfo
	{
		CAttrID	AttrID;
		int		ByteOffset;
	};

	enum
	{
		NewRow			= 0x01,
		UpdatedRow		= 0x02,
		DeletedRow		= 0x04,	// Marks row to be deleted at the next commit
		DestroyedRow	= 0x08	// Marks row as not existing in DB and invalid (reusable) in ValueTable
	};

	enum
	{
		_TrackModifications	= 0x01,
		_IsModified			= 0x02,
		_HasModifiedRows	= 0x04,
		_InBeginAddColumns	= 0x08
	};

	CFlags						Flags;

	void*						ValueBuffer;
	uchar*						RowStateBuffer;

	nArray<ColumnInfo>			Columns;
	nDictionary<CAttrID, int>	ColumnIndexMap;   // map attribute ID to column index
	nArray<int>					NewColumnIndices;       // indices of new Columns since last ResetModifiedState
	//nArray<void*>				UserData;

	int							FirstAddedColIndex;
	int							FirstNewRowIndex;
	int							NewRowsCount;
	int							FirstDeletedRowIndex;
	int							DeletedRowsCount;

	int							RowPitch;                             // pitch of a row in bytes
	int							NumRows;                              // number of rows
	int							AllocatedRows;                        // number of allocated rows

	void		Realloc(int NewPitch, int NewAllocRows);
	int			UpdateColumnOffsets();

	void**		GetValuePtr(int ColIdx, int RowIdx) const;

	nArray<int>	InternalFindRowIndicesByAttr(CAttrID AttrID, const CData& Value, bool FirstMatchOnly) const;
	//nArray<int>	InternalFindRowIndicesByAttrs(const nArray<CAttr>& Attrs, bool FirstMatchOnly) const; // CAttrSet

	void		SetColumnToDefaultValues(int ColIdx);
	void		SetRowToDefaultValues(int RowIdx);

	bool		IsSpecialType(const CType* T) const { return T == TVector4 || T == TMatrix44; }

public:

	CValueTable();
	virtual ~CValueTable();

	void				BeginAddColumns();
	void				AddColumn(CAttrID ID, bool IsNew = true);
	void				EndAddColumns();
	bool				HasColumn(CAttrID ID) const { return ColumnIndexMap.Contains(ID); }
	int					GetColumnIndex(CAttrID ID) const;
	int					GetNumColumns() const { return Columns.GetCount(); }
	CAttrID				GetColumnID(int ColIdx) const { return Columns[ColIdx].AttrID; }
	CStrID				GetColumnName(int ColIdx) const { return Columns[ColIdx].AttrID->GetName(); }
	AccessMode			GetColumnAccessMode(int ColIdx) const { return Columns[ColIdx].AttrID->GetAccessMode(); }
	const CType*		GetColumnValueType(int ColIdx) const { return Columns[ColIdx].AttrID->GetType(); }
	nArray<int>&		GetNewColumnIndices() { return NewColumnIndices; }

	void				Clear();
	void				ReserveRows(int NumRows) { n_assert(NumRows > 0); Realloc(RowPitch, AllocatedRows + NumRows); }
	int					AddRow();
	int					CopyRow(int FromRowIdx);
	int					CopyExtRow(CValueTable* pOther, int OtherRowIdx, bool CreateMissingCols = false);
	void				DeleteRow(int RowIdx);
	void				DeleteAllRows() { for (int i = 0; i < NumRows; i++) DeleteRow(i); }
	void				DeleteRowData(int RowIdx);

	uchar				GetRowState(int RowIdx) const;
	bool				IsRowNew(int RowIdx) const;
	bool				IsRowUpdated(int RowIdx) const;
	bool				IsRowOnlyUpdated(int RowIdx) const;
	bool				IsRowDeleted(int RowIdx) const;
	bool				IsRowValid(int RowIdx) const;
	bool				IsRowUntouched(int RowIdx) const;
	bool				IsRowModified(int RowIdx) const;
	bool				IsModified() const { return Flags.Is(_IsModified); }
	bool				HasModifiedRows() const { return Flags.Is(_HasModifiedRows); }
	void				SetModifiedTracking(bool Track) { Flags.SetTo(_TrackModifications, Track); }
	bool				GetModifiedTracking() const { return Flags.Is(_TrackModifications); }
	void				ClearNewRowStats();
	void				ClearDeletedRows();
	void				ClearRowFlags(int RowIdx);
	void				ResetModifiedState();
	int					GetRowCount() const { return NumRows; }
	int					GetFirstNewRowIndex() const { return FirstNewRowIndex; }
	int					GetNewRowsCount() const { return NewRowsCount; }
	int					GetFirstDeletedRowIndex() const { return FirstDeletedRowIndex; }
	int					GetDeletedRowsCount() const { return DeletedRowsCount; }

	nArray<int>			FindRowIndicesByAttr(CAttrID AttrID, const CData& Value, bool FirstMatchOnly) const;
	//nArray<int>			FindRowIndicesByAttrs(const nArray<CAttr>& Attrs, bool FirstMatchOnly) const; // CAttrSet
	int					FindRowIndexByAttr(CAttrID AttrID, const CData& Value) const;
	//int					FindRowIndexByAttrs(const nArray<CAttr>& Attrs) const; // CAttrSet
	//void				SetRowUserData(int RowIdx, void* p) { UserData[RowIdx] = p; }
	//void*				GetRowUserData(int RowIdx) const { return UserData[RowIdx]; }

	void				GetValue(int ColIdx, int RowIdx, CData& Val) const;
	void				GetValue(CAttrID AttrID, int RowIdx, CData& Val) const { GetValue(ColumnIndexMap[AttrID], RowIdx, Val); }
	template<class T>
	const T&			Get(int ColIdx, int RowIdx) const;
	template<class T>
	const T&			Get(CAttrID AttrID, int RowIdx) const { return Get<T>(ColumnIndexMap[AttrID], RowIdx); }
	void				SetValue(int ColIdx, int RowIdx, const CData& Val);
	void				SetValue(CAttrID AttrID, int RowIdx, const CData& Val) { SetValue(ColumnIndexMap[AttrID], RowIdx, Val); }
	template<class T>
	void				Set(int ColIdx, int RowIdx, const T& Val);
	template<class T>
	void				Set(CAttrID AttrID, int RowIdx, const T& Val) { Set(ColumnIndexMap[AttrID], RowIdx, Val); }
};
//---------------------------------------------------------------------

typedef Ptr<CValueTable> PValueTable;

inline void CValueTable::ClearNewRowStats()
{
	FirstNewRowIndex = MAX_SDWORD;
	NewRowsCount = 0;
}
//---------------------------------------------------------------------

// Doesn't update modification statistics
inline void CValueTable::ClearRowFlags(int RowIdx)
{
	n_assert(RowIdx > -1 && RowIdx < NumRows);
	RowStateBuffer[RowIdx] = 0;
}
//---------------------------------------------------------------------

inline void CValueTable::ResetModifiedState()
{
	Flags.Clear(_IsModified);
	Flags.Clear(_HasModifiedRows);
}
//---------------------------------------------------------------------

inline bool CValueTable::IsRowModified(int RowIdx) const
{
	n_assert(RowIdx > -1 && RowIdx < NumRows);
	return RowStateBuffer[RowIdx] != 0;
}
//---------------------------------------------------------------------

inline uchar CValueTable::GetRowState(int RowIdx) const
{
	n_assert(RowIdx > -1 && RowIdx < NumRows);
	return RowStateBuffer[RowIdx];
}
//---------------------------------------------------------------------

inline bool CValueTable::IsRowNew(int RowIdx) const
{
	n_assert(RowIdx > -1 && RowIdx < NumRows);
	return (RowStateBuffer[RowIdx] & NewRow) != 0;
}
//---------------------------------------------------------------------

inline bool CValueTable::IsRowUpdated(int RowIdx) const
{
	n_assert(RowIdx > -1 && RowIdx < NumRows);
	return (RowStateBuffer[RowIdx] & UpdatedRow) != 0;
}
//---------------------------------------------------------------------

inline bool CValueTable::IsRowOnlyUpdated(int RowIdx) const
{
	n_assert(RowIdx > -1 && RowIdx < NumRows);
	return RowStateBuffer[RowIdx] == UpdatedRow;
}
//---------------------------------------------------------------------

inline bool CValueTable::IsRowDeleted(int RowIdx) const
{
	n_assert(RowIdx > -1 && RowIdx < NumRows);
	return (RowStateBuffer[RowIdx] & DeletedRow) != 0;
}
//---------------------------------------------------------------------

// Returns true if row isn't deleted or destroyed
inline bool CValueTable::IsRowValid(int RowIdx) const
{
	n_assert(RowIdx > -1 && RowIdx < NumRows); //???incorporate into return value?
	return (RowStateBuffer[RowIdx] & (DeletedRow | DestroyedRow)) == 0;
}
//---------------------------------------------------------------------

inline bool CValueTable::IsRowUntouched(int RowIdx) const
{
	n_assert(RowIdx > -1 && RowIdx < NumRows); //???incorporate into return value?
	return RowStateBuffer[RowIdx] == 0;
}
//---------------------------------------------------------------------

inline int CValueTable::GetColumnIndex(CAttrID ID) const
{
	int Idx = ColumnIndexMap.FindIndex(ID);
	return (Idx != INVALID_INDEX) ? ColumnIndexMap[ID] : INVALID_INDEX;
}
//---------------------------------------------------------------------

inline void** CValueTable::GetValuePtr(int ColIdx, int RowIdx) const
{
	n_assert(ColIdx < Columns.GetCount() && RowIdx > -1 && RowIdx < NumRows);
	return (void**)((char*)ValueBuffer + RowIdx * RowPitch + Columns[ColIdx].ByteOffset);
}
//---------------------------------------------------------------------

template<> inline void CValueTable::Set<vector3>(int ColIdx, int RowIdx, const vector3& Val)
{
	Set<vector4>(ColIdx, RowIdx, vector4(Val));
}
//---------------------------------------------------------------------

template<class T> inline void CValueTable::Set(int ColIdx, int RowIdx, const T& Val)
{
	const CType* Type = GetColumnValueType(ColIdx);
	n_assert(!Type || Type == DATA_TYPE(T));
	n_assert(IsRowValid(RowIdx));

	void** pObj = GetValuePtr(ColIdx, RowIdx);
	if (Type) DATA_TYPE_NV(T)::CopyT(IsSpecialType(Type) ? (void**)&pObj : pObj, &Val);
	else *(CData*)pObj = Val;

	if (Flags.Is(_TrackModifications))
	{
		RowStateBuffer[RowIdx] |= UpdatedRow;
		Flags.Set(_IsModified);
		Flags.Set(_HasModifiedRows);
	}
}
//---------------------------------------------------------------------

inline void CValueTable::GetValue(int ColIdx, int RowIdx, CData& Val) const
{
	const CType* Type = GetColumnValueType(ColIdx);
	void** pObj = GetValuePtr(ColIdx, RowIdx);
	if (Type) Val.SetTypeValue(Type, IsSpecialType(Type) ? (void**)&pObj : pObj);
	else Val = *(CData*)pObj;
}
//---------------------------------------------------------------------

template<class T> inline const T& CValueTable::Get(int ColIdx, int RowIdx) const
{
	const CType* Type = GetColumnValueType(ColIdx);
	n_assert(!Type || Type == DATA_TYPE(T));
	void** pObj = GetValuePtr(ColIdx, RowIdx);
	return (Type) ?
		*(T*)DATA_TYPE_NV(T)::GetPtr(IsSpecialType(Type) ? (void**)&pObj : pObj) :
		((CData*)pObj)->GetValue<T>();
}
//---------------------------------------------------------------------

// Finds multiple row indices by matching attribute. This method can be slow since
// it may search linearly (and vertically) through the table.
inline nArray<int> CValueTable::FindRowIndicesByAttr(CAttrID AttrID, const CData& Value, bool FirstMatchOnly) const
{
	return InternalFindRowIndicesByAttr(AttrID, Value, FirstMatchOnly);
}
//---------------------------------------------------------------------

// Finds multiple row indices by multiple matching attribute. This method can be slow since
// it may search linearly (and vertically) through the table.
//inline nArray<int> CValueTable::FindRowIndicesByAttrs(const nArray<CAttr>& Attrs, bool FirstMatchOnly) const
//{
//	return InternalFindRowIndicesByAttrs(Attrs, FirstMatchOnly);
//}
////---------------------------------------------------------------------

// Finds single row index by matching attribute. This method can be slow since
// it may search linearly (and vertically) through the table.
inline int CValueTable::FindRowIndexByAttr(CAttrID AttrID, const CData& Value) const
{
	nArray<int> RowIndices = InternalFindRowIndicesByAttr(AttrID, Value, true);
	return (RowIndices.GetCount() == 1) ? RowIndices[0] : INVALID_INDEX; //???or if > 0?
}
//---------------------------------------------------------------------

// Finds single row index by multiple matching attributes. This method can be slow since
// it may search linearly (and vertically) through the table.
//inline int CValueTable::FindRowIndexByAttrs(const nArray<CAttr>& Attrs) const
//{
//	nArray<int> RowIndices = InternalFindRowIndicesByAttrs(Attrs, true);
//	return (RowIndices.GetCount() == 1) ? RowIndices[0] : INVALID_INDEX; //???or if > 0?
//}
////---------------------------------------------------------------------

inline void CValueTable::SetRowToDefaultValues(int RowIdx)
{
	for (int ColIdx = 0; ColIdx < GetNumColumns(); ColIdx++)
		SetValue(ColIdx, RowIdx, GetColumnID(ColIdx)->GetDefaultValue());
}
//---------------------------------------------------------------------

inline void CValueTable::SetColumnToDefaultValues(int ColIdx)
{
	const CData& DefVal = GetColumnID(ColIdx)->GetDefaultValue();
	for (int RowIdx = 0; RowIdx < GetRowCount(); RowIdx++) SetValue(ColIdx, RowIdx, DefVal);
}
//---------------------------------------------------------------------

}

#endif