#pragma once
#include <Data/Ptr.h>
#include <Data/Data.h>

// Parser for HRD files

namespace Data
{
class CDataArray;
class CParams;

class CHRDParser
{
private:

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
		CToken(ETable Tbl, int Idx, int Line, int Col): Table(Tbl), Index(Idx), Ln(Line), Cl(Col) {}
		bool IsA(ETable Tbl, int Idx) const { return Table == Tbl && Index == Idx; }
	};
			
	enum EType
	{
		T_INT,
		T_INT_HEX,
		T_FLOAT,
		T_STRING,
		T_STRID
	};
	
	std::vector<std::string> TableID;
	std::vector<std::string> TableRW;
	std::vector<std::string> TableDlm;
	std::vector<CData>       TableConst;

	const char*		LexerCursor;
	const char*		EndOfBuffer;
	const char*		TokenStart;
	
	int				ZeroIdx;
	
	UPTR			Line,
					Col;
					
	UPTR			ParserCursor;

	std::string*	pErr;
	
	// Lexical analysis
	//!!!void SetupRWAndDlm();
	bool Tokenize(std::vector<CToken>& Tokens);
	bool LexProcessID(std::vector<CToken>& Tokens);
	bool LexProcessHex(std::vector<CToken>& Tokens);
	bool LexProcessFloat(std::vector<CToken>& Tokens);
	bool LexProcessNumeric(std::vector<CToken>& Tokens);
	bool LexProcessString(std::vector<CToken>& Tokens, char QuoteChar = '"');
	bool LexProcessBigString(std::vector<CToken>& Tokens);
	bool LexProcessDlm(std::vector<CToken>& Tokens);
	bool LexProcessCommentLine();
	bool LexProcessCommentBlock();
	void DlmMatchChar(char Char, int Index, int Start, int End, int& MatchStart, int& MatchEnd);
	void SkipSpaces();
	void AddConst(std::vector<CToken>& Tokens, const std::string& Const, EType Type);
	
	// Syntax analysis
	bool ParseTokenStream(const std::vector<CToken>& Tokens, CParams& Output);
	bool ParseParam(const std::vector<CToken>& Tokens, CParams& Output);
	bool ParseData(const std::vector<CToken>& Tokens, CData& Output);
	bool ParseArray(const std::vector<CToken>& Tokens, CDataArray& Output);
	bool ParseSection(const std::vector<CToken>& Tokens, CParams& Output);
	bool ParseVector(const std::vector<CToken>& Tokens, CData& Output);

public:

	CHRDParser();

	bool ParseBuffer(const char* Buffer, UPTR Length, CParams& Result, std::string* pErrors = nullptr);
};

}
