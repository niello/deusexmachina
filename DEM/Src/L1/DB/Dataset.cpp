#include "Dataset.h"

#include "Database.h"

// Const strings to prevent excessive string object construction
extern const nString CommaFrag(",");
extern const nString TickFrag("\"");
extern const nString CloseBracketFrag(")");
extern const nString DeleteFromFrag("DELETE FROM ");
extern const nString WhereFrag(" WHERE ");
const nString SelectFrag("SELECT ");
const nString FromFrag(" FROM ");
const nString StarFrag("*");
const nString InsertIntoFrag("INSERT INTO ");
const nString OpenBracketFrag(" (");
const nString ValuesFrag(") VALUES (");
const nString WildcardFrag("?");
const nString UpdateFrag("UPDATE ");
const nString SetFrag(" SET ");
const nString AssignWildcardFrag("=?");
const nString AndFrag(" AND ");

const char* CompileSQLErrorMsg = "Error compiling SQL statement:\n%s\nWith error:\n%s\n";
const char* ExecSQLErrorMsg = "Error executing SQL statement:\n%s\nWith error:\n%s\n";

namespace DB
{

CDataset::CDataset(const PTable& HostTable): Table(HostTable), RowIdx(-1)
{
	VT = CValueTable::Create();
}
//---------------------------------------------------------------------

void CDataset::AddColumns(const CAttrID* AttrIDs, DWORD Count)
{
	n_assert(AttrIDs && Count);
	VT->BeginAddColumns();
	for (DWORD i = 0; i < Count; i++) VT->AddColumn(AttrIDs[i]);
	VT->EndAddColumns();
}
//---------------------------------------------------------------------

void CDataset::AddColumnsFromTable()
{
	n_assert(Table.IsValid());
	VT->BeginAddColumns();
	for (int ColIdx = 0; ColIdx < Table->GetNumColumns(); ColIdx++)
		VT->AddColumn(Table->GetColumn(ColIdx).AttrID, false);
	VT->EndAddColumns();
	InvalidateCommands();
}
//---------------------------------------------------------------------

void CDataset::PerformQuery(bool AppendResult)
{
	n_assert(VT.IsValid());
	n_assert(Table.IsValid());

	if (!AppendResult) VT->Clear();

	// Query will not compile if DB is missing some requested columns
	if (Table->HasUncommittedColumns()) Table->CommitUncommittedColumns();

	if (!CmdSelect.IsValid()) CmdSelect = CCommand::Create();
	if (!CmdSelect->IsValid()) CompileSelectCmd();

	//!!!BIND WHERE!

	if (!CmdSelect->Execute(Table->GetDB()))
		n_error(ExecSQLErrorMsg, CmdSelect->GetSQL().CStr(), CmdSelect->GetError().CStr());
}
//---------------------------------------------------------------------

// This writes any uncommitted changes to the table layout back into the database.
// NOTE: it is not possible to make an existing column primary, or to create an index
// for an existing column, this must happen when the table is actually created!
// This method also does not deletion of columns (SQLite doesn't support column removal).
void CDataset::CommitChanges(bool UseTransaction)
{
	if (VT->GetNewColumnIndices().GetCount() > 0)
	{
		for (int i = 0; i < VT->GetNewColumnIndices().GetCount(); i++)
			Table->AddColumn(VT->GetColumnID(VT->GetNewColumnIndices()[i]));
		VT->GetNewColumnIndices().Clear();
		InvalidateCommands();
	}

	if (Table->HasUncommittedColumns()) Table->CommitUncommittedColumns();

	if (VT->IsModified())
	{
		// Updating and deleting only works for tables with primary column
		if (Table->HasPrimaryColumn())
		{
			if (VT->GetDeletedRowsCount() > 0)
			{
				if (!CmdDelete.IsValid()) CmdDelete = CCommand::Create();
				if (!CmdDelete->IsValid()) CompileDeleteCmd();
			}

			//!!!HasModifiedRows -> GetUpdatedRowsCount! (the same stats as for New & Deleted needed)!
			if (VT->HasModifiedRows())
			{
				if (!CmdUpdate.IsValid()) CmdUpdate = CCommand::Create();
				if (!CmdUpdate->IsValid()) CompileUpdateCmd();
			}
		}

		if (VT->GetNewRowsCount() > 0)
		{
			if (!CmdInsert.IsValid()) CmdInsert = CCommand::Create();
			if (!CmdInsert->IsValid()) CompileInsertCmd();
		}

		if (UseTransaction) Table->GetDB()->BeginTransaction();

		if (Table->HasPrimaryColumn())
		{
			if (VT->GetDeletedRowsCount() > 0) ExecuteDeleteCmd();

			//!!!HasModifiedRows -> GetUpdatedRowsCount()! (the same stats as for New & Deleted needed)!
			if (VT->HasModifiedRows()) ExecuteUpdateCmd();
		}

		// Insert new data (unless we're told to update new rows with an UPDATE command)
		if (VT->GetNewRowsCount() > 0) ExecuteInsertCmd();

		if (UseTransaction) Table->GetDB()->EndTransaction();

		VT->ResetModifiedState();
	}
}
//---------------------------------------------------------------------

// Immediately delete rows from the database.
void CDataset::CommitDeletedRows(bool UseTransaction)
{
	n_assert(VT.IsValid());

	if (Table->HasPrimaryColumn() && VT->GetDeletedRowsCount() > 0)
	{
		if (!CmdDelete.IsValid()) CmdDelete = CCommand::Create();
		if (!CmdDelete->IsValid()) CompileDeleteCmd();
		if (UseTransaction) Table->GetDB()->BeginTransaction();
		ExecuteDeleteCmd();
		if (UseTransaction) Table->GetDB()->EndTransaction();
	}
}
//---------------------------------------------------------------------

void CDataset::InvalidateCommands()
{
	if (CmdSelect.IsValid() && CmdSelect->IsValid()) CmdSelect->Clear();
	if (CmdInsert.IsValid() && CmdInsert->IsValid()) CmdInsert->Clear();
	if (CmdUpdate.IsValid() && CmdUpdate->IsValid()) CmdUpdate->Clear();
	if (CmdDelete.IsValid() && CmdDelete->IsValid()) CmdDelete->Clear();
}
//---------------------------------------------------------------------

void CDataset::CompileSelectCmd()
{
	nString SQL;
	//SQL.Reserve(4096);

	if (SelectSQL.IsEmpty())
	{
		SQL = SelectFrag;

		if (VT->GetNumColumns())
		{
			// We just committed all Table columns into DB so we can remove corresponding VT NewColumnIndices
			nArray<bool> ColumnIsInDB(VT->GetNumColumns(), 0, true);
			for (int i = VT->GetNewColumnIndices().GetCount() - 1; i >= 0; i--)
			{
				int ColIdx = VT->GetNewColumnIndices()[i];
				if (Table->HasColumn(VT->GetColumnID(ColIdx))) VT->GetNewColumnIndices().Erase(i);
				else ColumnIsInDB[ColIdx] = false;
			}

			bool First = true;
			for (int ColIdx = 0; ColIdx < VT->GetNumColumns(); ColIdx++)
				if (ColumnIsInDB[ColIdx])
				{
					if (First) First = false;
					else SQL.Append(CommaFrag);
					SQL.Append(TickFrag);
					SQL.Append(VT->GetColumnName(ColIdx));
					SQL.Append(TickFrag);
				}
		}
		else SQL.Append(StarFrag);

		SQL.Append(FromFrag);
		SQL.Append(Table->GetName());
	}
	else SQL = SelectSQL;

	if (WhereSQL.IsValid())
	{
		SQL.Append(WhereFrag);
		SQL.Append(WhereSQL);
	}

	CmdSelect->SetSQL(SQL);
	CmdSelect->SetResultTable(VT);

	if (!CmdSelect->Compile(Table->GetDB()))
		n_error(CompileSQLErrorMsg, CmdSelect->GetSQL().CStr(), CmdSelect->GetError().CStr());
}
//---------------------------------------------------------------------

void CDataset::CompileInsertCmd()
{
	n_assert(CmdInsert.IsValid());
	n_assert(VT.IsValid());

	nString SQL;
	//SQL.Reserve(4096);
	SQL.Append(InsertIntoFrag);
	SQL.Append(Table->GetName());
	SQL.Append(OpenBracketFrag);

	// For an insert command we need to write ALL columns not just the ReadWrite columns
	for (int ColIdx = 0; ColIdx < VT->GetNumColumns(); ColIdx++)
	{
		SQL.Append(TickFrag);
		SQL.Append(VT->GetColumnName(ColIdx));
		SQL.Append(TickFrag);
		if (ColIdx < VT->GetNumColumns() - 1) SQL.Append(CommaFrag);
	}
	
	SQL.Append(ValuesFrag);
	
	for (int ColIdx = 0; ColIdx < VT->GetNumColumns(); ColIdx++)
	{
		SQL.Append(WildcardFrag);
		if (ColIdx < VT->GetNumColumns() - 1) SQL.Append(CommaFrag);
	}
	
	SQL.Append(CloseBracketFrag);

	CmdInsert->SetSQL(SQL);

	if (!CmdInsert->Compile(Table->GetDB()))
		n_error(CompileSQLErrorMsg, CmdInsert->GetSQL().CStr(), CmdInsert->GetError().CStr());
}
//---------------------------------------------------------------------

void CDataset::CompileUpdateCmd()
{
	n_assert(Table->HasPrimaryColumn());
	//n_assert(database.IsValid());
	n_assert(CmdUpdate.IsValid());
	n_assert(VT.IsValid());

	const nArray<int>& PKColumnIndices = Table->GetPKColumnIndices();
	int NonPKCount = VT->GetNumColumns() - PKColumnIndices.GetCount();
	if (PKVTColumns.GetCount() < PKColumnIndices.GetCount()) PKVTColumns.Reallocate(PKColumnIndices.GetCount(), 0);
	else PKVTColumns.Clear();
	if (NonPKVTColumns.GetCount() < NonPKCount) NonPKVTColumns.Reallocate(NonPKCount, 0);
	else NonPKVTColumns.Clear();
	for (int i = 0; i < Table->GetNumColumns(); i++)
	{
		const CColumn& CurrCol = Table->GetColumn(i);
		bool IsPrimary = CurrCol.Type & CColumn::Primary;

// Editor should be able to update ReadOnly rows
#ifndef _EDITOR
		if (!IsPrimary && CurrCol.GetAccessMode() == ReadOnly) continue;
#endif

		int VTColIdx = VT->GetColumnIndex(CurrCol.AttrID);
		if (VTColIdx != INVALID_INDEX)
		{
			if (IsPrimary) PKVTColumns.Append(VTColIdx);
			else NonPKVTColumns.Append(VTColIdx);
		}
		else if (IsPrimary)
			n_error("CTable::SeparateValueTableByPK(): some primary columns aren't present in VT");
	}

	if (NonPKVTColumns.GetCount() == 0) return;

	nString SQL;
	//SQL.Reserve(4096);
	SQL.Append(UpdateFrag);
	SQL.Append(Table->GetName());
	SQL.Append(SetFrag);

	bool First = true;
	for (int i = 0; i < NonPKVTColumns.GetCount(); i++)
	{
		if (First) First = false;
		else SQL.Append(CommaFrag);
		SQL.Append(TickFrag);
		SQL.Append(VT->GetColumnName(NonPKVTColumns[i]).CStr());
		SQL.Append(TickFrag);
		SQL.Append(AssignWildcardFrag);
	}

	SQL.Append(WhereFrag);

	First = true;
	for (int i = 0; i < PKVTColumns.GetCount(); i++)
	{
		if (First) First = false;
		else SQL.Append(AndFrag);
		SQL.Append(TickFrag);
		SQL.Append(VT->GetColumnName(PKVTColumns[i]).CStr());
		SQL.Append(TickFrag);
		SQL.Append(AssignWildcardFrag);
	}

	CmdUpdate->SetSQL(SQL);

	if (!CmdUpdate->Compile(Table->GetDB()))
		n_error(CompileSQLErrorMsg, CmdUpdate->GetSQL().CStr(), CmdUpdate->GetError().CStr());
}
//---------------------------------------------------------------------

void CDataset::CompileDeleteCmd()
{
	n_assert(CmdDelete.IsValid());

	nString SQL;
	//SQL.Reserve(1024);
	SQL.Append(DeleteFromFrag);
	SQL.Append(Table->GetName());
	SQL.Append(WhereFrag);
	
	const nArray<int>& PKColumnIndices = Table->GetPKColumnIndices();
	for (int i = 0; i < PKColumnIndices.GetCount(); i++)
	{
		SQL.Append(TickFrag);
		SQL.Append(Table->GetPrimaryColumn(i).GetName());
		SQL.Append(TickFrag);
		SQL.Append(AssignWildcardFrag);
		if (i < PKColumnIndices.GetCount() - 1) SQL.Append(AndFrag);
	}

	CmdDelete->SetSQL(SQL);

	if (!CmdDelete->Compile(Table->GetDB()))
		n_error(CompileSQLErrorMsg, CmdDelete->GetSQL().CStr(), CmdDelete->GetError().CStr());
}
//---------------------------------------------------------------------

void CDataset::ExecuteInsertCmd()
{
	n_assert(CmdInsert.IsValid() && CmdInsert->GetSQL().IsValid());
	n_assert(VT);

	const nArray<int>& PKColumnIndices = Table->GetPKColumnIndices();

	int IntPKColIdx =
		(PKColumnIndices.GetCount() == 1 && Table->GetPrimaryColumn().GetValueType() == TInt) ?
		PKColumnIndices[0] :
		INVALID_INDEX;

	int RowIdx = VT->GetFirstNewRowIndex();
	int Added = 0;
	for (; RowIdx < VT->GetRowCount() && Added < VT->GetNewRowsCount(); RowIdx++)
	{
		// If row has NewRow flag, it isn't deleted in a current CValueTable logic.
		if (!VT->IsRowNew(RowIdx)) continue;

		++Added;

		CData Value;
		for (int ColIdx = 0; ColIdx < VT->GetNumColumns(); ColIdx++)
		{
			if (ColIdx == IntPKColIdx)
			{
				// If we insert into table with INTEGER PK (ROWID alias, see SQLite manual), we bind
				// NULL PK (do no binding, which is equal) to benefit from the ROWID autoincrementing.
				// We bind NULL (autoinc) only if value from VT == 0. Else we use explicit VT value.
				int PKValue = VT->Get<int>(ColIdx, RowIdx);
				if (PKValue) CmdInsert->BindValue(ColIdx, PKValue);
			}
			else
			{
				VT->GetValue(ColIdx, RowIdx, Value);
				CmdInsert->BindValue(ColIdx, Value);
			}
		}

		if (!CmdInsert->Execute(Table->GetDB()))
			n_error("CTable::ExecuteInsertCmd(): error in row '%d' for command '%s' with '%s'", 
				RowIdx,
				CmdInsert->GetSQL().CStr(), 
				CmdInsert->GetError().CStr());

		VT->ClearRowFlags(RowIdx);
	}
	
	VT->ClearNewRowStats();
}
//---------------------------------------------------------------------

void CDataset::ExecuteUpdateCmd()
{
	n_assert(Table->HasPrimaryColumn());
	n_assert(VT.IsValid());
	n_assert(CmdUpdate.IsValid() && CmdUpdate->IsValid());

	if (NonPKVTColumns.GetCount() == 0) return;

	//???organize as new & deleted? first idx & count
	for (int RowIdx = 0; RowIdx < VT->GetRowCount(); RowIdx++)
	{
		// Check whether the row was updated, but not added or deleted
		if (!VT->IsRowOnlyUpdated(RowIdx)) continue;

		int WildCardIdx = 0;

		for (int ColIdx = 0; ColIdx < NonPKVTColumns.GetCount(); ColIdx++)
		{
			CData Value;
			VT->GetValue(NonPKVTColumns[ColIdx], RowIdx, Value);
			CmdUpdate->BindValue(WildCardIdx++, Value);
		}

		// Bind primary key(s) to WHERE statement
		for (int ColIdx = 0; ColIdx < PKVTColumns.GetCount(); ColIdx++)
		{
			CData Value;
			VT->GetValue(PKVTColumns[ColIdx], RowIdx, Value);
			CmdUpdate->BindValue(WildCardIdx++, Value);
		}

		if (!CmdUpdate->Execute(Table->GetDB()))
			n_error("CDataset::ExecuteUpdateCmd(): error in row '%d' for command '%s'", 
				RowIdx, CmdUpdate->GetSQL().CStr());

		VT->ClearRowFlags(RowIdx);
	}

	//!!!if will be stats, clear here!
}
//---------------------------------------------------------------------

void CDataset::ExecuteDeleteCmd()
{
	n_assert(CmdDelete.IsValid() && CmdDelete->IsValid());
	n_assert(Table->HasPrimaryColumn());

	const nArray<int>& PKColumnIndices = Table->GetPKColumnIndices();
	nArray<int> PKVTColumns(PKColumnIndices.GetCount(), 0);
	for (int i = 0; i < PKColumnIndices.GetCount(); i++)
	{
		int VTColIdx = VT->GetColumnIndex(Table->GetPrimaryColumn(i).AttrID);
		if (VTColIdx != INVALID_INDEX)
		{
			PKVTColumns.Append(VTColIdx);
			if (PKVTColumns.GetCount() == PKColumnIndices.GetCount()) break;
		}
		else n_error("CTable::CompileUpdateCommand(): some primary columns aren't present in VT");
	}

	if (VT->GetNumColumns() == PKColumnIndices.GetCount()) return;

	int RowIdx = VT->GetFirstDeletedRowIndex();
	int Deleted = 0;
	for (; RowIdx < VT->GetRowCount() && Deleted < VT->GetDeletedRowsCount(); RowIdx++)
	{
		if (!VT->IsRowDeleted(RowIdx)) continue;

		++Deleted;

		int WildCardIdx = 0;
		for (int j = 0; j < PKVTColumns.GetCount(); j++)
		{
			CData Value;
			VT->GetValue(PKVTColumns[j], RowIdx, Value);
			CmdDelete->BindValue(WildCardIdx++, Value);
		}

		if (!CmdDelete->Execute(Table->GetDB()))
			n_error("CDataset::ExecuteDeleteCmd(): error in row '%d' for command '%s': %s", 
				RowIdx, 
				CmdDelete->GetSQL().CStr(), 
				CmdDelete->GetError().CStr());
	}

	VT->ClearDeletedRows();
}
//---------------------------------------------------------------------

} // namespace DB