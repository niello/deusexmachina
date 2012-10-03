#pragma once
#ifndef __DEM_L1_DB_COMMAND_H__
#define __DEM_L1_DB_COMMAND_H__

#include <Core/RefCounted.h>
#include <util/nstring.h>
#include "AttrID.h"
#include "ValueTable.h"

// Wraps a general SQL command. Commands may contain placeholders
// and can be precompiled for faster execution.

typedef struct sqlite3_stmt sqlite3_stmt;

namespace DB
{
typedef Ptr<class CDatabase> PDatabase;

class CCommand: public Core::CRefCounted
{
	__DeclareClass(CCommand);

protected:

	nString			SQLCmd;
	nString			ErrorMsg;
	PValueTable		VT;
	sqlite3_stmt*	SQLiteStmt;
	nArray<int>		ResultIdxMap;

	void SetError(const nString& Error) { ErrorMsg = Error; }
	void ReadRow();

public:

	CCommand();
	virtual ~CCommand();

	void	SetSQL(const nString& SQL);
	void	SetResultTable(CValueTable* pResultTable = NULL);
	bool	Compile(const PDatabase& DB);
	bool	Compile(const PDatabase& DB, const nString& SQL, CValueTable* pResultTable = NULL);
	bool	Execute(const PDatabase& DB);
	bool	Execute(const PDatabase& DB, const nString& SQL, CValueTable* pResultTable = NULL);
	void	Clear();

	int		BindingIndexOf(const nString& Name) const;
	int		BindingIndexOf(CAttrID AttrID) const;
	void	BindValue(int Idx, const CData& Val);
	void	BindValue(const nString& Name, const CData& Val) { BindValue(BindingIndexOf(Name), Val); }
	void	BindValue(CAttrID ID, const CData& Val) { BindValue(BindingIndexOf(ID), Val); }
	//???add BindValue<T> or can't be virtual?

	const nString&		GetError() const { return ErrorMsg; }
	const nString&		GetSQL() const { return SQLCmd; }
	const PValueTable&	GetResultTable() const { return VT; }
	bool				IsValid() const { return SQLiteStmt != NULL; }
};
//---------------------------------------------------------------------

typedef Ptr<CCommand> PCommand;

inline void CCommand::SetSQL(const nString& SQL)
{
	n_assert(SQL.IsValid());
	Clear();
	SQLCmd = SQL;
}
//---------------------------------------------------------------------

inline void CCommand::SetResultTable(CValueTable* pResultTable)
{
	ResultIdxMap.Clear();
	VT = pResultTable;
}
//---------------------------------------------------------------------

inline bool CCommand::Compile(const PDatabase& DB, const nString& SQL, CValueTable* pResultTable)
{
	n_assert(SQL.IsValid());
	Clear();
	SQLCmd = SQL;
	VT = pResultTable;
	return Compile(DB);
}
//---------------------------------------------------------------------

inline bool CCommand::Execute(const PDatabase& DB, const nString& SQL, CValueTable* pResultTable)
{
	if (Compile(DB, SQL, pResultTable)) return Execute(DB);
	FAIL;
}
//---------------------------------------------------------------------

}

#endif
