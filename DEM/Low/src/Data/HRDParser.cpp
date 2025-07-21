#include "HRDParser.h"

#include <Data/DataArray.h>
#include <Data/Params.h>
#include <Data/StringUtils.h>
#include <ctype.h>

namespace Data
{
enum
{
	RW_FALSE = 0,
	RW_TRUE = 2,
	RW_NULL = 1
};

enum
{
	DLM_EQUAL,
	DLM_SQ_BR_OPEN,
	DLM_COMMA,
	DLM_SQ_BR_CLOSE,
	DLM_CUR_BR_OPEN,
	DLM_CUR_BR_CLOSE,
	DLM_BR_OPEN,
	DLM_BR_CLOSE
};

CHRDParser::CHRDParser(): pErr(nullptr)
{
	// NB: Keep sorted and keep enum above updated
	TableRW.push_back(CString("false"));
	TableRW.push_back(CString("null"));
	TableRW.push_back(CString("true"));
	
	TableDlm.push_back(CString("="));
	TableDlm.push_back(CString("["));
	TableDlm.push_back(CString(","));
	TableDlm.push_back(CString("]"));
	TableDlm.push_back(CString("{"));
	TableDlm.push_back(CString("}"));
	TableDlm.push_back(CString("("));
	TableDlm.push_back(CString(")"));
}
//---------------------------------------------------------------------

bool CHRDParser::ParseBuffer(const char* Buffer, UPTR Length, CParams& Result, CString* pErrors)
{
	Result.Clear();

	if (!Buffer) FAIL;

	ZeroIdx = INVALID_INDEX;

	// Allow empty file to be parsed
	if (!Length) OK;

	pErr = pErrors;
	
	// Check for UTF-8 BOM signature (0xEF 0xBB 0xBF)
	if (Length >= 3 && Buffer[0] == (char)0xEF && Buffer[1] == (char)0xBB && Buffer[2] == (char)0xBF)
		LexerCursor = Buffer + 3;
	else LexerCursor = Buffer;
	
	EndOfBuffer = Buffer + Length;

	if (LexerCursor == EndOfBuffer) FAIL;

	Line = Col = 1;

	std::vector<CToken> Tokens;
	if (!Tokenize(Tokens))
	{
		if (pErr) pErr->Add("Lexical analysis of HRD failed\n");
		TableID.clear();
		TableConst.clear(); //???always keep "0" const?
		FAIL;
	}
	
	if (!ParseTokenStream(Tokens, Result))
	{
		if (pErr) pErr->Add("Syntax analysis of HRD failed\n");
		Result.Clear();
		TableID.clear();
		TableConst.clear(); //???always keep "0" const?
		//Tokens.Clear();
		FAIL;
	}
	
	TableID.clear();
	TableConst.clear(); //???always keep "0" const?
	//Tokens.Clear();

	pErr = nullptr;

	OK;
}
//---------------------------------------------------------------------

bool CHRDParser::Tokenize(std::vector<CToken>& Tokens)
{
	while (LexerCursor < EndOfBuffer)
	{
		char CurrChar = *LexerCursor;
		TokenStart = LexerCursor;
		if (CurrChar == '0')
		{
			LexerCursor++;
			Col++;
			if (LexerCursor < EndOfBuffer)
			{
				CurrChar = *LexerCursor;
				if (CurrChar == 'x' || CurrChar == 'X')
				{
					if (!LexProcessHex(Tokens)) FAIL;
					continue;
				}
				else if (CurrChar == '.')
				{
					if (!LexProcessFloat(Tokens)) FAIL;
					continue;
				}
				else if (isdigit(CurrChar))
				{
					if (!LexProcessNumeric(Tokens)) FAIL;
					continue;
				}
			}

			//???add zero const at parsing start?
			if (ZeroIdx == INVALID_INDEX)
			{
				ZeroIdx = TableConst.size();
				TableConst.push_back((int)0); //!!!preallocate & set inplace!
			}
			Tokens.push_back(CToken(TBL_CONST, ZeroIdx, Line, Col));
		}
		else if (CurrChar == '.')
		{
			LexerCursor++;
			Col++;
			if (LexerCursor < EndOfBuffer && isdigit(*LexerCursor))
			{
				if (!LexProcessFloat(Tokens)) FAIL;
				continue;
			}
			if (!LexProcessDlm(Tokens)) FAIL; //???cursor 1 char back?
		}
		else if (CurrChar == '"' || CurrChar == '\'')
		{
			if (!LexProcessString(Tokens, CurrChar)) FAIL;
		}
		else if (CurrChar == '<')
		{
			LexerCursor++;
			Col++;
			if (LexerCursor < EndOfBuffer)
			{
				CurrChar = *LexerCursor;
				if (CurrChar == '"')
				{
					if (!LexProcessBigString(Tokens)) FAIL;
					continue;
				}
			}
			if (!LexProcessDlm(Tokens)) FAIL; //???cursor 1 char back?
		}
		else if (CurrChar == '/')
		{
			LexerCursor++;
			Col++;
			if (LexerCursor < EndOfBuffer)
			{
				CurrChar = *LexerCursor;
				if (CurrChar == '/')
				{
					if (!LexProcessCommentLine()) FAIL;
					continue;
				}
				else if (CurrChar == '*')
				{
					if (!LexProcessCommentBlock()) FAIL;
					continue;
				}
			}
			if (!LexProcessDlm(Tokens)) FAIL; //???cursor 1 char back?
		}
		else if (CurrChar == '-' || isdigit(CurrChar))
		{
			if (!LexProcessNumeric(Tokens)) FAIL;
		}
		else if (isalpha(CurrChar) || CurrChar == '_')
		{
			if (!LexProcessID(Tokens)) FAIL;
		}
		else if (CurrChar == ' ' || CurrChar == '\t' || CurrChar == '\n' || CurrChar == '\r')
			SkipSpaces();
		else if (!LexProcessDlm(Tokens)) FAIL;
	}
	
	OK;
}
//---------------------------------------------------------------------

bool CHRDParser::LexProcessID(std::vector<CToken>& Tokens)
{
	LexerCursor++;
	Col++;
	while (LexerCursor < EndOfBuffer)
	{
		char CurrChar = *LexerCursor;
		if (isalpha(CurrChar) || isdigit(CurrChar) || CurrChar == '_')
		{
			LexerCursor++;
			Col++;
		}
		else break;
	}
	
	CString NewID;
	NewID.Set(TokenStart, LexerCursor - TokenStart);

	{
		auto It = std::lower_bound(TableRW.cbegin(), TableRW.cend(), NewID);
		if (It != TableRW.end() && *It == NewID)
		{
			Tokens.push_back(CToken(TBL_RW, std::distance(TableID.cbegin(), It), Line, Col));
			OK;
		}
	}
	
	auto It = std::find(TableID.cbegin(), TableID.cend(), NewID);
	if (It == TableID.cend())
	{
		TableID.push_back(NewID);
		It = std::prev(TableID.cend());
	}
	Tokens.push_back(CToken(TBL_ID, std::distance(TableID.cbegin(), It), Line, Col));
	OK;
}
//---------------------------------------------------------------------

bool CHRDParser::LexProcessHex(std::vector<CToken>& Tokens)
{
	do
	{
		LexerCursor++;
		Col++;
	}
	while (LexerCursor < EndOfBuffer && isxdigit(*LexerCursor));
	
	int TokenLength = LexerCursor - TokenStart;
	if (TokenLength < 3)
	{
		if (pErr)
		{
			CString S;
			S.Format("Hexadecimal constant should contain at least one hex digit after '0x' (Ln:%u, Col:%u)\n", Line, Col);
			pErr->Add(S);
		}
		FAIL;
	}
	
	CString Token;
	Token.Set(TokenStart, TokenLength);
	AddConst(Tokens, Token, T_INT_HEX);
	OK;
}
//---------------------------------------------------------------------

bool CHRDParser::LexProcessFloat(std::vector<CToken>& Tokens)
{
	// Dot and at least one digit were read

	do
	{
		LexerCursor++;
		Col++;
	}
	while (LexerCursor < EndOfBuffer && isdigit(*LexerCursor));
		
	CString Token;
	Token.Set(TokenStart, LexerCursor - TokenStart);
	AddConst(Tokens, Token, T_FLOAT);
	OK;
}
//---------------------------------------------------------------------

bool CHRDParser::LexProcessNumeric(std::vector<CToken>& Tokens)
{
	LexerCursor++;
	Col++;
	while (LexerCursor < EndOfBuffer)
	{
		char CurrChar = *LexerCursor;
		if (isdigit(CurrChar))
		{
			LexerCursor++;
			Col++;
		}
		else if (CurrChar == '.') return LexProcessFloat(Tokens);
		else break;
	}

	CString Token;
	Token.Set(TokenStart, LexerCursor - TokenStart);
	AddConst(Tokens, Token, T_INT);
	OK;
}
//---------------------------------------------------------------------

bool CHRDParser::LexProcessString(std::vector<CToken>& Tokens, char QuoteChar)
{
	CString NewConst;
	//NewConst.Reserve(256); //!!!need grow!
	
	LexerCursor++;
	Col++;
	while (LexerCursor < EndOfBuffer)
	{
		char CurrChar = *LexerCursor;
		if (CurrChar == '\\')
		{
			LexerCursor++;
			Col++;
			if (LexerCursor < EndOfBuffer)
			{
				CurrChar = *LexerCursor;
				switch (CurrChar)
				{
					case 't': NewConst += "\t"; break;	//!!!NEED CString += char!
					case '\\': NewConst += "\\"; break;
					case '"': NewConst += "\""; break;
					case '\'': NewConst += "'"; break;
					case 'n': NewConst += "\n"; break;
					case '\r':
						if (LexerCursor + 1 < EndOfBuffer && LexerCursor[1] == '\n') LexerCursor++;
						Col = 0;
						Line++;
						NewConst += "\n";
						break;
					case '\n':
						if (LexerCursor + 1 < EndOfBuffer && LexerCursor[1] == '\r') LexerCursor++;
						Col = 0;
						Line++;
						NewConst += "\n";
						break;
					default:
						{
							//!!!NEED CString += char!
							char Tmp[3];
							Tmp[0] = '\\';
							Tmp[1] = CurrChar;
							Tmp[2] = 0;
							NewConst += Tmp;
							break;
						}
				}
				LexerCursor++;
				Col++;
			}
			else break;
		}
		else if (CurrChar == QuoteChar)
		{
			SkipSpaces();
			if (LexerCursor < EndOfBuffer && (*LexerCursor) == QuoteChar)
			{
				LexerCursor++;
				Col++;
				continue;
			}

			AddConst(Tokens, NewConst, (QuoteChar == '\'') ? T_STRID : T_STRING);
			OK;
		}
		else
		{
			//!!!NEED CString += char! or better pre-allocate+grow & simply set characters by NewConst[Len] = c
			char Tmp[2];
			Tmp[0] = CurrChar;
			Tmp[1] = 0;
			NewConst += Tmp;
			LexerCursor++;
			Col++;
		}
	}
	
	if (pErr)
	{
		CString S;
		S.Format("Unexpected end of file while parsing string constant (Ln:%u, Col:%u)\n", Line, Col);
		pErr->Add(S);
	}

	FAIL;
}
//---------------------------------------------------------------------

// <" STRING CONTENTS ">
bool CHRDParser::LexProcessBigString(std::vector<CToken>& Tokens)
{
	CString NewConst; //!!!preallocate and grow like array!
	
	LexerCursor++;
	Col++;
	while (LexerCursor < EndOfBuffer)
	{
		char CurrChar = *LexerCursor;
		if (CurrChar == '"')
		{
			LexerCursor++;
			Col++;
			if (LexerCursor < EndOfBuffer)
			{
				if ((*LexerCursor) == '>')
				{
					AddConst(Tokens, NewConst, T_STRING);
					LexerCursor++;
					Col++;
					OK;
				}
				else
				{
					NewConst += "\"";
					continue;
				}
			}
			else break;
		}

		//!!!NEED CString += char! or better pre-allocate+grow & simply set characters by NewConst[Len] = c
		char Tmp[2];
		Tmp[0] = CurrChar;
		Tmp[1] = 0;
		NewConst += Tmp;
		LexerCursor++;
		Col++;
	}
	
	if (pErr)
	{
		CString S;
		S.Format("Unexpected end of file while parsing big string constant (Ln:%u, Col:%u)\n", Line, Col);
		pErr->Add(S);
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CHRDParser::LexProcessDlm(std::vector<CToken>& Tokens)
{
	int	Start = 0,
		End = TableDlm.size(),
		MatchStart,
		MatchEnd,
		DetectedLength = 0;

	char CurrChar;

	do
	{
		CurrChar = *LexerCursor;
		
		// TableDlm is sorted, params: char, char pos, start from, end at, match from (out), match end (out)
		DlmMatchChar(CurrChar, DetectedLength, Start, End, MatchStart, MatchEnd);
		if (MatchStart != INVALID_INDEX)
		{
			Start = MatchStart;
			End = MatchEnd;
			DetectedLength++;
			LexerCursor++;
			Col++;
		}
		else break;
	}
	while (LexerCursor < EndOfBuffer);

	if (DetectedLength)
	{
		// Find exact matching
		DlmMatchChar(0, DetectedLength, Start, End, MatchStart, MatchEnd);
		
		if (MatchStart == INVALID_INDEX)
		{
			if (pErr)
			{
				CString S;
				S.Format("Unknown delimiter (Ln:%u, Col:%u)\n", Line, Col);
				pErr->Add(S);
			}
			FAIL;
		}

		n_assert(MatchStart + 1 == MatchEnd); // Assert there are no duplicates
		Tokens.push_back(CToken(TBL_DLM, MatchStart, Line, Col));
		OK;
	}
	
	// No matching dlm at all, invalid character in stream
	if (pErr)
	{
		CString S;
		S.Format("Invalid character '%c' (%u) (Ln:%u, Col:%u)\n", CurrChar, CurrChar, Line, Col);
		pErr->Add(S);
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CHRDParser::LexProcessCommentLine()
{
	LexerCursor++;
	Col++;
	while (LexerCursor < EndOfBuffer)
	{
		char CurrChar = *LexerCursor;
		if (CurrChar == '\n' || CurrChar == '\r') OK;
		else
		{
			LexerCursor++;
			Col++;
		}
	}
	
	OK;
}
//---------------------------------------------------------------------

bool CHRDParser::LexProcessCommentBlock()
{
	LexerCursor++;
	Col++;
	while (LexerCursor < EndOfBuffer)
	{
		char CurrChar = *LexerCursor;
		LexerCursor++;
		Col++;
		if (CurrChar == '*' && LexerCursor < EndOfBuffer && (*LexerCursor) == '/')
		{
			LexerCursor++;
			Col++;
			OK;
		}
	}

	if (pErr)
	{
		CString S;
		S.Format("Unexpected end of file while parsing comment block\n");
		pErr->Add(S);
	}

	FAIL;
}
//---------------------------------------------------------------------

void CHRDParser::DlmMatchChar(char Char, int Index, int Start, int End, int& MatchStart, int& MatchEnd)
{
	MatchStart = INVALID_INDEX;
	int i = Start;
	for (; i < End; ++i)
	{
		if ((int)TableDlm[i].GetLength() < Index) continue;

		if (MatchStart == INVALID_INDEX)
		{
			if (TableDlm[i][Index] == Char) MatchStart = i;
		}
		else if (TableDlm[i][Index] != Char) break;
	}
	MatchEnd = i;
}
//---------------------------------------------------------------------

void CHRDParser::SkipSpaces()
{
	LexerCursor++;
	Col++;
	while (LexerCursor < EndOfBuffer)
	{
		char CurrChar = *LexerCursor;
		switch (CurrChar)
		{
			case ' ':
			case '\t':
				LexerCursor++;
				Col++;
				break;
			case '\n':
				LexerCursor++;
				if (LexerCursor < EndOfBuffer && (*LexerCursor) == '\r') LexerCursor++;
				Col = 1;
				Line++;
				break;
			case '\r':
				LexerCursor++;
				if (LexerCursor < EndOfBuffer && (*LexerCursor) == '\n') LexerCursor++;
				Col = 1;
				Line++;
				break;
			default: return;
		}
	}
}
//---------------------------------------------------------------------

void CHRDParser::AddConst(std::vector<CToken>& Tokens, const CString& Const, EType Type)
{
	CData Data;
	switch (Type)
	{
		case T_INT:		Data = StringUtils::ToInt(Const.CStr()); break;
		case T_INT_HEX:	Data = (int)strtoul(Const.CStr(), nullptr, 16); break; //!!!can use 0 base to autodetect radix
		case T_FLOAT:	Data = StringUtils::ToFloat(Const.CStr()); break;
		case T_STRING:	Data = Const; break;
		case T_STRID:	Data = CStrID(Const.CStr()); break;
		default:		Sys::Error("Unknown data type\n");
	}
	
	auto It = std::find(TableConst.cbegin(), TableConst.cend(), Data);
	if (It == TableConst.cend())
	{
		TableConst.push_back(Data);
		It = std::prev(TableConst.cend());
	}
	Tokens.push_back(CToken(TBL_CONST, std::distance(TableConst.cbegin(), It), Line, Col));
}
//---------------------------------------------------------------------

// ROOT = { PARAM }
bool CHRDParser::ParseTokenStream(const std::vector<CToken>& Tokens, CParams& Output)
{
	ParserCursor = 0;
	while (ParserCursor < Tokens.size() && ParseParam(Tokens, Output))
		/*do nothing, or scream with joy if you want :)*/;
	return ParserCursor == Tokens.size();
}
//---------------------------------------------------------------------

// PARAM = ID|CONST_STRID [ '=' ] DATA
bool CHRDParser::ParseParam(const std::vector<CToken>& Tokens, CParams& Output)
{
	CStrID Key;

	const CToken& CurrToken = Tokens[ParserCursor];
	if (CurrToken.Table == TBL_ID)
	{
		Key = CStrID(TableID[CurrToken.Index].CStr());
	}
	else if (CurrToken.Table == TBL_CONST && TableConst[CurrToken.Index].IsA<CStrID>())
	{
		Key = TableConst[CurrToken.Index].GetValue<CStrID>();
	}
	else
	{
		if (pErr)
		{
			CString S;
			S.Format("ID or CStrID constant expected (Ln:%u, Col:%u)\n", CurrToken.Ln, CurrToken.Cl);
			pErr->Add(S);
		}
		FAIL;
	}
	
	if (++ParserCursor < Tokens.size())
	{
		int CursorBackup = ParserCursor;
		if (Tokens[ParserCursor].IsA(TBL_DLM, DLM_EQUAL) && ++ParserCursor >= Tokens.size())
		{
			ParserCursor = CursorBackup;
			if (pErr)
			{
				CString S;
				S.Format("Unexpected end of file after ID in PARAM\n");
				pErr->Add(S);
			}
			FAIL;
		}
		
		//!!!too much CData copyings!
		CData Data;
		if (!ParseData(Tokens, Data))
		{
			ParserCursor = CursorBackup;
			FAIL;
		}

		//!!!can check duplicates here!
		Output.Set(Key, Data);
		OK;
	}
	
	ParserCursor--;
	if (pErr)
	{
		CString S;
		S.Format("Unexpected end of file while parsing PARAM\n");
		pErr->Add(S);
	}
	FAIL;
}
//---------------------------------------------------------------------

// DATA = CONST | ARRAY | SECTION
bool CHRDParser::ParseData(const std::vector<CToken>& Tokens, CData& Output)
{
	const CToken& CurrToken = Tokens[ParserCursor];

	if (CurrToken.Table == TBL_CONST)
	{
		Output = TableConst[CurrToken.Index];
		ParserCursor++;
		OK;
	}
	
	if (CurrToken.IsA(TBL_RW, RW_TRUE))
	{
		Output = true;
		ParserCursor++;
		OK;
	}

	if (CurrToken.IsA(TBL_RW, RW_FALSE))
	{
		Output = false;
		ParserCursor++;
		OK;
	}

	if (CurrToken.IsA(TBL_RW, RW_NULL))
	{
		Output.Clear();
		ParserCursor++;
		OK;
	}

	int CursorBackup = ParserCursor;

	if (ParseVector(Tokens, Output)) OK;

	PDataArray Array(n_new(CDataArray));
	if (ParseArray(Tokens, *Array))
	{
		Output = Array;
		OK;
	}

	ParserCursor = CursorBackup;
	PParams Section(n_new(CParams));
	if (ParseSection(Tokens, *Section))
	{
		Output = Section;
		OK;
	}
	
	Output.Clear();
	ParserCursor = CursorBackup;
	if (pErr)
	{
		CString S;
		S.Format("Parsing of DATA failed (Ln:%u, Col:%u)\n", CurrToken.Ln, CurrToken.Cl);
		pErr->Add(S);
	}

#ifdef _DEBUG
	CString TokenValue;
	if (CurrToken.Table == TBL_RW) TokenValue = TableRW[CurrToken.Index];
	else if (CurrToken.Table == TBL_ID) TokenValue = TableID[CurrToken.Index];
	else if (CurrToken.Table == TBL_DLM) TokenValue = TableDlm[CurrToken.Index];
	else if (TableConst[CurrToken.Index].IsA<CString>())
		TokenValue = TableConst[CurrToken.Index].GetValue<CString>();
	else TokenValue = CString("Non-string (numeric) constant");
	
	if (pErr)
	{
		CString S;
		S.Format("Current token: %s\n", TokenValue.CStr());
		pErr->Add(S);
	}

	if (ParserCursor > 0)
	{
		const auto& PrevToken = Tokens[ParserCursor - 1];
		if (PrevToken.Table == TBL_RW) TokenValue = TableRW[PrevToken.Index];
		else if (PrevToken.Table == TBL_ID) TokenValue = TableID[PrevToken.Index];
		else if (PrevToken.Table == TBL_DLM) TokenValue = TableDlm[PrevToken.Index];
		else if (TableConst[PrevToken.Index].IsA<CString>())
			TokenValue = TableConst[PrevToken.Index].GetValue<CString>();
		else TokenValue = CString("Non-string (numeric) constant");
		
		if (pErr)
		{
			CString S;
			S.Format("Previous token: %s\n", TokenValue.CStr());
			pErr->Add(S);
		}
	}
#endif

	FAIL;
}
//---------------------------------------------------------------------

// ARRAY = '[' [ DATA { ',' DATA } ] ']'
bool CHRDParser::ParseArray(const std::vector<CToken>& Tokens, CDataArray& Output)
{
	if (!Tokens[ParserCursor].IsA(TBL_DLM, DLM_SQ_BR_OPEN)) FAIL;
	
	if (Tokens[ParserCursor + 1].IsA(TBL_DLM, DLM_SQ_BR_CLOSE))
	{
		ParserCursor += 2;
		OK;
	}

	int CursorBackup = ParserCursor;

	while (++ParserCursor < Tokens.size())
	{
		CData Data;
		if (!ParseData(Tokens, Data))
		{
			ParserCursor = CursorBackup;
			FAIL;
		}
		Output.Add(Data);
	
		if (ParserCursor >= Tokens.size()) break;
		
		const CToken& CurrToken = Tokens[ParserCursor];
		if (CurrToken.IsA(TBL_DLM, DLM_COMMA)) continue;
		else if (CurrToken.IsA(TBL_DLM, DLM_SQ_BR_CLOSE))
		{
			ParserCursor++;
			OK;
		}
		else
		{
			ParserCursor = CursorBackup;
			if (pErr)
			{
				CString S;
				S.Format("',' or ']' expected (Ln:%u, Col:%u)\n", CurrToken.Ln, CurrToken.Cl);
				pErr->Add(S);
			}
			FAIL;
		}
	}

	ParserCursor = CursorBackup;
	if (pErr)
	{
		CString S;
		S.Format("Unexpected end of file while parsing ARRAY\n");
		pErr->Add(S);
	}
	FAIL;
}
//---------------------------------------------------------------------

// SECTION = '{' { PARAM } '}'
bool CHRDParser::ParseSection(const std::vector<CToken>& Tokens, CParams& Output)
{
	if (!Tokens[ParserCursor].IsA(TBL_DLM, DLM_CUR_BR_OPEN)) FAIL;
	
	int CursorBackup = ParserCursor;
	
	ParserCursor++;
	while (ParserCursor < Tokens.size())
	{
		const CToken& CurrToken = Tokens[ParserCursor];
		if (CurrToken.IsA(TBL_DLM, DLM_CUR_BR_CLOSE))
		{
			ParserCursor++;
			OK;
		}
		else if (!ParseParam(Tokens, Output))
		{
			ParserCursor = CursorBackup;
			FAIL;
		}
	}

	ParserCursor = CursorBackup;
	if (pErr)
	{
		CString S;
		S.Format("Unexpected end of file while parsing SECTION\n");
		pErr->Add(S);
	}
	FAIL;
}
//---------------------------------------------------------------------

// VECTOR = '(' [ NUMBER { ',' NUMBER } ] ')'
bool CHRDParser::ParseVector(const std::vector<CToken>& Tokens, CData& Output)
{
	if (!Tokens[ParserCursor].IsA(TBL_DLM, DLM_BR_OPEN)) FAIL;

	int CursorBackup = ParserCursor;

	std::vector<float> Floats;

	while (++ParserCursor < Tokens.size())
	{
		{
			const CToken& CurrToken = Tokens[ParserCursor];

			// Parse data and ensure it is number
			CData Data;
			if (!(ParseData(Tokens, Data) && (Data.IsA<int>() || Data.IsA<float>())))
			{
				if (pErr)
				{
					CString S;
					S.Format("Numeric constant expected in vector (Ln:%u, Col:%u)\n", CurrToken.Ln, CurrToken.Cl);
					pErr->Add(S);
				}
				ParserCursor = CursorBackup;
				FAIL;
			}
			Floats.push_back(Data.IsA<int>() ? (float)Data.GetValue<int>() : Data.GetValue<float>());

			if (ParserCursor >= Tokens.size()) break;
		}
		
		const CToken& CurrToken = Tokens[ParserCursor];
		if (CurrToken.IsA(TBL_DLM, DLM_COMMA)) continue;
		else if (CurrToken.IsA(TBL_DLM, DLM_BR_CLOSE))
		{
			switch (Floats.size())
			{
				//case 2:
				//	Output = vector2(Floats[0], Floats[1]);
				//	break;

				case 3:
					Output = vector3(Floats[0], Floats[1], Floats[2]);
					break;
				case 4:
					Output = vector4(Floats[0], Floats[1], Floats[2], Floats[3]);
					break;

				//case 9:
				//	Output = matrix33(	Floats[0], Floats[1], Floats[2],
				//						Floats[3], Floats[4], Floats[5],
				//						Floats[6], Floats[7], Floats[8]);
				//	break;

				case 16:
					Output = matrix44(	Floats[0], Floats[1], Floats[2], Floats[3],
										Floats[4], Floats[5], Floats[6], Floats[7],
										Floats[8], Floats[9], Floats[10], Floats[11],
										Floats[12], Floats[13], Floats[14], Floats[15]);
					break;
				
				default:
					ParserCursor = CursorBackup;
					if (pErr)
					{
						CString S;
						S.Format("Unexpected vector element count: %d (Ln:%u, Col:%u)\n", Floats.size(), CurrToken.Ln, CurrToken.Cl);
						pErr->Add(S);
					}
					FAIL;
			}

			ParserCursor++;
			OK;
		}
		else
		{
			ParserCursor = CursorBackup;
			if (pErr)
			{
				CString S;
				S.Format("',' or ']' expected (Ln:%u, Col:%u)\n", CurrToken.Ln, CurrToken.Cl);
				pErr->Add(S);
			}
			FAIL;
		}
	}

	ParserCursor = CursorBackup;
	if (pErr)
	{
		CString S;
		S.Format("Unexpected end of file while parsing VECTOR\n");
		pErr->Add(S);
	}
	FAIL;
}
//---------------------------------------------------------------------

}
