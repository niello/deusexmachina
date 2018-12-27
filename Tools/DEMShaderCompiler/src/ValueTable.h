#pragma once
#ifndef __DEM_L1_DB_VALUE_TABLE_H__
#define __DEM_L1_DB_VALUE_TABLE_H__

#include <Data/Buffer.h>
#include <Data/StringID.h>
#include <Data/Flags.h>
#include <Data/Dictionary.h>
#include <Data/Data.h>

namespace DB //???to Data?
{

class CValueTable
{
private:

	struct ColumnInfo
	{
		CStrID				ID;
		const Data::CType*	Type;
		int					ByteOffset;
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

	Data::CFlags				Flags;

	void*						ValueBuffer;
	U8*							RowStateBuffer;

	CArray<ColumnInfo>			Columns;
	CDictionary<CStrID, int>	ColumnIndexMap;		// map attribute ID to column index
	CArray<int>					NewColumnIndices;	// indices of new Columns since last ResetModifiedState

	int							FirstAddedColIndex;
	UPTR						FirstNewRowIndex;
	UPTR						NewRowsCount;
	UPTR						FirstDeletedRowIndex;
	int							DeletedRowsCount;

	UPTR						RowPitch;			// pitch of a row in bytes
	UPTR						NumRows;			// number of rows
	UPTR						AllocatedRows;		// number of allocated rows

	void		Realloc(UPTR NewPitch, UPTR NewAllocRows);
	int			UpdateColumnOffsets();

	void**		GetValuePtr(UPTR ColIdx, UPTR RowIdx) const;

	CArray<int>	InternalFindRowIndicesByAttr(CStrID AttrID, const Data::CData& Value, bool FirstMatchOnly) const;

	void		SetColumnToDefaultValues(UPTR ColIdx);
	void		SetRowToDefaultValues(UPTR RowIdx);

	bool		IsSpecialType(const Data::CType* T) const { FAIL; } // { return T == TVector4 || T == TMatrix44; }

public:

	CValueTable();
	~CValueTable() { Clear(); }

	void				BeginAddColumns();
	void				AddColumn(CStrID ID, const Data::CType* Type, bool IsNew = true);
	void				EndAddColumns();
	bool				HasColumn(CStrID ID) const { return ColumnIndexMap.Contains(ID); }
	int					GetColumnIndex(CStrID ID) const;
	UPTR				GetColumnCount() const { return Columns.GetCount(); }
	CStrID				GetColumnID(UPTR ColIdx) const { return Columns[ColIdx].ID; }
	const Data::CType*	GetColumnValueType(UPTR ColIdx) const { return Columns[ColIdx].Type; }
	const CArray<int>&	GetNewColumnIndices() { return NewColumnIndices; }

	void				Clear();
	void				ReserveRows(UPTR NumRows) { n_assert(NumRows > 0); Realloc(RowPitch, AllocatedRows + NumRows); }
	int					AddRow();
	int					CopyRow(UPTR FromRowIdx);
	int					CopyExtRow(CValueTable* pOther, UPTR OtherRowIdx, bool CreateMissingCols = false);
	void				DeleteRow(UPTR RowIdx);
	void				DeleteAllRows() { for (UPTR i = 0; i < NumRows; i++) DeleteRow(i); }
	void				DeleteRowData(UPTR RowIdx);

	U8					GetRowState(UPTR RowIdx) const;
	bool				IsRowNew(UPTR RowIdx) const;
	bool				IsRowUpdated(UPTR RowIdx) const;
	bool				IsRowOnlyUpdated(UPTR RowIdx) const;
	bool				IsRowDeleted(UPTR RowIdx) const;
	bool				IsRowValid(UPTR RowIdx) const;
	bool				IsRowUntouched(UPTR RowIdx) const;
	bool				IsRowModified(UPTR RowIdx) const;
	bool				IsModified() const { return Flags.Is(_IsModified); }
	bool				HasModifiedRows() const { return Flags.Is(_HasModifiedRows); }
	void				TrackModifications(bool Track) { Flags.SetTo(_TrackModifications, Track); }
	bool				IsTrackingModifications() const { return Flags.Is(_TrackModifications); }
	void				ClearNewRowStats();
	void				ClearDeletedRows();
	void				ClearRowFlags(UPTR RowIdx);
	void				ResetModifiedState();
	UPTR				GetRowCount() const { return NumRows; }
	int					GetFirstNewRowIndex() const { return FirstNewRowIndex; }
	int					GetNewRowsCount() const { return NewRowsCount; }
	int					GetFirstDeletedRowIndex() const { return FirstDeletedRowIndex; }
	int					GetDeletedRowsCount() const { return DeletedRowsCount; }

	CArray<int>			FindRowIndicesByValue(CStrID AttrID, const Data::CData& Value, bool FirstMatchOnly) const;
	int					FindRowIndexByValue(CStrID AttrID, const Data::CData& Value) const;

	void				GetValue(UPTR ColIdx, UPTR RowIdx, Data::CData& Val) const;
	void				GetValue(CStrID AttrID, UPTR RowIdx, Data::CData& Val) const { GetValue(ColumnIndexMap[AttrID], RowIdx, Val); }
	template<class T>
	const T&			Get(UPTR ColIdx, UPTR RowIdx) const;
	template<class T>
	const T&			Get(CStrID AttrID, UPTR RowIdx) const { return Get<T>(ColumnIndexMap[AttrID], RowIdx); }
	void				SetValue(UPTR ColIdx, UPTR RowIdx, const Data::CData& Val);
	void				SetValue(CStrID AttrID, UPTR RowIdx, const Data::CData& Val) { SetValue(ColumnIndexMap[AttrID], RowIdx, Val); }
	template<class T>
	void				Set(UPTR ColIdx, UPTR RowIdx, const T& Val);
	template<class T>
	void				Set(CStrID AttrID, UPTR RowIdx, const T& Val) { Set(ColumnIndexMap[AttrID], RowIdx, Val); }
};

inline CValueTable::CValueTable():
	Columns(12, 4),
	NewColumnIndices(8, 8),
	FirstAddedColIndex(INVALID_INDEX),
	FirstNewRowIndex(INTPTR_MAX),
	NewRowsCount(0),
	FirstDeletedRowIndex(INTPTR_MAX),
	DeletedRowsCount(0),
	RowPitch(0),
	NumRows(0),
	AllocatedRows(0),
	ValueBuffer(NULL),
	RowStateBuffer(NULL),
	Flags(_TrackModifications)
{
}
//---------------------------------------------------------------------

inline void CValueTable::ClearNewRowStats()
{
	FirstNewRowIndex = INTPTR_MAX;
	NewRowsCount = 0;
}
//---------------------------------------------------------------------

// Doesn't update modification statistics
inline void CValueTable::ClearRowFlags(UPTR RowIdx)
{
	n_assert(RowIdx < NumRows);
	RowStateBuffer[RowIdx] = 0;
}
//---------------------------------------------------------------------

inline void CValueTable::ResetModifiedState()
{
	Flags.Clear(_IsModified);
	Flags.Clear(_HasModifiedRows);
}
//---------------------------------------------------------------------

inline bool CValueTable::IsRowModified(UPTR RowIdx) const
{
	n_assert(RowIdx < NumRows);
	return RowStateBuffer[RowIdx] != 0;
}
//---------------------------------------------------------------------

inline U8 CValueTable::GetRowState(UPTR RowIdx) const
{
	n_assert(RowIdx < NumRows);
	return RowStateBuffer[RowIdx];
}
//---------------------------------------------------------------------

inline bool CValueTable::IsRowNew(UPTR RowIdx) const
{
	n_assert(RowIdx < NumRows);
	return (RowStateBuffer[RowIdx] & NewRow) != 0;
}
//---------------------------------------------------------------------

inline bool CValueTable::IsRowUpdated(UPTR RowIdx) const
{
	n_assert(RowIdx < NumRows);
	return (RowStateBuffer[RowIdx] & UpdatedRow) != 0;
}
//---------------------------------------------------------------------

inline bool CValueTable::IsRowOnlyUpdated(UPTR RowIdx) const
{
	n_assert(RowIdx < NumRows);
	return RowStateBuffer[RowIdx] == UpdatedRow;
}
//---------------------------------------------------------------------

inline bool CValueTable::IsRowDeleted(UPTR RowIdx) const
{
	n_assert(RowIdx < NumRows);
	return (RowStateBuffer[RowIdx] & DeletedRow) != 0;
}
//---------------------------------------------------------------------

// Returns true if row isn't deleted or destroyed
inline bool CValueTable::IsRowValid(UPTR RowIdx) const
{
	n_assert(RowIdx < NumRows); //???incorporate into return value?
	return (RowStateBuffer[RowIdx] & (DeletedRow | DestroyedRow)) == 0;
}
//---------------------------------------------------------------------

inline bool CValueTable::IsRowUntouched(UPTR RowIdx) const
{
	n_assert(RowIdx < NumRows); //???incorporate into return value?
	return RowStateBuffer[RowIdx] == 0;
}
//---------------------------------------------------------------------

inline int CValueTable::GetColumnIndex(CStrID ID) const
{
	int Idx = ColumnIndexMap.FindIndex(ID);
	return (Idx != INVALID_INDEX) ? ColumnIndexMap[ID] : INVALID_INDEX;
}
//---------------------------------------------------------------------

inline void** CValueTable::GetValuePtr(UPTR ColIdx, UPTR RowIdx) const
{
	n_assert(ColIdx < Columns.GetCount() && RowIdx < NumRows);
	return (void**)((char*)ValueBuffer + RowIdx * RowPitch + Columns[ColIdx].ByteOffset);
}
//---------------------------------------------------------------------

template<class T> inline void CValueTable::Set(UPTR ColIdx, UPTR RowIdx, const T& Val)
{
	const Data::CType* Type = GetColumnValueType(ColIdx);
	n_assert(!Type || Type == DATA_TYPE(T));
	n_assert(IsRowValid(RowIdx));

	void** pObj = GetValuePtr(ColIdx, RowIdx);
	if (Type) DATA_TYPE_NV(T)::CopyT(IsSpecialType(Type) ? (void**)&pObj : pObj, &Val);
	else *(Data::CData*)pObj = Val;

	if (Flags.Is(_TrackModifications))
	{
		RowStateBuffer[RowIdx] |= UpdatedRow;
		Flags.Set(_IsModified);
		Flags.Set(_HasModifiedRows);
	}
}
//---------------------------------------------------------------------

inline void CValueTable::GetValue(UPTR ColIdx, UPTR RowIdx, Data::CData& Val) const
{
	const Data::CType* Type = GetColumnValueType(ColIdx);
	void** pObj = GetValuePtr(ColIdx, RowIdx);
	if (Type) Val.SetTypeValue(Type, IsSpecialType(Type) ? (void**)&pObj : pObj);
	else Val = *(Data::CData*)pObj;
}
//---------------------------------------------------------------------

template<class T> inline const T& CValueTable::Get(UPTR ColIdx, UPTR RowIdx) const
{
	const Data::CType* Type = GetColumnValueType(ColIdx);
	n_assert(!Type || Type == DATA_TYPE(T));
	void** pObj = GetValuePtr(ColIdx, RowIdx);
	return (Type) ?
		*(T*)DATA_TYPE_NV(T)::GetPtr(IsSpecialType(Type) ? (void**)&pObj : pObj) :
		((Data::CData*)pObj)->GetValue<T>();
}
//---------------------------------------------------------------------

// Finds multiple row indices by matching attribute. This method can be slow since
// it may search linearly (and vertically) through the table.
inline CArray<int> CValueTable::FindRowIndicesByValue(CStrID AttrID, const Data::CData& Value, bool FirstMatchOnly) const
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
inline int CValueTable::FindRowIndexByValue(CStrID AttrID, const Data::CData& Value) const
{
	CArray<int> RowIndices = InternalFindRowIndicesByAttr(AttrID, Value, true);
	return (RowIndices.GetCount() == 1) ? RowIndices[0] : INVALID_INDEX; //???or if > 0?
}
//---------------------------------------------------------------------

// Finds single row index by multiple matching attributes. This method can be slow since
// it may search linearly (and vertically) through the table.
//inline int CValueTable::FindRowIndexByAttrs(const nArray<CAttr>& Attrs) const
//{
//	nArray<int> RowIndices = InternalFindRowIndicesByAttrs(Attrs, true);
//	return (RowIndices.Size() == 1) ? RowIndices[0] : INVALID_INDEX; //???or if > 0?
//}
////---------------------------------------------------------------------

inline void CValueTable::SetRowToDefaultValues(UPTR RowIdx)
{
	for (UPTR ColIdx = 0; ColIdx < GetColumnCount(); ++ColIdx)
		SetValue(ColIdx, RowIdx, Data::CData(GetColumnValueType(ColIdx)));
}
//---------------------------------------------------------------------

inline void CValueTable::SetColumnToDefaultValues(UPTR ColIdx)
{
	Data::CData DefVal(GetColumnValueType(ColIdx));
	for (UPTR RowIdx = 0; RowIdx < GetRowCount(); ++RowIdx)
		SetValue(ColIdx, RowIdx, DefVal);
}
//---------------------------------------------------------------------

}

#endif