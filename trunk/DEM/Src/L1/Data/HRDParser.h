#pragma once
#ifndef __DEM_L1_HRD_PARSER_H__
#define __DEM_L1_HRD_PARSER_H__

#include <Core/RefCounted.h>
#include "Data.h"

// Parser for HRD files

namespace Data
{
typedef Ptr<class CDataArray> PDataArray;
typedef Ptr<class CParams> PParams;

#define HRDParser Data::CHRDParser::Instance()

class CHRDParser: public Core::CRefCounted
{
	DeclareRTTI;
	DeclareFactory(CHRDParser);

private:

	static CHRDParser*	Singleton;

	enum ETable
	{
		TBL_RW,
		TBL_ID,
		TBL_CONST,
		TBL_DLM
	};
	
	struct CToken
	{
		ETable	Table;
		int		Index;
		int		Ln, Cl;
		CToken() {}
		CToken(ETable Tbl, int Idx): Table(Tbl), Index(Idx), Ln(HRDParser->Line), Cl(HRDParser->Col) {}
		bool IsA(ETable Tbl, int Idx) { return Table == Tbl && Index == Idx; }
	};
			
	enum EType
	{
		T_INT,
		T_INT_HEX,
		T_FLOAT,
		T_STRING,
		T_STRID
	};
	
	nArray<nString>	TableID;
	nArray<nString>	TableRW;
	nArray<nString>	TableDlm;
	nArray<CData>	TableConst;

	LPCSTR			LexerCursor;
	LPCSTR			EndOfBuffer;
	LPCSTR			TokenStart;
	
	int				ZeroIdx;
	
	DWORD			Line,
					Col;
					
	int				ParserCursor;
	
	//???nArray<CToken> Tokens;?
	
	// Lexical analysis
	//!!!void SetupRWAndDlm();
	bool Tokenize(nArray<CToken>& Tokens);
	bool LexProcessID(nArray<CToken>& Tokens);
	bool LexProcessHex(nArray<CToken>& Tokens);
	bool LexProcessFloat(nArray<CToken>& Tokens);
	bool LexProcessNumeric(nArray<CToken>& Tokens);
	bool LexProcessString(nArray<CToken>& Tokens, char QuoteChar = '"');
	bool LexProcessBigString(nArray<CToken>& Tokens);
	bool LexProcessDlm(nArray<CToken>& Tokens);
	bool LexProcessCommentLine();
	bool LexProcessCommentBlock();
	void DlmMatchChar(char Char, int Index, int Start, int End, int& MatchStart, int& MatchEnd);
	void SkipSpaces();
	void AddConst(nArray<CToken>& Tokens, const nString& Const, EType Type);
	
	// Syntax analysis
	bool ParseTokenStream(const nArray<CToken>& Tokens, PParams Output);
	bool ParseParam(const nArray<CToken>& Tokens, PParams Output);
	bool ParseData(const nArray<CToken>& Tokens, CData& Output);
	bool ParseArray(const nArray<CToken>& Tokens, PDataArray Output);
	bool ParseSection(const nArray<CToken>& Tokens, PParams Output);
	bool ParseVector(const nArray<CToken>& Tokens, CData& Output);

public:

	CHRDParser();
	~CHRDParser();

	static CHRDParser* Instance() { n_assert(Singleton); return Singleton; }

	bool ParseBuffer(LPCSTR Buffer, DWORD Length, PParams& Result);
};

RegisterFactory(CHRDParser);

}

#endif
