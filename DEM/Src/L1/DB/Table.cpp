#include "Table.h"

#include "DBServer.h"
#include "Dataset.h"
#include "Database.h"

namespace Attr
{
	DeclareString(name);
	DeclareBool(pk);
}

// Const strings to prevent excessive string object construction
extern const nString CommaFrag;
extern const nString TickFrag;
extern const nString CloseBracketFrag;
extern const nString DeleteFromFrag;
extern const nString WhereFrag;
const nString IntegerFrag(" INTEGER");
const nString RealFrag(" REAL");
const nString TextFrag(" TEXT");
const nString BlobFrag(" BLOB");

namespace DB
{
__ImplementClass(DB::CTable, 'DBTB', Core::CRefCounted);

CTable::CTable(): _IsConnected(false), Columns(0, 4)
{
}
//---------------------------------------------------------------------

// NOTE: destroying a Table object does not automatically commit changes!
// Changes will be lost if CommitChanges() isn't called manually. This is
// because the Table could have been dropped from the db, in this case
// a CommitChanges() would add unnessecary overhead.
CTable::~CTable()
{
	if (IsConnected()) Disconnect(false);
}
//---------------------------------------------------------------------

void CTable::SetName(const nString& newName)
{
	if (IsConnected())
	{
		nString SQL;
		SQL.Format("ALTER TABLE '%s' RENAME TO '%s'", Name.CStr(), newName.CStr());
		PCommand Cmd = CCommand::Create();
		Cmd->Execute(Database, SQL);
	}
	Name = newName;
}
//---------------------------------------------------------------------

// Add a new Column to the table. The change will not be synchronized with
// the Database until a CommitChanges() is called on the Table object.
void CTable::AddColumn(const CColumn& NewColumn)
{
	if (!HasColumn(NewColumn.AttrID))
	{
		Columns.Append(NewColumn);
		NameIdxMap.Add(NewColumn.GetName().CStr(), Columns.GetCount() - 1);
		AttrIDIdxMap.Add(NewColumn.AttrID, Columns.GetCount() - 1);
		if (NewColumn.Type & CColumn::Primary) PKColumnIndices.Append(Columns.GetCount() - 1);
	}
}
//---------------------------------------------------------------------

PDataset CTable::CreateDataset()
{
	return PDataset(n_new(CDataset)(this));
}
//---------------------------------------------------------------------

// Private helper method which checks if an associated Database table exists.
bool CTable::TableExists()
{
	n_assert(Database.IsValid());
	n_assert(Name.IsValid());

	nString SQL;
	SQL.Format("SELECT Name FROM 'sqlite_master' WHERE type='table' AND Name='%s'", Name.CStr());
	PCommand Cmd = CCommand::Create();
	PValueTable result = CValueTable::Create();
	Cmd->Execute(Database, SQL, result);
	return (result->GetRowCount() > 0);
}
//---------------------------------------------------------------------

// Private helper method which deletes the associated Database table.
void CTable::DropTable()
{
	n_assert(Database.IsValid());
	n_assert(Name.IsValid());

	nString SQL;
	SQL.Format("DROP TABLE '%s'", Name.CStr());
	PCommand Cmd = CCommand::Create();
	Cmd->Execute(Database, SQL);
}
//---------------------------------------------------------------------

nString CTable::BuildColumnDef(const CColumn& Column)
{
    nString Def(TickFrag);
    Def.Append(Column.GetName());
    Def.Append(TickFrag);
	const CType* Type = Column.GetValueType();
    if (Type == TInt || Type == TBool) Def.Append(IntegerFrag);
	else if (Type == TFloat) Def.Append(RealFrag);
	else if (Type == TString || Type == TStrID) Def.Append(TextFrag);
	else if (Type != NULL) Def.Append(BlobFrag);

	// when Type == NULL Column is dynamically typed

	return Def;
}
//---------------------------------------------------------------------

void CTable::CreateTable()
{
	n_assert(Database.IsValid());
	n_assert(Name.IsValid());

	nString SQL;
	SQL.Format("CREATE TABLE '%s' ( ", Name.CStr());
	nString PK;
	for (int ColIdx = 0; ColIdx < GetNumColumns(); ColIdx++)
	{
		CColumn& Column = Columns[ColIdx];
		Column.Committed = true;
		SQL.Append(BuildColumnDef(Column));

		if (Column.Type & CColumn::Primary)
		{
			if (PK.IsValid()) PK.Append(CommaFrag);
			PK.Append(Column.GetName());
		}

		if (ColIdx < GetNumColumns() - 1) SQL.Append(CommaFrag);
	}

	if (PK.IsValid())
	{
		SQL.Append(", PRIMARY KEY (");
		SQL.Append(PK);
		SQL.Append(") ON CONFLICT REPLACE");
	}

	SQL.Append(CloseBracketFrag);

	// execute the SQL statement
	PCommand Cmd = CCommand::Create();
	Cmd->Execute(Database, SQL);

	// create indices
	for (int Col = 0; Col < GetNumColumns(); Col++)
	{
		const CColumn& Column = GetColumn(Col);
		if (Column.Type & CColumn::Indexed)
		{
			n_assert(!(Column.Type & CColumn::Primary));
			SQL.Format("CREATE INDEX %s_%s ON %s ( %s )", 
			GetName().CStr(),
			Column.GetName().CStr(), 
			GetName().CStr(), 
			Column.GetName().CStr());
			Cmd->Execute(Database, SQL);
		}
    }
}
//---------------------------------------------------------------------

// Synchronizes column list between this table & DB table, OR-ing column sets
void CTable::ReadTableLayout(bool IgnoreUnknownColumns)
{
	n_assert(Database.IsValid());
	n_assert(Name.IsValid());

	nString SQL;
	SQL.Format("PRAGMA table_info(%s)", Name.CStr());
	PCommand Cmd = CCommand::Create();
	PValueTable TblInfo = CValueTable::Create();
	Cmd->Execute(Database, SQL, TblInfo);

	// Each row in the result describes a Column
	for (int i = 0; i < TblInfo->GetRowCount(); i++)
	{
		const nString& ColName = TblInfo->Get<nString>(Attr::name, i);
		if (DBSrv->IsValidAttrName(ColName))
		{
			CAttrID AttrID = DBSrv->FindAttrID(ColName.CStr());
			if (!HasColumn(AttrID))
			{
				CColumn NewColumn;
				NewColumn.AttrID = AttrID;
				if (TblInfo->Get<bool>(Attr::pk, i))
					NewColumn.Type = CColumn::Primary;
				NewColumn.Committed = true;
				AddColumn(NewColumn);
			}
		}
		else if (!IgnoreUnknownColumns)
			n_error("CTable::ReadTableLayout(): invalid Column '%s' in table '%s'!", 
				ColName.CStr(), Name.CStr());
	}

	SQL.Format("PRAGMA index_list(%s)", GetName().CStr());
	PValueTable IndexList = CValueTable::Create();
	Cmd->Execute(Database, SQL, IndexList);
	for (int i = 0; i < IndexList->GetRowCount(); i++)
	{
		const nString& IndexName = IndexList->Get<nString>(Attr::name, i);
		if (!IndexName.MatchPattern("sqlite_*"))
		{
			PCommand IndexInfoCmd = CCommand::Create();
			PValueTable IndexInfo = CValueTable::Create();
			SQL.Format("PRAGMA index_info(%s)", IndexName.CStr());
			IndexInfoCmd->Execute(Database, SQL, IndexInfo);
			if (1 == IndexInfo->GetRowCount())
			{
				// we only look at single-Column indices
				const nString& ColName = IndexInfo->Get<nString>(Attr::name, 0);
				if (DBSrv->IsValidAttrName(ColName))
				{
					CAttrID AttrID = DBSrv->FindAttrID(ColName.CStr());
					if (HasColumn(AttrID))
					{
						CColumn& Column = const_cast<CColumn&>(GetColumn(AttrID));
						if (Column.Type != CColumn::Primary) Column.Type = CColumn::Indexed;
					}
				}
			}
		}
	}
}
//---------------------------------------------------------------------

// This connects the table object with a Database, from that moment on,
// all changes to the table object will be synchronized with the Database.
// Changes will be batched until CommitChanges() is called. 
void CTable::Connect(const PDatabase& db, ConnectMode connectMode, bool IgnoreUnknownColumns)
{
	n_assert(!_IsConnected);
	n_assert(Name.IsValid());
	n_assert(db);
	
	Database = db;
	_IsConnected = true;

	switch (connectMode)
	{
		case ForceCreate:
			n_assert(!TableExists());
			CreateTable();
			break;
		case AssumeExists:
			ReadTableLayout(IgnoreUnknownColumns);
			break;
		default:
			if (TableExists()) ReadTableLayout(IgnoreUnknownColumns);
			else CreateTable();
	}
}
//---------------------------------------------------------------------

// This disconnects the table from the Database. If the dropTable argument is true
// (default is false), the actual Database table will be deleted as well. Otherwise,
// a CommitChanges is invoked to write any local changes back into the Database,
// and just the C++ table object is disconnected.
void CTable::Disconnect(bool DropDBTable)
{
	n_assert(_IsConnected);
	if (DropDBTable) DropTable();
	Database = NULL;
	_IsConnected = false;
}
//---------------------------------------------------------------------

// This checks for uncommited Column and alters the Database table structure accordingly.
void CTable::CommitUncommittedColumns()
{
	n_assert(HasUncommittedColumns());

	PCommand Cmd = CCommand::Create();

	nString SQL;
	for (int i = 0; i < Columns.GetCount(); i++)
	{
		CColumn& Column = Columns[i];
		if (!Column.Committed)
		{
			Column.Committed = true;

			// note: it is illegal to add primary or unique Columns after table creation
			n_assert(!(Column.Type & CColumn::Primary))
			SQL.Format("ALTER TABLE '%s' ADD COLUMN ", Name.CStr());
			SQL.Append(BuildColumnDef(Column));
			n_assert(Cmd->Execute(Database, SQL));

			// need to create an index for the new Column?
			if (Column.Type & CColumn::Indexed)
			{
				SQL.Format("CREATE INDEX %s_%s ON %s ( %s )", 
					GetName().CStr(),
					Column.GetName().CStr(), 
					GetName().CStr(), 
					Column.GetName().CStr());
				n_assert(Cmd->Execute(Database, SQL));
			}
		}
	}
}
//---------------------------------------------------------------------

void CTable::DeleteWhere(const nString& WhereSQL)
{
	n_assert(WhereSQL.IsValid());
	//if (WhereSQL.IsEmpty()) Truncate();
	PCommand Cmd = DB::CCommand::Create();
	Cmd->Execute(Database, DeleteFromFrag + Name + WhereFrag + WhereSQL);
}
//---------------------------------------------------------------------

void CTable::Truncate()
{
	// SQLite3 uses truncate optimization for DELETE FROM Table without WHERE
	PCommand Cmd = DB::CCommand::Create();
	Cmd->Execute(Database, DeleteFromFrag + Name);
}
//---------------------------------------------------------------------

void CTable::CreateMultiColumnIndex(const nArray<CAttrID>& columnIds)
{
	n_assert(IsConnected());

	nString SQL;
	//SQL.Reserve(4096);
	SQL.Append("CREATE INDEX ");
	SQL.Append(GetName());
	SQL.Append("_");

	for (int i = 0; i < columnIds.GetCount(); i++)
	{
		n_assert(HasColumn(columnIds[i]));
		SQL.Append(columnIds[i]->GetName());
		if (i < columnIds.GetCount() - 1) SQL.Append("_");
	}

	SQL.Append(" ON ");
	SQL.Append(GetName());
	SQL.Append(" ( ");

	for (int i = 0; i < columnIds.GetCount(); i++)
	{
		SQL.Append("'");
		SQL.Append(columnIds[i]->GetName());
		SQL.Append("'");
		if (i < columnIds.GetCount() - 1) SQL.Append(",");
	}

	SQL.Append(" ) ");

	PCommand Cmd = CCommand::Create();
	Cmd->Execute(Database, SQL);
}
//---------------------------------------------------------------------

} // namespace DB
