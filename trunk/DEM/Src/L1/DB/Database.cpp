#include "Database.h"

#include "DBServer.h"
#include "Data/DataServer.h"
#include <sqlite3.h>

namespace Attr
{
	// Some SQLite-internal names
	DefineString(name);
	DefineString(type);
	DefineInt(cid);
	DefineInt(notnull);
	DefineBlob(dflt_value);
	DefineBool(pk);
	DefineInt(seq);
	DefineBool(unique);
	DefineInt(seqno);

	//DefineString(AttrName, ReadWrite);
	//DefineString(AttrType, ReadWrite);
	//DefineBool(AttrReadWrite, ReadWrite);
	//DefineBool(AttrDynamic, ReadWrite);
}

BEGIN_ATTRS_REGISTRATION(Database)
	// Some SQLite-internal names
	RegisterString(name, ReadOnly);
	RegisterString(type, ReadOnly);
	RegisterInt(cid, ReadOnly);
	RegisterInt(notnull, ReadOnly);
	RegisterBlob(dflt_value, ReadOnly);
	RegisterBool(pk, ReadOnly);
	RegisterInt(seq, ReadOnly);
	RegisterBool(unique, ReadOnly);
	RegisterInt(seqno, ReadOnly);
END_ATTRS_REGISTRATION

namespace DB
{
//__ImplementClass(DB::CDatabase, 'DBAS', Core::CRefCounted);
ImplementRTTI(DB::CDatabase, Core::CRefCounted);

CDatabase::CDatabase():        
	AccessMode(DBAM_ReadWriteExisting),
	CacheSize(2000),
	TmpStorageType(Memory),
	BusyTimeout(100),
	TransactionDepth(0),
	SQLiteHandle(NULL)
{
}
//---------------------------------------------------------------------

CDatabase::~CDatabase()
{
    n_assert(!IsOpen());
}
//---------------------------------------------------------------------

bool CDatabase::Open()
{
	n_assert(!IsOpen());
	n_assert(!SQLiteHandle);
	n_assert(!URI.IsEmpty());
	n_assert(!TransactionDepth);
	Flags.Set(_IsOpen);

	int SQLiteOpenFlags;
	switch (AccessMode)
	{
		case DBAM_ReadOnly:				SQLiteOpenFlags = SQLITE_OPEN_READONLY; break;
		case DBAM_ReadWriteExisting:	SQLiteOpenFlags = SQLITE_OPEN_READWRITE; break;
		case DBAM_ReadWriteCreate:		SQLiteOpenFlags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE; break;
		default:						n_error("Unknown SQLite DB access mode");
	}

	int Error;
	if (Flags.Is(InMemoryDB)) Error = sqlite3_open(":memory:", &SQLiteHandle);
	else Error = sqlite3_open_v2(URI.Get(), &SQLiteHandle, SQLiteOpenFlags, "Nebula2");

	if (Error != SQLITE_OK)
	{
		SetError(sqlite3_errmsg(SQLiteHandle));
		SQLiteHandle = NULL;
		Flags.Clear(_IsOpen);
		FAIL;
	}

	if (sqlite3_busy_timeout(SQLiteHandle, BusyTimeout) != SQLITE_OK ||
		sqlite3_extended_result_codes(SQLiteHandle, 1) != SQLITE_OK)
	{
		SetError(sqlite3_errmsg(SQLiteHandle));
		Close();
		FAIL;
	}

	nString SQL;
	PCommand Cmd = CCommand::Create();
	
	Cmd->Execute(this, "PRAGMA journal_mode=MEMORY");

	if (IsExclusiveMode())
		Cmd->Execute(this, "PRAGMA locking_mode=EXCLUSIVE");

	SQL.Format("PRAGMA cache_size=%d", CacheSize);
	Cmd->Execute(this, SQL);

	SQL = IsSyncMode() ? "PRAGMA synchronous=ON" : "PRAGMA synchronous=OFF";
	Cmd->Execute(this, SQL);

	SQL = (Memory == TmpStorageType) ? "PRAGMA temp_store=MEMORY" : "PRAGMA temp_store=FILE";
	Cmd->Execute(this, SQL);

	BeginTransactionCmd = CCommand::Create();
	BeginTransactionCmd->Compile(this, "BEGIN DEFERRED");
	EndTransactionCmd = CCommand::Create();
	EndTransactionCmd->Compile(this, "COMMIT");

	// For in-memory database we have to copy all tables from db on disk
	if (Flags.Is(InMemoryDB))
	{
		AttachDatabase(URI, "fileDB");

		Cmd = CCommand::Create();
		PValueTable Result = CValueTable::Create();
		Cmd->Execute(this, "SELECT name FROM fileDB.sqlite_master WHERE type='Table'", Result);

		for (int i = 0; i < Result->GetRowCount(); i++)
		{
			nString SQL("CREATE TABLE ");
			nString TblName(Result->Get<nString>(Attr::name, i));                
			SQL.Append(TblName);
			SQL.Append(" AS SELECT * FROM fileDB.");
			SQL.Append(TblName);
			sqlite3_exec(SQLiteHandle, SQL.Get(), NULL, NULL, NULL);        
		}
		DetachDatabase("fileDB");
	}

	ReadTableLayouts();

	OK;
}
//---------------------------------------------------------------------

void CDatabase::Close()
{
	n_assert(IsOpen());
	n_assert(SQLiteHandle);
	n_assert(!TransactionDepth);

	for (int i = 0; i < Tables.Size(); i++)
		Tables[i]->Disconnect(false);
	Tables.Clear();

	BeginTransactionCmd = NULL;
	EndTransactionCmd = NULL;

	if (sqlite3_close(SQLiteHandle) != SQLITE_OK)
	{
		SetError(sqlite3_errmsg(SQLiteHandle));
		n_error("CDatabase::Close(): Failed to close db '%s' with ErrorStr '%s'\n", 
			URI.Get(), ErrorStr.Get());
	}
	SQLiteHandle = NULL;
	Flags.Clear(_IsOpen);
}
//---------------------------------------------------------------------

void CDatabase::BeginTransaction()
{
	if (++TransactionDepth == 1) n_assert(BeginTransactionCmd->Execute(this));
}
//---------------------------------------------------------------------

void CDatabase::EndTransaction()
{
	n_assert(TransactionDepth);
	if (--TransactionDepth == 0) n_assert(EndTransactionCmd->Execute(this));
}   
//---------------------------------------------------------------------

void CDatabase::ReadTableLayouts()
{
	n_assert(GetNumTables() == 0);

	// query existing Tables
	PCommand Cmd = CCommand::Create();
	PValueTable Result = CValueTable::Create();
	Cmd->Execute(this, "SELECT name FROM sqlite_master WHERE type='table'", Result);

	// first check if there is an _Attributes Table, if it exists,
	// register all attributes in that Table
	nArray<int> attrTableRowIndices = Result->FindRowIndicesByAttr(Attr::name, nString("_Attributes"), true);
	/*
	if (!attrTableRowIndices.Empty())
	{
		// connect the attributes Table as usual...
		Ptr<CTable> attrsTable = CTable::Create();
		attrsTable->SetName(Result->Get<nString>(Attr::name, attrTableRowIndices[0]));
		Tables.Append(attrsTable.get());
		attrsTable->Connect(this, Table::AssumeExists, ignoreUnknownColumns);

		// register all attributes in the Table...
		PTable tempTablePtr = attrsTable;
		RegisterAttributes(tempTablePtr);
	}
	*/

	// Create a Table object and connect it to the database,
	// the Table will read the layout when it is connected
	for (int RowIdx = 0; RowIdx < Result->GetRowCount(); RowIdx++)
		if (attrTableRowIndices.Empty() || RowIdx != attrTableRowIndices[0])
		{
			PTable Table = CTable::Create();
			Table->SetName(Result->Get<nString>(Attr::name, RowIdx));
			Tables.Append(Table.get());
			Table->Connect(this, CTable::AssumeExists, DoesIgnoreUnknownColumns());
		}
}
//---------------------------------------------------------------------

void CDatabase::AddTable(const PTable& Table)
{
	n_assert(IsOpen());
	n_assert(Table.isvalid());
	n_assert(!HasTable(Table->GetName()));
	Tables.Append(Table);
	Table->Connect(this, CTable::ForceCreate);
}
//---------------------------------------------------------------------

void CDatabase::DeleteTable(const nString& TableName)
{
	int Idx = FindTableIndex(TableName);
	n_assert(INVALID_INDEX != Idx);
	Tables[Idx]->Disconnect(true);
	Tables.Erase(Idx);
}
//---------------------------------------------------------------------

// NOTE: we cannot use a more efficient container for the Tables because the
// Table name can change anytime outside our control.
int CDatabase::FindTableIndex(const nString& TableName) const
{
	n_assert(TableName.IsValid());
	n_assert(IsOpen());
	for (int i = 0; i < Tables.Size(); i++)
		if (Tables[i]->GetName() == TableName) return i;
	return INVALID_INDEX;
}
//---------------------------------------------------------------------

bool CDatabase::AttachDatabase(const nString& URI, const nString& DBName)
{
	n_assert(IsOpen());
	n_assert(DBName.IsValid());

	if (DataSrv->FileExists(URI))
	{
		nString SQL;
		SQL.Format("ATTACH DATABASE 'file:%s?vfs=Nebula2' AS '%s'", URI.Get(), DBName.Get());
		PCommand Cmd = CCommand::Create();
		if (!Cmd->Execute(this, SQL))
		{
			n_error("CDatabase::AttachDatabase(%s, %s) failed with sqlite ErrorStr: %s",
				URI.Get(), DBName.Get(), Cmd->GetError().Get());
			FAIL;
		}
		OK;
	}
	else n_error("CDatabase::AttachDatabase: file '%s' doesn't exist!", URI.Get());

	FAIL;
}
//---------------------------------------------------------------------

void CDatabase::DetachDatabase(const nString& DBName)
{
	n_assert(IsOpen());
	n_assert(DBName.IsValid());
	nString SQL;
	SQL.Format("DETACH DATABASE '%s'", DBName.Get());
	PCommand Cmd = CCommand::Create();
	if (!Cmd->Execute(this, SQL))
		n_error("CDatabase::AttachDatabase(%s, %s) failed with sqlite ErrorStr: %s",
			URI.Get(), DBName.Get(), Cmd->GetError().Get());
}
//---------------------------------------------------------------------

void CDatabase::CopyInMemoryDatabaseToFile(const nString& FileURI)
{
	AttachDatabase(FileURI, "fileDB");

	PCommand Cmd = CCommand::Create();
	PValueTable Result = CValueTable::Create();
	Cmd->Execute(this, "SELECT name FROM sqlite_master WHERE type='Table'", Result);

	for (int i = 0; i < Result->GetRowCount(); i++)
	{
		nString SQL("CREATE TABLE fileDB.");
		SQL.Append(Result->Get<nString>(Attr::name, i));
		SQL.Append(" AS SELECT * FROM ");
		SQL.Append(Result->Get<nString>(Attr::name, i));
		sqlite3_exec(SQLiteHandle, SQL.Get(), NULL, NULL, NULL);
	}
	DetachDatabase("fileDB");
}
//---------------------------------------------------------------------

/**
    Registers additional attributes defined in the database's special
    _Attributes Table. This enables an application to function even
    without knowing all attributes beforehand.

    FIXME: we could also check here if the attributes in the database
    are type-compatible with the already existing attributes(?).
*/
/*
void CDatabase::RegisterAttributes(PTable& attrTable)
{
    n_assert(0 != attrTable);

    // create a dataset and read Table contents
    PDataset dataset = attrTable->CreateDataset();
    dataset->AddColumnsFromTable();
    dataset->PerformQuery();
    PValueTable values = dataset->Values();
    int RowIdx;
    int numRows = values->GetRowCount();
    for (RowIdx = 0; RowIdx < numRows; RowIdx++)
    {
        const nString& attrName = values->Get<nString>(Attr::AttrName, RowIdx);        
        if (!DBSrv->IsValidAttrName(attrName))
        {
            nString valueString = values->Get<nString>(Attr::AttrType, RowIdx);
        //#if NEBULA3_DATABASE_LEGACY_VECTORS
            // nebula2 export support
            if (valueString == "vector3" || valueString == "vector4")
            {
                valueString = "float4";
            }
        //#endif
            const CType* valueType = CAttr::StringToValueType(valueString);
            bool readWrite = values->GetBool(Attr::AttrReadWrite, RowIdx);
            AccessMode accessMode = readWrite ? Attr::ReadWrite : Attr::ReadOnly;
            Attr::AttributeDefinitionBase::RegisterDynamicAttribute(attrName, Util::FourCC(), valueType, accessMode);
        }
    }
}
*/

} // namespace DB
