#include <ShaderDB.h>
#include <ConsoleApp.h>
#include <ValueTable.h>
#include <Data/Params.h>
#include <Data/StringUtils.h>
#include <IO/IOServer.h>
#include <sqlite3.h>

#define INIT_SQL(Stmt, SQL) \
	if (sqlite3_prepare_v2(SQLiteHandle, SQL, -1, &Stmt, NULL) != SQLITE_OK) \
	{ \
		Sys::Log("Error compiling SQL: %s\n", SQL); \
		CloseDB(); \
		FAIL; \
	}
#define SAFE_RELEASE_SQL(Stmt) if (Stmt) { sqlite3_finalize(Stmt); Stmt = NULL; }

sqlite3*		SQLiteHandle = NULL;
sqlite3_stmt*	SQLFindShader = NULL;
sqlite3_stmt*	SQLGetObjFile = NULL;
sqlite3_stmt*	SQLFindObjFileUsage = NULL;
sqlite3_stmt*	SQLFindObjFile = NULL;
sqlite3_stmt*	SQLFindFreeObjFileRec = NULL;
sqlite3_stmt*	SQLInsertNewObjFileRec = NULL;
sqlite3_stmt*	SQLUpdateObjFileRec = NULL;
sqlite3_stmt*	SQLReleaseObjFileRec = NULL;
sqlite3_stmt*	SQLInsertNewShaderRec = NULL;
sqlite3_stmt*	SQLUpdateShaderRec = NULL;
sqlite3_stmt*	SQLClearDefines = NULL;
sqlite3_stmt*	SQLInsertDefine = NULL;
sqlite3_stmt*	SQLGetDefines = NULL;

bool BindQueryParams(sqlite3_stmt* SQLiteStmt, const Data::CParams& Params)
{
	if (!SQLiteStmt || sqlite3_reset(SQLiteStmt) != SQLITE_OK) FAIL;

	CString ParamName(NULL, 0, 64);
	for (int i = 0; i < Params.GetCount(); ++i)
	{
		const Data::CParam& Prm = Params.Get(i);

		ParamName = ":";
		ParamName += Prm.GetName().CStr();
		int Idx = sqlite3_bind_parameter_index(SQLiteStmt, ParamName.CStr());
		if (Idx < 1) continue;

		const Data::CData& Val = Prm.GetRawValue();
		int Error = SQLITE_OK;
		if (Val.IsVoid()) ; //equal to "Error = sqlite3_bind_null(SQLiteStmt, Idx);"
		else if (Val.IsA<int>()) Error = sqlite3_bind_int(SQLiteStmt, Idx, Val);
		else if (Val.IsA<float>()) Error = sqlite3_bind_double(SQLiteStmt, Idx, (double)Val.GetValue<float>());
		else if (Val.IsA<bool>()) Error = sqlite3_bind_int(SQLiteStmt, Idx, (bool)Val ? 1 : 0);
		else if (Val.IsA<CString>())
		{
			// NOTE: the string should be in UTF-8 format.
			Error = sqlite3_bind_text(SQLiteStmt, Idx, Val.GetValuePtr<CString>()->CStr(),
				-1, SQLITE_TRANSIENT);
		}
		else if (Val.IsA<CStrID>())
		{
			// NOTE: the string should be in UTF-8 format.
			Error = sqlite3_bind_text(SQLiteStmt, Idx, Val.GetValue<CStrID>().CStr(),
				-1, SQLITE_TRANSIENT);
		}
		else if (Val.IsA<Data::CBuffer>())
		{
			const Data::CBuffer* Blob = Val.GetValuePtr<Data::CBuffer>();
			if (Blob->IsValid())
				Error = sqlite3_bind_blob(SQLiteStmt, Idx, Blob->GetPtr(), Blob->GetSize(), SQLITE_TRANSIENT);
		}
		else
		{
			n_msg(VL_ERROR, "BindQueryParams() > invalid parameter type!");
			FAIL;
		}

		if (Error != SQLITE_OK) FAIL;
	}

	OK;
}
//---------------------------------------------------------------------

bool ExecuteStatement(sqlite3_stmt* SQLiteStmt, DB::CValueTable* pOutTable = NULL, const Data::CParams* pParams = NULL)
{
	if (!SQLiteStmt) FAIL;

	if (pParams && !BindQueryParams(SQLiteStmt, *pParams)) FAIL;

	if (pOutTable)
	{
		pOutTable->TrackModifications(false);
			
		int ColCount = sqlite3_column_count(SQLiteStmt);
		if (ColCount > 0)
		{
			pOutTable->BeginAddColumns();
			for (int ResultColIdx = 0; ResultColIdx < ColCount; ++ResultColIdx)
			{
				CStrID ColID = CStrID(sqlite3_column_name(SQLiteStmt, ResultColIdx));
				if (!pOutTable->HasColumn(ColID))
					pOutTable->AddColumn(ColID, NULL, false);
			}
			pOutTable->EndAddColumns();
		}
	}
		
	int Result;
	do
	{
		Result = sqlite3_step(SQLiteStmt);
		switch (Result)
		{
			case SQLITE_BUSY:	Sys::Sleep(1); break;
			case SQLITE_DONE:	break;
			case SQLITE_ROW:
			{
				if (!pOutTable) break;

				int RowIdx = pOutTable->AddRow();
				int ResultColIdx;
				const int ColCount = sqlite3_data_count(SQLiteStmt);
				for (ResultColIdx = 0; ResultColIdx < ColCount; ++ResultColIdx)
				{
					int ResultColType = sqlite3_column_type(SQLiteStmt, ResultColIdx);
					if (SQLITE_NULL == ResultColType) continue;

					int ColIdx = pOutTable->GetColumnIndex(CStrID(sqlite3_column_name(SQLiteStmt, ResultColIdx)));
					if (ColIdx < 0) continue;

					const Data::CType* Type = pOutTable->GetColumnValueType(ColIdx);
					if (Type == TInt)
					{
						n_assert(SQLITE_INTEGER == ResultColType);
						int Val = sqlite3_column_int(SQLiteStmt, ResultColIdx);
						pOutTable->Set<int>(ColIdx, RowIdx, Val);
					}
					else if (Type == TFloat)
					{
						n_assert(SQLITE_FLOAT == ResultColType);
						float Val = (float)sqlite3_column_double(SQLiteStmt, ResultColIdx);                        
						pOutTable->Set<float>(ColIdx, RowIdx, Val);
					}
					else if (Type == TBool)
					{
						n_assert(SQLITE_INTEGER == ResultColType);
						int Val = sqlite3_column_int(SQLiteStmt, ResultColIdx);
						pOutTable->Set<bool>(ColIdx, RowIdx, (Val == 1));
					}
					else if (Type == TString)
					{
						n_assert(SQLITE_TEXT == ResultColType);
						CString Val((const char*)sqlite3_column_text(SQLiteStmt, ResultColIdx));
						pOutTable->Set<CString>(ColIdx, RowIdx, Val);
					}
					else if (Type == TStrID)
					{
						n_assert(SQLITE_TEXT == ResultColType);
						CStrID Val((LPCSTR)sqlite3_column_text(SQLiteStmt, ResultColIdx));
						pOutTable->Set<CStrID>(ColIdx, RowIdx, Val);
					}
					else if (Type == TBuffer)
					{
						n_assert(SQLITE_BLOB == ResultColType);
						const void* ptr = sqlite3_column_blob(SQLiteStmt, ResultColIdx);
						int size = sqlite3_column_bytes(SQLiteStmt, ResultColIdx);
						Data::CBuffer Blob(ptr, size);
						pOutTable->Set<Data::CBuffer>(ColIdx, RowIdx, Blob);
					}
					else if (!Type)
					{
						// Variable type column, it supports int, float & string. Bool is represented as int.
						// If ScriptObject wants to save bool to DB & then restore it, this object should
						// implement OnLoad scripted method or smth and convert int values to bool inside.
						switch (ResultColType)
						{
							case SQLITE_INTEGER:
								pOutTable->Set<int>(ColIdx, RowIdx, sqlite3_column_int(SQLiteStmt, ResultColIdx));
								break;
							case SQLITE_FLOAT:
								pOutTable->Set<float>(ColIdx, RowIdx, (float)sqlite3_column_double(SQLiteStmt, ResultColIdx));
								break;
							case SQLITE_TEXT:
								pOutTable->Set<CString>(ColIdx, RowIdx, CString((LPCSTR)sqlite3_column_text(SQLiteStmt, ResultColIdx)));
								break;
							default: Sys::Error("ExecuteStatement() > unsupported variable-type DB column type!");
						}
					}
					else Sys::Error("ExecuteStatement() > unsupported type!");
				}

				break;
			}
			case SQLITE_ERROR:
			{
				n_msg(VL_ERROR, "SQLite error during sqlite3_step(): %s\n", sqlite3_errmsg(SQLiteHandle));
				FAIL;
			}
			default:
			{
				n_msg(VL_ERROR, "sqlite3_step() returned error code %d, %s\n", Result, sqlite3_errmsg(SQLiteHandle));
				FAIL;
			}
		}
	}
	while (Result != SQLITE_DONE);

	if (pOutTable) pOutTable->TrackModifications(true);

	OK;
}
//---------------------------------------------------------------------

bool ExecuteSQLQuery(const char* pSQL, DB::CValueTable* pOutTable = NULL, const Data::CParams* pParams = NULL)
{
	if (!pSQL) OK;

	bool Result = true;
	do
	{
		sqlite3_stmt* SQLiteStmt = NULL;
		if (sqlite3_prepare_v2(SQLiteHandle, pSQL, -1, &SQLiteStmt, &pSQL) != SQLITE_OK)
		{
			n_msg(VL_ERROR, "SQLite error: %s\n", sqlite3_errmsg(SQLiteHandle));
			FAIL;
		}

		while (*pSQL && strchr(DEM_WHITESPACE, *pSQL)) ++pSQL;

		// In the last query fill table
		Result = ExecuteStatement(SQLiteStmt, (!*pSQL && pOutTable) ? pOutTable : NULL, pParams);

		sqlite3_finalize(SQLiteStmt);
	}
	while (Result && *pSQL);

	return Result;
}
//---------------------------------------------------------------------

// NB: pURI must be encoded in UTF-8
bool OpenDB(const char* pURI)
{
	n_assert(!SQLiteHandle);

	int Result = sqlite3_open_v2(pURI, &SQLiteHandle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL); //"Nebula2");
	if (Result != SQLITE_OK)
	{
		n_msg(VL_ERROR, "SQLite error: %s\n", sqlite3_errmsg(SQLiteHandle));
		SQLiteHandle = NULL;
		FAIL;
	}

	if (sqlite3_busy_timeout(SQLiteHandle, 100) != SQLITE_OK ||
		sqlite3_extended_result_codes(SQLiteHandle, 1) != SQLITE_OK)
	{
		n_msg(VL_ERROR, "SQLite error: %s\n", sqlite3_errmsg(SQLiteHandle));
		CloseDB();
		FAIL;
	}

	//???synchronous mode?
	const char* pSQL = "\
PRAGMA journal_mode=MEMORY;\
PRAGMA locking_mode=EXCLUSIVE;\
PRAGMA cache_size=2048;\
PRAGMA synchronous=ON;\
PRAGMA temp_store=MEMORY";
	if (!ExecuteSQLQuery(pSQL))
	{
		CloseDB();
		FAIL;
	}

	// Query tables and create them if DB is empty
	DB::CValueTable Tables;
	if (!ExecuteSQLQuery("SELECT name FROM sqlite_master WHERE type='table'", &Tables))
	{
		CloseDB();
		FAIL;
	}

	if (!Tables.GetRowCount())
	{
		const char* pCreateDBSQL = "\
CREATE TABLE 'Files' (\
	'ID' INTEGER,\
	'Path' VARCHAR(1024) NOT NULL,\
	'Size' INTEGER,\
	'CRC' INTEGER,\
	PRIMARY KEY (ID ASC) ON CONFLICT REPLACE);\
\
CREATE TABLE 'Shaders' (\
	'ID' INTEGER,\
	'ShaderType' INTEGER,\
	'Target' INTEGER,\
	'EntryPoint' VARCHAR(64),\
	'CompilerFlags' INTEGER,\
	'SrcPath' VARCHAR(1024) NOT NULL,\
	'SrcModifyTimestamp' INTEGER,\
	'SrcSize' INTEGER,\
	'SrcCRC' INTEGER,\
	'ObjFileID' INTEGER,\
	'InputSigFileID' INTEGER,\
	PRIMARY KEY (ID ASC) ON CONFLICT REPLACE,\
	FOREIGN KEY (ObjFileID) REFERENCES Files(ID));\
\
CREATE TABLE 'Macros' (\
	'ShaderID' INTEGER,\
	'Name' VARCHAR(64),\
	'Value' VARCHAR(1024),\
	PRIMARY KEY (ShaderID, Name) ON CONFLICT REPLACE,\
	FOREIGN KEY (ShaderID) REFERENCES Shaders(ID));\
\
CREATE INDEX Files_MainIndex ON Files (Size, CRC);\
\
CREATE INDEX Shaders_MainIndex ON Shaders (SrcPath, ShaderType, Target, EntryPoint)";
		if (!ExecuteSQLQuery(pCreateDBSQL))
		{
			CloseDB();
			FAIL;
		}
	}

	INIT_SQL(SQLFindShader, "SELECT * FROM Shaders WHERE SrcPath=:Path AND ShaderType=:Type AND Target=:Target AND EntryPoint=:Entry");
	INIT_SQL(SQLGetObjFile, "SELECT * FROM Files WHERE ID=:ID");
	INIT_SQL(SQLFindObjFileUsage, "SELECT ID FROM Shaders WHERE ObjFileID=:ID OR InputSigFileID=:ID");
	INIT_SQL(SQLFindObjFile, "SELECT ID, Path FROM Files WHERE Size=:Size AND CRC=:CRC");
	INIT_SQL(SQLFindFreeObjFileRec, "SELECT ID FROM Files WHERE Size = 0 ORDER BY ID LIMIT 1");
	INIT_SQL(SQLInsertNewObjFileRec, "INSERT INTO Files (Path, Size, CRC) VALUES (\"\", 0, 0)");
	INIT_SQL(SQLUpdateObjFileRec, "UPDATE Files SET Path=:Path, Size=:Size, CRC=:CRC WHERE ID=:ID");
	INIT_SQL(SQLReleaseObjFileRec, "UPDATE Files SET Size=0 WHERE ID=:ID");
	INIT_SQL(SQLInsertNewShaderRec, "INSERT INTO Shaders (SrcPath) VALUES (\"\")");
	INIT_SQL(SQLUpdateShaderRec,
			 "UPDATE Shaders SET "
			 "	ShaderType=:ShaderType, "
			 "	Target=:Target, "
			 "	EntryPoint=:EntryPoint, "
			 "	CompilerFlags=:CompilerFlags, "
			 "	SrcPath=:SrcPath, "
			 "	SrcModifyTimestamp=:SrcModifyTimestamp, "
			 "	SrcSize=:SrcSize, "
			 "	SrcCRC=:SrcCRC, "
			 "	ObjFileID=:ObjFileID, "
			 "	InputSigFileID=:InputSigFileID "
			 "WHERE ID=:ID");
	INIT_SQL(SQLClearDefines, "DELETE FROM Macros WHERE ShaderID = :ShaderID");
	INIT_SQL(SQLInsertDefine, "INSERT INTO Macros (ShaderID, Name, Value) VALUES (:ShaderID, :Name, :Value)");
	INIT_SQL(SQLGetDefines, "SELECT Name, Value FROM Macros WHERE ShaderID = :ShaderID"); // ORDER BY Name");

	OK;
}
//---------------------------------------------------------------------

void CloseDB()
{
	SAFE_RELEASE_SQL(SQLFindShader);
	SAFE_RELEASE_SQL(SQLGetObjFile);
	SAFE_RELEASE_SQL(SQLFindObjFileUsage);
	SAFE_RELEASE_SQL(SQLFindObjFile);
	SAFE_RELEASE_SQL(SQLFindFreeObjFileRec);
	SAFE_RELEASE_SQL(SQLInsertNewObjFileRec);
	SAFE_RELEASE_SQL(SQLUpdateObjFileRec);
	SAFE_RELEASE_SQL(SQLReleaseObjFileRec);
	SAFE_RELEASE_SQL(SQLInsertNewShaderRec);
	SAFE_RELEASE_SQL(SQLUpdateShaderRec);
	SAFE_RELEASE_SQL(SQLClearDefines);
	SAFE_RELEASE_SQL(SQLInsertDefine);
	SAFE_RELEASE_SQL(SQLGetDefines);

	if (SQLiteHandle)
	{
		if (sqlite3_close(SQLiteHandle) != SQLITE_OK)
		{
			n_msg(VL_ERROR, "SQLite error: %s\n", sqlite3_errmsg(SQLiteHandle));
		}
		SQLiteHandle = NULL;
	}
}
//---------------------------------------------------------------------

bool FindShaderRec(CShaderDBRec& InOut)
{
	Data::CParams Params(4);
	Params.Set(CStrID("Path"), InOut.SrcFile.Path);
	Params.Set(CStrID("Type"), (int)InOut.ShaderType);
	Params.Set(CStrID("Target"), (int)InOut.Target);
	Params.Set(CStrID("Entry"), InOut.EntryPoint);

	DB::CValueTable Shaders;
	if (!ExecuteStatement(SQLFindShader, &Shaders, &Params)) FAIL;

	DB::CValueTable Defines;
	Data::CParams DefineParams(1);

	bool Found = false;

	int Col_ID = Shaders.GetColumnIndex(CStrID("ID"));
	for (int ShIdx = 0; ShIdx < Shaders.GetRowCount(); ++ShIdx)
	{
		DefineParams.Set(CStrID("ShaderID"), Shaders.Get<int>(Col_ID, ShIdx));
		if (!ExecuteStatement(SQLGetDefines, &Defines, &DefineParams)) FAIL;

		// Local defines store terminating { NULL, NULL }, so sub 1 from its count
		DWORD DBDefineCount = Defines.GetRowCount();
		DWORD LocalDefineCount = InOut.Defines.GetCount() ? InOut.Defines.GetCount() - 1 : 0;
		if (DBDefineCount != LocalDefineCount)
		{
			Found = false;
			break;
		}

		Found = true;

		int Col_Name = Defines.GetColumnIndex(CStrID("Name"));
		int Col_Value = Defines.GetColumnIndex(CStrID("Value"));
		for (int DefIdx = 0; DefIdx < Defines.GetRowCount(); ++DefIdx)
		{
			const char* pName = Defines.Get<CString>(Col_Name, DefIdx).CStr();
			n_assert(pName && *pName);

			for (int NewDefIdx = 0; NewDefIdx < InOut.Defines.GetCount(); ++NewDefIdx)
			{
				CMacroDBRec& Macro = InOut.Defines[NewDefIdx];
				if (!strcmp(Macro.Name, pName))
				{
					Data::CData Value;
					Defines.GetValue(Col_Value, DefIdx, Value);
					if ((!Macro.Value) != Value.IsNull() && strcmp(Macro.Value, Value.GetValue<CString>().CStr()))
					{
						Found = false;
					}
					break;
				}
			}

			if (!Found) break;
		}

		if (Found)
		{
			InOut.ID = Shaders.Get<int>(Col_ID, ShIdx);
			InOut.CompilerFlags = Shaders.Get<int>(CStrID("CompilerFlags"), ShIdx);
			InOut.SrcModifyTimestamp = Shaders.Get<int>(CStrID("SrcModifyTimestamp"), ShIdx);
			InOut.SrcFile.Size = Shaders.Get<int>(CStrID("SrcSize"), ShIdx);
			InOut.SrcFile.CRC = Shaders.Get<int>(CStrID("SrcCRC"), ShIdx);
			OK;
		}
	}

	InOut.ID = 0;
	InOut.CompilerFlags = 0;
	InOut.ObjFile.ID = 0;
	InOut.ObjFile.Path = CString::Empty;
	InOut.InputSigFile.ID = 0;
	InOut.InputSigFile.Path = CString::Empty;

	FAIL; // Not found
}
//---------------------------------------------------------------------

bool WriteShaderRec(CShaderDBRec& InOut)
{
	DWORD ID;
	if (InOut.ID == 0)
	{
		if (!ExecuteStatement(SQLInsertNewShaderRec)) FAIL;
		ID = (DWORD)sqlite3_last_insert_rowid(SQLiteHandle);
	}
	else ID = InOut.ID;

	Data::CParams Params(11);
	Params.Set(CStrID("ID"), (int)ID);
	Params.Set(CStrID("ShaderType"), (int)InOut.ShaderType);
	Params.Set(CStrID("Target"), (int)InOut.Target);
	Params.Set(CStrID("EntryPoint"), InOut.EntryPoint);
	Params.Set(CStrID("CompilerFlags"), (int)InOut.CompilerFlags);
	Params.Set(CStrID("SrcPath"), InOut.SrcFile.Path);
	Params.Set(CStrID("SrcModifyTimestamp"), (int)InOut.SrcModifyTimestamp);
	Params.Set(CStrID("SrcSize"), (int)InOut.SrcFile.Size);
	Params.Set(CStrID("SrcCRC"), (int)InOut.SrcFile.CRC);
	Params.Set(CStrID("ObjFileID"), (int)InOut.ObjFile.ID);
	Params.Set(CStrID("InputSigFileID"), (int)InOut.InputSigFile.ID);

	if (!ExecuteStatement(SQLUpdateShaderRec, NULL, &Params)) FAIL;

	InOut.ID = ID;

	Params.Clear();
	Params.Set(CStrID("ShaderID"), (int)InOut.ID);
	if (!ExecuteStatement(SQLClearDefines, NULL, &Params)) FAIL;

	for (int i = 0; i < InOut.Defines.GetCount(); ++i)
	{
		CMacroDBRec& Macro = InOut.Defines[i];
		if (!Macro.Name || !*Macro.Name) continue;
		Params.Set(CStrID("Name"), CString(Macro.Name));
		Params.Set(CStrID("Value"), CString(Macro.Value));
		if (!ExecuteStatement(SQLInsertDefine, NULL, &Params)) FAIL;
	}

	OK;
}
//---------------------------------------------------------------------

bool FindObjFile(CFileData& InOut, const void* pBinaryData)
{
	if (InOut.Size)
	{
		Data::CParams Params(2);
		Params.Set(CStrID("Size"), (int)InOut.Size);
		Params.Set(CStrID("CRC"), (int)InOut.CRC);

		DB::CValueTable Result;
		if (!ExecuteStatement(SQLFindObjFile, &Result, &Params)) FAIL;

		Data::CBuffer Buf;
		int Col_ID = Result.GetColumnIndex(CStrID("ID"));
		int Col_Path = Result.GetColumnIndex(CStrID("Path"));
		for (int i = 0; i < Result.GetRowCount(); ++i)
		{
			CString Path = Result.Get<CString>(Col_Path, i);
			if (!IOSrv->LoadFileToBuffer(Path, Buf) || Buf.GetSize() != InOut.Size) continue;
			if (!memcmp(pBinaryData, Buf.GetPtr(), Buf.GetSize()))
			{
				InOut.ID = Result.Get<int>(Col_ID, i);
				InOut.Path = Path;
				OK; // Found
			}
		}
	}

	InOut.ID = 0;
	InOut.Path = CString::Empty;

	FAIL; // Not found
}
//---------------------------------------------------------------------

bool RegisterObjFile(CFileData& InOut, const char* Extension)
{
	DWORD ID;
	if (InOut.ID == 0)
	{
		DB::CValueTable Result;
		if (!ExecuteStatement(SQLFindFreeObjFileRec, &Result)) FAIL;

		if (Result.GetRowCount())
		{
			ID = Result.Get<int>(CStrID("ID"), 0);
		}
		else
		{
			if (!ExecuteStatement(SQLInsertNewObjFileRec)) FAIL;
			ID = (DWORD)sqlite3_last_insert_rowid(SQLiteHandle);
		}
	}
	else ID = InOut.ID;

	if (InOut.Path.IsEmpty())
	{
		InOut.Path = "Shaders:Bin/" + StringUtils::FromInt(ID) + "." + Extension;
	}

	Data::CParams Params(4);
	Params.Set(CStrID("Path"), InOut.Path);
	Params.Set(CStrID("Size"), (int)InOut.Size);
	Params.Set(CStrID("CRC"), (int)InOut.CRC);
	Params.Set(CStrID("ID"), (int)ID);

	if (!ExecuteStatement(SQLUpdateObjFileRec, NULL, &Params)) FAIL;

	InOut.ID = ID;

	OK;
}
//---------------------------------------------------------------------

bool ReleaseObjFile(DWORD ID, CString& OutPath)
{
	if (ID == 0) FAIL;

	DB::CValueTable Result;
	if (!ExecuteStatement(SQLFindObjFileUsage, &Result)) FAIL;

	// References found, don't delete file
	if (Result.GetRowCount()) FAIL;

	Data::CParams Params(1);
	Params.Set(CStrID("ID"), (int)ID);

	// Get file path to delete
	Result.Clear();
	if (!ExecuteStatement(SQLGetObjFile, &Result, &Params)) FAIL;
	OutPath = Result.Get<CString>(CStrID("Path"), 0);

	return ExecuteStatement(SQLReleaseObjFileRec, NULL, &Params);
}
//---------------------------------------------------------------------
