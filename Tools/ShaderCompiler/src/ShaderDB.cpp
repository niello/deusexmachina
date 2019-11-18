#include "ShaderDB.h"

#include <ValueTable.h>
#include <Utils.h>
#include <sqlite3.h>
#include <thread>
#include <chrono>
#include <cassert>
#include <filesystem>

namespace fs = std::filesystem;

namespace DB
{

static sqlite3* SQLiteHandle = nullptr;

bool BindQueryParams(sqlite3_stmt* SQLiteStmt, const Data::CParams& Params)
{
	if (!SQLiteStmt || sqlite3_reset(SQLiteStmt) != SQLITE_OK) return false;

	std::string ParamName;
	ParamName.reserve(64);
	for (const auto& Pair : Params)
	{
		ParamName = ":";
		ParamName += Pair.first.CStr();
		int Idx = sqlite3_bind_parameter_index(SQLiteStmt, ParamName.c_str());
		if (Idx < 1) continue;

		const Data::CData& Val = Pair.second;
		int Error = SQLITE_OK;
		if (Val.IsVoid()); //equal to "Error = sqlite3_bind_null(SQLiteStmt, Idx);"
		else if (Val.IsA<int>()) Error = sqlite3_bind_int(SQLiteStmt, Idx, Val);
		else if (Val.IsA<float>()) Error = sqlite3_bind_double(SQLiteStmt, Idx, (double)Val.GetValue<float>());
		else if (Val.IsA<bool>()) Error = sqlite3_bind_int(SQLiteStmt, Idx, (bool)Val ? 1 : 0);
		else if (Val.IsA<std::string>())
		{
			// NOTE: the string should be in UTF-8 format.
			Error = sqlite3_bind_text(SQLiteStmt, Idx, Val.GetValuePtr<std::string>()->c_str(),
				-1, SQLITE_TRANSIENT);
		}
		else if (Val.IsA<CStrID>())
		{
			// NOTE: the string should be in UTF-8 format.
			Error = sqlite3_bind_text(SQLiteStmt, Idx, Val.GetValue<CStrID>().CStr(),
				-1, SQLITE_TRANSIENT);
		}
		else if (Val.IsA<CBuffer>())
		{
			const CBuffer* Blob = Val.GetValuePtr<CBuffer>();
			if (Blob && !Blob->empty())
				Error = sqlite3_bind_blob(SQLiteStmt, Idx, Blob->data(), Blob->size(), SQLITE_TRANSIENT);
		}
		else
		{
			//Messages += "BindQueryParams() > invalid parameter type!\n";
			return false;
		}

		if (Error != SQLITE_OK) return false;
	}

	return true;
}
//---------------------------------------------------------------------

bool InitSQL(sqlite3_stmt** ppStmt, const char* pSQL)
{
	if (sqlite3_prepare_v2(SQLiteHandle, pSQL, -1, ppStmt, nullptr) == SQLITE_OK)
		return true;

	CloseConnection();
	assert(false && "Error compiling SQL");
	return false;
}
//---------------------------------------------------------------------

bool ExecuteStatement(sqlite3_stmt* SQLiteStmt, CValueTable* pOutTable = nullptr, const Data::CParams* pParams = nullptr)
{
	if (!SQLiteStmt) return false;

	if (pParams && !BindQueryParams(SQLiteStmt, *pParams)) return false;

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
					pOutTable->AddColumn(ColID, nullptr, false);
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
			case SQLITE_BUSY:	std::this_thread::sleep_for(std::chrono::seconds(1)); break;
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
					if (Type == DATA_TYPE(int))
					{
						assert(SQLITE_INTEGER == ResultColType);
						int Val = sqlite3_column_int(SQLiteStmt, ResultColIdx);
						pOutTable->Set<int>(ColIdx, RowIdx, Val);
					}
					else if (Type == DATA_TYPE(float))
					{
						assert(SQLITE_FLOAT == ResultColType);
						float Val = (float)sqlite3_column_double(SQLiteStmt, ResultColIdx);
						pOutTable->Set<float>(ColIdx, RowIdx, Val);
					}
					else if (Type == DATA_TYPE(bool))
					{
						assert(SQLITE_INTEGER == ResultColType);
						int Val = sqlite3_column_int(SQLiteStmt, ResultColIdx);
						pOutTable->Set<bool>(ColIdx, RowIdx, (Val == 1));
					}
					else if (Type == DATA_TYPE(std::string))
					{
						assert(SQLITE_TEXT == ResultColType);
						std::string Val((const char*)sqlite3_column_text(SQLiteStmt, ResultColIdx));
						pOutTable->Set<std::string>(ColIdx, RowIdx, Val);
					}
					else if (Type == DATA_TYPE(CStrID))
					{
						assert(SQLITE_TEXT == ResultColType);
						CStrID Val((const char*)sqlite3_column_text(SQLiteStmt, ResultColIdx));
						pOutTable->Set<CStrID>(ColIdx, RowIdx, Val);
					}
					else if (Type == DATA_TYPE(CBuffer))
					{
						assert(SQLITE_BLOB == ResultColType);
						auto ptr = static_cast<const uint8_t*>(sqlite3_column_blob(SQLiteStmt, ResultColIdx));
						int size = sqlite3_column_bytes(SQLiteStmt, ResultColIdx);
						pOutTable->Set<CBuffer>(ColIdx, RowIdx, CBuffer(ptr, ptr + size));
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
								pOutTable->Set<std::string>(ColIdx, RowIdx, std::string((const char*)sqlite3_column_text(SQLiteStmt, ResultColIdx)));
								break;
							default: assert(false && "ExecuteStatement() > unsupported variable-type DB column type!");
						}
					}
					else assert(false && "ExecuteStatement() > unsupported type!");
				}

				break;
			}
			case SQLITE_ERROR:
			{
				//Messages += "SQLite error during sqlite3_step(): ";
				//Messages += sqlite3_errmsg(SQLiteHandle);
				//Messages += '\n';
				return false;
			}
			default:
			{
				//Messages += "sqlite3_step() returned error code ";
				//Messages += std::to_string(Result);
				//Messages += ", ";
				//Messages += sqlite3_errmsg(SQLiteHandle);
				//Messages += '\n';
				return false;
			}
		}
	}
	while (Result != SQLITE_DONE);

	if (pOutTable) pOutTable->TrackModifications(true);

	return true;
}
//---------------------------------------------------------------------

bool ExecuteSQLQuery(const char* pSQL, CValueTable* pOutTable, const Data::CParams* pParams)
{
	if (!pSQL) return true;

	bool Result = true;
	do
	{
		sqlite3_stmt* SQLiteStmt = nullptr;
		if (sqlite3_prepare_v2(SQLiteHandle, pSQL, -1, &SQLiteStmt, &pSQL) != SQLITE_OK)
		{
			//Messages += "SQLite error: ";
			//Messages += sqlite3_errmsg(SQLiteHandle);
			//Messages += '\n';
			return false;
		}

		while (*pSQL && strchr(" \r\n\t", *pSQL)) ++pSQL;

		// In the last query fill table
		Result = ExecuteStatement(SQLiteStmt, (!*pSQL && pOutTable) ? pOutTable : nullptr, pParams);

		sqlite3_finalize(SQLiteStmt);
	}
	while (Result && *pSQL);

	return Result;
}
//---------------------------------------------------------------------

// NB: pURI must be encoded in UTF-8
bool OpenConnection(const char* pURI)
{
	assert(!SQLiteHandle);

	fs::create_directories(fs::path(pURI).parent_path());

	const int Result = sqlite3_open_v2(pURI, &SQLiteHandle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
	if (Result != SQLITE_OK)
	{
		//Messages += "SQLite error: ";
		//Messages += sqlite3_errmsg(SQLiteHandle);
		//Messages += '\n';
		SQLiteHandle = nullptr;
		return false;
	}

	if (sqlite3_busy_timeout(SQLiteHandle, 100) != SQLITE_OK ||
		sqlite3_extended_result_codes(SQLiteHandle, 1) != SQLITE_OK)
	{
		//Messages += "SQLite error: ";
		//Messages += sqlite3_errmsg(SQLiteHandle);
		//Messages += '\n';
		CloseConnection();
		return false;
	}

	// PRAGMA threads = N; - for worker threads
	// The same as sqlite3_limit, see https://www.sqlite.org/c3ref/limit.html

	constexpr char* pSQLPragmas = "\
PRAGMA encoding = \"UTF-8\";\
PRAGMA journal_mode=MEMORY;\
PRAGMA cache_size=2048;\
PRAGMA synchronous=NORMAL;\
PRAGMA temp_store=MEMORY";
	if (!ExecuteSQLQuery(pSQLPragmas))
	{
		CloseConnection();
		return false;
	}

	// Query tables and create them if DB is empty
	CValueTable Tables;
	if (!ExecuteSQLQuery("SELECT name FROM sqlite_master WHERE type='table'", &Tables))
	{
		CloseConnection();
		return false;
	}

	if (!Tables.GetRowCount())
	{
		const char* pCreateDBSQL = "\
CREATE TABLE 'ShaderBinaries' (\
	'ID' INTEGER,\
	'Path' VARCHAR(1024) NOT NULL,\
	'Size' INTEGER,\
	'BytecodeSize' INTEGER,\
	'CRC' INTEGER,\
	PRIMARY KEY (ID ASC) ON CONFLICT REPLACE);\
\
CREATE TABLE 'InputSignatures' (\
	'ID' INTEGER,\
	'Folder' VARCHAR(1024) NOT NULL,\
	'Size' INTEGER,\
	'CRC' INTEGER,\
	PRIMARY KEY (ID ASC) ON CONFLICT REPLACE);\
\
CREATE TABLE 'Shaders' (\
	'ID' INTEGER,\
	'ShaderType' INTEGER,\
	'Target' INTEGER,\
	'EntryPoint' VARCHAR(64),\
	'CompilerVersion' INTEGER,\
	'CompilerFlags' INTEGER,\
	'SrcPath' VARCHAR(1024) NOT NULL,\
	'SrcModifyTimestamp' INTEGER,\
	'SrcSize' INTEGER,\
	'SrcCRC' INTEGER,\
	'BinaryFileID' INTEGER,\
	'InputSigFileID' INTEGER,\
	PRIMARY KEY (ID ASC) ON CONFLICT REPLACE,\
	FOREIGN KEY (BinaryFileID) REFERENCES ShaderBinaries(ID),\
	FOREIGN KEY (InputSigFileID) REFERENCES InputSignatures(ID));\
\
CREATE TABLE 'Macros' (\
	'ShaderID' INTEGER,\
	'Name' VARCHAR(64),\
	'Value' VARCHAR(1024),\
	PRIMARY KEY (ShaderID, Name) ON CONFLICT REPLACE,\
	FOREIGN KEY (ShaderID) REFERENCES Shaders(ID));\
\
CREATE INDEX ShaderBinaries_MainIndex ON ShaderBinaries (Size, CRC);\
\
CREATE INDEX Shaders_MainIndex ON Shaders (SrcPath, ShaderType, Target, EntryPoint)";
		if (!ExecuteSQLQuery(pCreateDBSQL))
		{
			CloseConnection();
			return false;
		}
	}

	return true;
}
//---------------------------------------------------------------------

#define SAFE_RELEASE_SQL(Stmt) if (Stmt) { sqlite3_finalize(Stmt); Stmt = nullptr; }

void CloseConnection()
{
	if (!SQLiteHandle) return;

	// sqlite3_finalize all precompiled statements here

	if (sqlite3_close(SQLiteHandle) != SQLITE_OK)
	{
		//Messages += "SQLite error: ";
		//Messages += sqlite3_errmsg(SQLiteHandle);
		//Messages += '\n';
	}

	SQLiteHandle = nullptr;
}
//---------------------------------------------------------------------

bool FindShaderRecord(CShaderRecord& InOut)
{
	Data::CParams Params;
	Params.emplace_back(CStrID("Path"), InOut.SrcFile.Path);
	Params.emplace_back(CStrID("Type"), static_cast<int>(InOut.ShaderType));
	Params.emplace_back(CStrID("Target"), static_cast<int>(InOut.Target));
	Params.emplace_back(CStrID("Entry"), InOut.EntryPoint);

	constexpr char* pSQLSelect =
		"SELECT ID, CompilerVersion, CompilerFlags, SrcModifyTimestamp, SrcSize, SrcCRC, BinaryFileID, InputSigFileID "
		"FROM Shaders "
		"WHERE SrcPath=:Path AND ShaderType=:Type AND Target=:Target AND EntryPoint=:Entry";

	CValueTable Shaders;
	if (!ExecuteSQLQuery(pSQLSelect, &Shaders, &Params)) return false;

	Data::CParams DefineParams;

	bool Found = false;

	const int Col_ID = Shaders.GetColumnIndex(CStrID("ID"));
	for (size_t ShIdx = 0; ShIdx < Shaders.GetRowCount(); ++ShIdx)
	{
		DefineParams.emplace_back(CStrID("ShaderID"), Shaders.Get<int>(Col_ID, ShIdx));

		CValueTable Defines;
		if (!ExecuteSQLQuery("SELECT Name, Value FROM Macros WHERE ShaderID = :ShaderID"/*ORDER BY Name*/, &Defines, &DefineParams)) return false;

		const size_t DBDefineCount = Defines.GetRowCount();
		const size_t LocalDefineCount = InOut.Defines.size();
		if (DBDefineCount != LocalDefineCount)
		{
			Found = false;
			break;
		}

		Found = true;

		int Col_Name = Defines.GetColumnIndex(CStrID("Name"));
		int Col_Value = Defines.GetColumnIndex(CStrID("Value"));
		for (size_t DefIdx = 0; DefIdx < Defines.GetRowCount(); ++DefIdx)
		{
			const auto& DBDefineName = Defines.Get<std::string>(Col_Name, DefIdx);
			assert(!DBDefineName.empty());

			//???TODO: compare through INNER JOIN / WHERE?
			for (const auto& NewDefine : InOut.Defines)
			{
				if (NewDefine.first == DBDefineName)
				{
					Data::CData Value;
					Defines.GetValue(Col_Value, DefIdx, Value);
					const std::string DBDefineValue = Value.IsNull() ? std::string{} : Value.GetValue<std::string>();
					if (NewDefine.second != DBDefineValue)
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
			InOut.CompilerVersion = Shaders.Get<int>(CStrID("CompilerVersion"), ShIdx);
			InOut.CompilerFlags = Shaders.Get<int>(CStrID("CompilerFlags"), ShIdx);
			InOut.SrcModifyTimestamp = Shaders.Get<int>(CStrID("SrcModifyTimestamp"), ShIdx);
			InOut.SrcFile.Size = Shaders.Get<int>(CStrID("SrcSize"), ShIdx);
			InOut.SrcFile.CRC = Shaders.Get<int>(CStrID("SrcCRC"), ShIdx);
			InOut.ObjFile.ID = Shaders.Get<int>(CStrID("BinaryFileID"), ShIdx);
			InOut.InputSigFile.ID = Shaders.Get<int>(CStrID("InputSigFileID"), ShIdx);
			return true;
		}
	}

	InOut.ID = 0;
	InOut.CompilerVersion = 0;
	InOut.CompilerFlags = 0;
	InOut.ObjFile.ID = 0;
	InOut.ObjFile.Path.clear();
	InOut.InputSigFile.ID = 0;
	InOut.InputSigFile.Folder.clear();

	return false; // Not found
}
//---------------------------------------------------------------------

bool WriteShaderRecord(CShaderRecord& InOut)
{
	Data::CParams Params;
	Params.emplace_back(CStrID("ShaderType"), static_cast<int>(InOut.ShaderType));
	Params.emplace_back(CStrID("Target"), static_cast<int>(InOut.Target));
	Params.emplace_back(CStrID("EntryPoint"), InOut.EntryPoint);
	Params.emplace_back(CStrID("CompilerVersion"), static_cast<int>(InOut.CompilerVersion));
	Params.emplace_back(CStrID("CompilerFlags"), static_cast<int>(InOut.CompilerFlags));
	Params.emplace_back(CStrID("SrcPath"), InOut.SrcFile.Path);
	Params.emplace_back(CStrID("SrcModifyTimestamp"), static_cast<int>(InOut.SrcModifyTimestamp));
	Params.emplace_back(CStrID("SrcSize"), static_cast<int>(InOut.SrcFile.Size));
	Params.emplace_back(CStrID("SrcCRC"), static_cast<int>(InOut.SrcFile.CRC));
	Params.emplace_back(CStrID("BinaryFileID"), static_cast<int>(InOut.ObjFile.ID));
	Params.emplace_back(CStrID("InputSigFileID"), static_cast<int>(InOut.InputSigFile.ID));

	auto ID = InOut.ID;
	if (!ID)
	{
		// We have no ID, insert a new line
		constexpr char* pSQLInsert =
			"INSERT INTO Shaders "
			"   (ShaderType, Target, EntryPoint, CompilerVersion, CompilerFlags, SrcPath, SrcModifyTimestamp, SrcSize, SrcCRC, BinaryFileID, InputSigFileID) "
			"VALUES "
			"   (:ShaderType, :Target, :EntryPoint, :CompilerVersion, :CompilerFlags, :SrcPath, :SrcModifyTimestamp, :SrcSize, :SrcCRC, :BinaryFileID, :InputSigFileID)";
		if (!ExecuteSQLQuery(pSQLInsert, nullptr, &Params)) return false;

		ID = static_cast<uint32_t>(sqlite3_last_insert_rowid(SQLiteHandle));
		if (!ID) return false;
	}
	else
	{
		// We have ID, update existing row
		Params.emplace_back(CStrID("ID"), static_cast<int>(ID));

		constexpr char* pSQLUpdate =
			"UPDATE Shaders SET "
			"	ShaderType=:ShaderType, "
			"	Target=:Target, "
			"	EntryPoint=:EntryPoint, "
			"	CompilerVersion=:CompilerVersion, "
			"	CompilerFlags=:CompilerFlags, "
			"	SrcPath=:SrcPath, "
			"	SrcModifyTimestamp=:SrcModifyTimestamp, "
			"	SrcSize=:SrcSize, "
			"	SrcCRC=:SrcCRC, "
			"	BinaryFileID=:BinaryFileID, "
			"	InputSigFileID=:InputSigFileID "
			"WHERE ID=:ID";
		if (!ExecuteSQLQuery(pSQLUpdate, nullptr, &Params)) return false;
	}

	InOut.ID = ID;

	// TODO: check if batch is better
	/*
	DELETE FROM Macros WHERE ShaderID = :ShaderID;

	INSERT INTO table1 (column1,column2 ,..)
	VALUES 
	   (value1,value2 ,...),
	   (value1,value2 ,...),
		...
	   (value1,value2 ,...);	
	*/

	Params.clear();
	Params.emplace_back(CStrID("ShaderID"), static_cast<int>(InOut.ID));
	if (!ExecuteSQLQuery("DELETE FROM Macros WHERE ShaderID = :ShaderID", nullptr, &Params)) return false;

	for (const auto& Macro : InOut.Defines)
	{
		if (Macro.first.empty()) continue;
		Params.emplace_back(CStrID("Name"), Macro.first);
		Params.emplace_back(CStrID("Value"), Macro.second);
		if (!ExecuteSQLQuery("INSERT INTO Macros (ShaderID, Name, Value) VALUES (:ShaderID, :Name, :Value)", nullptr, &Params)) return false;
	}

	return true;
}
//---------------------------------------------------------------------

bool FindSignatureRecord(CSignatureRecord& InOut, const char* pBasePath, const void* pBinaryData)
{
	InOut.ID = 0;
	InOut.Folder.clear();

	if (!InOut.Size) return false; // Not found

	Data::CParams Params;
	Params.emplace_back(CStrID("Size"), static_cast<int>(InOut.Size));
	Params.emplace_back(CStrID("CRC"), static_cast<int>(InOut.CRC));

	CValueTable Result;
	if (!ExecuteSQLQuery("SELECT ID, Folder FROM InputSignatures WHERE Size=:Size AND CRC=:CRC", &Result, &Params)) return false;

	const int Col_ID = Result.GetColumnIndex(CStrID("ID"));
	const int Col_Folder = Result.GetColumnIndex(CStrID("Folder"));
	for (size_t i = 0; i < Result.GetRowCount(); ++i)
	{
		const int ID = Result.Get<int>(Col_ID, i);
		const std::string Folder = Result.Get<std::string>(Col_Folder, i);

		// If binary data is passed in, compare bytewise
		if (pBinaryData)
		{
			auto Path = fs::path(Folder) / (std::to_string(ID) + ".sig");
			if (pBasePath && Path.is_relative())
				Path = fs::path(pBasePath) / Path;

			std::vector<char> Buffer;
			if (!ReadAllFile(Path.string().c_str(), Buffer)) continue;
			if (Buffer.size() != InOut.Size) continue;
			if (memcmp(pBinaryData, Buffer.data(), Buffer.size()) != 0) continue;
		}

		InOut.ID = ID;
		InOut.Folder = std::move(Folder);
		return true; // Found
	}

	return false; // Not found
}
//---------------------------------------------------------------------

bool WriteSignatureRecord(CSignatureRecord& InOut)
{
	if (InOut.Folder.empty() || !InOut.Size) return false;

	auto ID = InOut.ID;
	if (!ID)
	{
		// If new record, try to find free ID
		CValueTable Result;
		if (!ExecuteSQLQuery("SELECT ID FROM InputSignatures WHERE Size = 0 ORDER BY ID LIMIT 1", &Result)) return false;
		if (Result.GetRowCount())
			ID = Result.Get<int>(CStrID("ID"), 0);
	}

	Data::CParams Params;
	Params.emplace_back(CStrID("Folder"), InOut.Folder);
	Params.emplace_back(CStrID("Size"), static_cast<int>(InOut.Size));
	Params.emplace_back(CStrID("CRC"), static_cast<int>(InOut.CRC));

	if (!ID)
	{
		// No free ID, insert a new line
		if (!ExecuteSQLQuery("INSERT INTO InputSignatures (Folder, Size, CRC) VALUES (:Folder, :Size, :CRC)", nullptr, &Params)) return false;
		ID = static_cast<uint32_t>(sqlite3_last_insert_rowid(SQLiteHandle));
		if (!ID) return false;
	}
	else
	{
		// We have ID, update existing row
		Params.emplace_back(CStrID("ID"), static_cast<int>(ID));
		if (!ExecuteSQLQuery("UPDATE InputSignatures SET Folder=:Folder, Size=:Size, CRC=:CRC WHERE ID=:ID", nullptr, &Params)) return false;
	}

	InOut.ID = ID;
	return true;
}
//---------------------------------------------------------------------

bool ReleaseSignatureRecord(uint32_t ID, std::string& OutPath)
{
	if (ID == 0) return false;

	Data::CParams Params;
	Params.emplace_back(CStrID("ID"), static_cast<int>(ID));

	CValueTable Result;
	if (!ExecuteSQLQuery("SELECT ID FROM Shaders WHERE InputSigFileID=:ID", &Result, &Params)) return false;

	// References found, don't delete file
	if (Result.GetRowCount()) return false;

	// Get file path to delete
	Result.Clear();
	if (!ExecuteSQLQuery("SELECT Folder FROM InputSignatures WHERE ID=:ID", &Result, &Params)) return false;
	if (!Result.GetRowCount()) return false;

	const auto& Folder = Result.Get<std::string>(CStrID("Folder"), 0);
	OutPath = (fs::path(Folder) / (std::to_string(ID) + ".sig")).string();

	return ExecuteSQLQuery("UPDATE InputSignatures SET Size=0 WHERE ID=:ID", nullptr, &Params);
}
//---------------------------------------------------------------------

bool FindBinaryRecord(CBinaryRecord& InOut, const char* pBasePath, const void* pBinaryData, bool USM)
{
	InOut.ID = 0;
	InOut.Path.clear();

	if (!InOut.BytecodeSize) return false; // Not found

	Data::CParams Params;
	Params.emplace_back(CStrID("BytecodeSize"), static_cast<int>(InOut.BytecodeSize));
	Params.emplace_back(CStrID("CRC"), static_cast<int>(InOut.CRC));

	CValueTable Result;
	if (!ExecuteSQLQuery("SELECT ID, Path FROM ShaderBinaries WHERE BytecodeSize=:BytecodeSize AND CRC=:CRC", &Result, &Params)) return false;

	const int Col_ID = Result.GetColumnIndex(CStrID("ID"));
	const int Col_Path = Result.GetColumnIndex(CStrID("Path"));
	for (size_t i = 0; i < Result.GetRowCount(); ++i)
	{
		std::string PathStr = Result.Get<std::string>(Col_Path, i);

		// If binary data is passed in, compare bytewise
		if (pBinaryData)
		{
			auto Path = fs::path(PathStr);
			if (pBasePath && Path.is_relative())
				Path = fs::path(pBasePath) / Path;

			std::vector<char> Buffer;
			if (!ReadAllFile(Path.string().c_str(), Buffer)) continue;

			const char* pFileData = Buffer.data();
			size_t FileSize = Buffer.size();
			if (USM)
			{
				// Shader bytecode only, skip header (9 bytes)
				const uint32_t BytecodeOffset = *reinterpret_cast<const uint32_t*>(pFileData + 9);
				pFileData += BytecodeOffset;
				FileSize -= BytecodeOffset;
				if (FileSize != InOut.BytecodeSize) continue;
			}
			else
			{
				// Shader bytecode and metadata, skip header (9 bytes) & bytecode offset (4 bytes)
				if (FileSize <= 13 + InOut.BytecodeSize) continue;
				pFileData += 13;
				FileSize -= 13;
			}

			if (memcmp(pBinaryData, pFileData, FileSize) != 0) continue;
		}

		InOut.ID = Result.Get<int>(Col_ID, i);
		InOut.Path = std::move(PathStr);
		return true; // Found
	}

	return false; // Not found
}
//---------------------------------------------------------------------

bool WriteBinaryRecord(CBinaryRecord& InOut)
{
	if (InOut.Path.empty() || !InOut.Size) return false;

	auto ID = InOut.ID;
	if (!ID)
	{
		// If new record, try to find free ID
		CValueTable Result;
		if (!ExecuteSQLQuery("SELECT ID FROM ShaderBinaries WHERE Size = 0 ORDER BY ID LIMIT 1", &Result)) return false;
		if (Result.GetRowCount())
			ID = Result.Get<int>(CStrID("ID"), 0);
	}

	Data::CParams Params;
	Params.emplace_back(CStrID("Path"), InOut.Path);
	Params.emplace_back(CStrID("Size"), static_cast<int>(InOut.Size));
	Params.emplace_back(CStrID("BytecodeSize"), static_cast<int>(InOut.BytecodeSize));
	Params.emplace_back(CStrID("CRC"), static_cast<int>(InOut.CRC));

	if (!ID)
	{
		// No free ID, insert a new line
		if (!ExecuteSQLQuery("INSERT INTO ShaderBinaries (Path, Size, BytecodeSize, CRC) VALUES (:Path, :Size, :BytecodeSize, :CRC)", nullptr, &Params)) return false;
		ID = static_cast<uint32_t>(sqlite3_last_insert_rowid(SQLiteHandle));
		if (!ID) return false;
	}
	else
	{
		// We have ID, update existing row
		Params.emplace_back(CStrID("ID"), static_cast<int>(ID));
		if (!ExecuteSQLQuery("UPDATE ShaderBinaries SET Path=:Path, Size=:Size, BytecodeSize=:BytecodeSize, CRC=:CRC WHERE ID=:ID", nullptr, &Params)) return false;
	}

	InOut.ID = ID;
	return true;
}
//---------------------------------------------------------------------

bool ReleaseBinaryRecord(uint32_t ID, std::string& OutPath)
{
	if (ID == 0) return false;

	Data::CParams Params;
	Params.emplace_back(CStrID("ID"), static_cast<int>(ID));

	CValueTable Result;
	if (!ExecuteSQLQuery("SELECT ID FROM Shaders WHERE BinaryFileID=:ID", &Result, &Params)) return false;

	// References found, don't delete file
	if (Result.GetRowCount()) return false;

	// Get file path to delete
	Result.Clear();
	if (!ExecuteSQLQuery("SELECT * FROM ShaderBinaries WHERE ID=:ID", &Result, &Params)) return false;
	OutPath = Result.Get<std::string>(CStrID("Path"), 0);

	return ExecuteSQLQuery("UPDATE ShaderBinaries SET Size=0 WHERE ID=:ID", nullptr, &Params);
}
//---------------------------------------------------------------------

bool FindObjFileByID(uint32_t ID, CBinaryRecord& Out)
{
	Data::CParams Params;
	Params.emplace_back(CStrID("ID"), static_cast<int>(ID));

	CValueTable Result;
	if (!ExecuteSQLQuery("SELECT * FROM ShaderBinaries WHERE ID=:ID", &Result, &Params)) return false;
	if (!Result.GetRowCount()) return false;

	Out.ID = Result.Get<int>(CStrID("ID"), 0);
	Out.Path = Result.Get<std::string>(CStrID("Path"), 0);
	Out.Size = Result.Get<int>(CStrID("Size"), 0);
	Out.BytecodeSize = Result.Get<int>(CStrID("BytecodeSize"), 0);
	Out.CRC = Result.Get<int>(CStrID("CRC"), 0);

	return true; // Found
}
//---------------------------------------------------------------------

}