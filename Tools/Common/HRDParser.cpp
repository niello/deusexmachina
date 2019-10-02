#include "HRDParser.h"
#include <StringID.h>
#include <algorithm>
#include <sstream>
#include <ctype.h>

namespace Data
{
enum
{
	RW_FALSE = 0,
	RW_NULL = 1,
	RW_TRUE = 2
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

CHRDParser::CHRDParser()
{
	// NB: Keep sorted and keep enum above updated
	TableRW.assign({ "false", "null", "true" });
	TableDlm.assign({ "=", "[", ",", "]", "{", "}", "(", ")" });
}
//---------------------------------------------------------------------

bool CHRDParser::ParseBuffer(const char* Buffer, size_t Length, CParams& Result, std::ostringstream* pErrors)
{
	if (!Buffer) return false;

	ZeroIdx = -1;

	// Allow empty file to be parsed
	if (!Length) return true;

	pErr = pErrors;
	
	// Check for UTF-8 BOM signature (0xEF 0xBB 0xBF)
	if (Length >= 3 && Buffer[0] == (char)0xEF && Buffer[1] == (char)0xBB && Buffer[2] == (char)0xBF)
		LexerCursor = Buffer + 3;
	else LexerCursor = Buffer;
	
	EndOfBuffer = Buffer + Length;

	if (LexerCursor == EndOfBuffer) return false;

	Line = Col = 1;

	std::vector<CToken> Tokens;
	if (!Tokenize(Tokens))
	{
		if (pErr) *pErr << "Lexical analysis of HRD failed\n";
		TableID.clear();
		TableConst.clear(); //???always keep "0" const?
		//Tokens.Clear();
		pErr = nullptr;
		return false;
	}
	
	if (!ParseTokenStream(Tokens, Result))
	{
		if (pErr) *pErr << "Syntax analysis of HRD failed\n";
		Result.clear();
		TableID.clear();
		TableConst.clear(); //???always keep "0" const?
		//Tokens.Clear();
		pErr = nullptr;
		return false;
	}
	
	TableID.clear();
	TableConst.clear(); //???always keep "0" const?
	//Tokens.clear();
	pErr = nullptr;

	return true;
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
					if (!LexProcessHex(Tokens)) return false;
					continue;
				}
				else if (CurrChar == '.')
				{
					if (!LexProcessFloat(Tokens)) return false;
					continue;
				}
				else if (isdigit(CurrChar))
				{
					if (!LexProcessNumeric(Tokens)) return false;
					continue;
				}
			}

			//???add zero const at parsing start?
			if (ZeroIdx == -1)
			{
				ZeroIdx = TableConst.size();
				TableConst.push_back((int)0);
			}
			Tokens.push_back(CToken(TBL_CONST, ZeroIdx, Line, Col));
		}
		else if (CurrChar == '.')
		{
			LexerCursor++;
			Col++;
			if (LexerCursor < EndOfBuffer && isdigit(*LexerCursor))
			{
				if (!LexProcessFloat(Tokens)) return false;
				continue;
			}
			if (!LexProcessDlm(Tokens)) return false; //???cursor 1 char back?
		}
		else if (CurrChar == '"' || CurrChar == '\'')
		{
			if (!LexProcessString(Tokens, CurrChar)) return false;
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
					if (!LexProcessBigString(Tokens)) return false;
					continue;
				}
			}
			if (!LexProcessDlm(Tokens)) return false; //???cursor 1 char back?
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
					if (!LexProcessCommentLine()) return false;
					continue;
				}
				else if (CurrChar == '*')
				{
					if (!LexProcessCommentBlock()) return false;
					continue;
				}
			}
			if (!LexProcessDlm(Tokens)) return false; //???cursor 1 char back?
		}
		else if (CurrChar == '-' || isdigit(CurrChar))
		{
			if (!LexProcessNumeric(Tokens)) return false;
		}
		else if (isalpha(CurrChar) || CurrChar == '_')
		{
			if (!LexProcessID(Tokens)) return false;
		}
		else if (CurrChar == ' ' || CurrChar == '\t' || CurrChar == '\n' || CurrChar == '\r')
			SkipSpaces();
		else if (!LexProcessDlm(Tokens)) return false;
	}
	
	return true;
}
//---------------------------------------------------------------------

bool CHRDParser::LexProcessID(std::vector<CToken>& Tokens)
{
	bool HasLetter = ((*LexerCursor) != '_'); //isalpha(*LexerCursor);
	LexerCursor++;
	Col++;
	while (LexerCursor < EndOfBuffer)
	{
		char CurrChar = *LexerCursor;
		if (isalpha(CurrChar))
		{
			LexerCursor++;
			Col++;
			HasLetter = true;
		}
		else if (isdigit(CurrChar) || CurrChar == '_')
		{
			LexerCursor++;
			Col++;
		}
		else break;
	}

	if (!HasLetter)
	{
		if (pErr)
			*pErr << "'_' allowed only in IDs, but ID should contain at least one letter (Ln:" << Line << ", Col:" << Col << ")\n";
		return false;
	}
	
	std::string NewID;
	NewID.assign(TokenStart, LexerCursor - TokenStart);

	{
		auto It = std::find(TableRW.cbegin(), TableRW.cend(), NewID);
		if (It != TableRW.cend())
		{
			Tokens.push_back(CToken(TBL_RW, std::distance(TableRW.cbegin(), It), Line, Col));
			return true;
		}
	}

	int Idx;
	auto It = std::find(TableID.cbegin(), TableID.cend(), NewID);
	if (It == TableID.cend())
	{
		Idx = TableID.size();
		TableID.push_back(NewID);
	}
	else Idx = std::distance(TableID.cbegin(), It);

	Tokens.push_back(CToken(TBL_ID, Idx, Line, Col));

	return true;
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
			*pErr << "Hexadecimal constant should contain at least one hex digit after '0x' (Ln:" << Line << ", Col:" << Col << ")\n";
		return false;
	}
	
	std::string Token;
	Token.assign(TokenStart, TokenLength);
	AddConst(Tokens, Token, T_INT_HEX);
	return true;
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
		
	std::string Token;
	Token.assign(TokenStart, LexerCursor - TokenStart);
	AddConst(Tokens, Token, T_FLOAT);
	return true;
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

	std::string Token;
	Token.assign(TokenStart, LexerCursor - TokenStart);
	AddConst(Tokens, Token, T_INT);
	return true;
}
//---------------------------------------------------------------------

bool CHRDParser::LexProcessString(std::vector<CToken>& Tokens, char QuoteChar)
{
	std::string NewConst;
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
					case 't': NewConst += "\t"; break;	//!!!NEED std::string += char!
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
							//!!!NEED std::string += char!
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
			return true;
		}
		else
		{
			//!!!NEED std::string += char! or better pre-allocate+grow & simply set characters by NewConst[Len] = c
			char Tmp[2];
			Tmp[0] = CurrChar;
			Tmp[1] = 0;
			NewConst += Tmp;
			LexerCursor++;
			Col++;
		}
	}
	
	if (pErr)
		*pErr << "Unexpected end of file while parsing string constant (Ln:" << Line << ", Col:" << Col << ")\n";

	return false;
}
//---------------------------------------------------------------------

// <" STRING CONTENTS ">
bool CHRDParser::LexProcessBigString(std::vector<CToken>& Tokens)
{
	std::string NewConst; //!!!preallocate and grow like array!
	
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
					return true;
				}
				else
				{
					NewConst += "\"";
					continue;
				}
			}
			else break;
		}

		//!!!NEED std::string += char! or better pre-allocate+grow & simply set characters by NewConst[Len] = c
		char Tmp[2];
		Tmp[0] = CurrChar;
		Tmp[1] = 0;
		NewConst += Tmp;
		LexerCursor++;
		Col++;
	}
	
	if (pErr)
		*pErr << "Unexpected end of file while parsing big string constant (Ln:" << Line << ", Col:" << Col << ")\n";

	return false;
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
		if (MatchStart != -1)
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
		
		if (MatchStart == -1)
		{
			if (pErr)
				*pErr << "Unknown delimiter (Ln:" << Line << ", Col:" << Col << ")\n";
			return false;
		}

		assert(MatchStart + 1 == MatchEnd); // Assert there are no duplicates
		Tokens.push_back(CToken(TBL_DLM, MatchStart, Line, Col));
		return true;
	}
	
	// No matching dlm at all, invalid character in stream
	if (pErr)
		*pErr << "Invalid character '" << CurrChar << "' (" << static_cast<uint32_t>(CurrChar) << ") (Ln:" << Line << ", Col:" << Col << ")\n";

	return false;
}
//---------------------------------------------------------------------

bool CHRDParser::LexProcessCommentLine()
{
	LexerCursor++;
	Col++;
	while (LexerCursor < EndOfBuffer)
	{
		char CurrChar = *LexerCursor;
		if (CurrChar == '\n' || CurrChar == '\r') return true;
		else
		{
			LexerCursor++;
			Col++;
		}
	}
	
	return true;
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
			return true;
		}
	}

	if (pErr)
		*pErr << "Unexpected end of file while parsing comment block\n";

	return false;
}
//---------------------------------------------------------------------

void CHRDParser::DlmMatchChar(char Char, int Index, int Start, int End, int& MatchStart, int& MatchEnd)
{
	MatchStart = -1;
	int i = Start;
	for (; i < End; ++i)
	{
		if ((int)TableDlm[i].size() < Index) continue;

		if (MatchStart == -1)
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

void CHRDParser::AddConst(std::vector<CToken>& Tokens, const std::string& Const, EType Type)
{
	CData Data;
	switch (Type)
	{
		case T_INT:		Data = std::atoi(Const.c_str()); break;
		case T_INT_HEX:	Data = static_cast<int>(strtoul(Const.c_str(), nullptr, 16)); break; //!!!can use 0 base to autodetect radix
		case T_FLOAT:	Data = static_cast<float>(std::atof(Const.c_str())); break;
		case T_STRING:	Data = Const; break;
		case T_STRID:	Data = CStrID(Const.c_str()); break;
		default:		assert(false && "Unknown data type\n");
	}

	int Idx;
	auto It = std::find(TableConst.cbegin(), TableConst.cend(), Data);
	if (It == TableConst.cend())
	{
		Idx = TableConst.size();
		TableConst.push_back(Data);
	}
	else Idx = std::distance(TableConst.cbegin(), It);
	Tokens.push_back(CToken(TBL_CONST, Idx, Line, Col));
}
//---------------------------------------------------------------------

// ROOT = { PARAM }
bool CHRDParser::ParseTokenStream(const std::vector<CToken>& Tokens, CParams& Output)
{
	ParserCursor = 0;
	while (ParserCursor < Tokens.size() && ParseParam(Tokens, Output))
		/*do nothing*/;
	return ParserCursor == Tokens.size();
}
//---------------------------------------------------------------------

// PARAM = ID [ '=' ] DATA
bool CHRDParser::ParseParam(const std::vector<CToken>& Tokens, CParams& Output)
{
	const CToken& CurrToken = Tokens[ParserCursor];
	if (CurrToken.Table != TBL_ID)
	{
		if (pErr)
			*pErr << "ID expected (Ln:" << CurrToken.Ln << ", Col:" << CurrToken.Cl << ")\n";
		return false;
	}
	
	if (++ParserCursor < Tokens.size())
	{
		int CursorBackup = ParserCursor;
		if (Tokens[ParserCursor].IsA(TBL_DLM, DLM_EQUAL) && ++ParserCursor >= Tokens.size())
		{
			ParserCursor = CursorBackup;
			if (pErr)
				*pErr << "Unexpected end of file after ID in PARAM\n";
			return false;
		}
		
		//!!!too much CData copyings!
		CData Data;
		if (!ParseData(Tokens, Data))
		{
			ParserCursor = CursorBackup;
			return false;
		}

		//!!!can check duplicates here!
		Output.emplace(CStrID(TableID[CurrToken.Index].c_str()), std::move(Data));
		return true;
	}
	
	ParserCursor--;
	if (pErr)
		*pErr << "Unexpected end of file while parsing PARAM\n";
	return false;
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
		return true;
	}
	
	if (CurrToken.IsA(TBL_RW, RW_TRUE))
	{
		Output = true;
		ParserCursor++;
		return true;
	}

	if (CurrToken.IsA(TBL_RW, RW_FALSE))
	{
		Output = false;
		ParserCursor++;
		return true;
	}

	if (CurrToken.IsA(TBL_RW, RW_NULL))
	{
		Output.Clear();
		ParserCursor++;
		return true;
	}

	int CursorBackup = ParserCursor;

	if (ParseVector(Tokens, Output)) return true;

	CDataArray Array;
	if (ParseArray(Tokens, Array))
	{
		Output = std::move(Array);
		return true;
	}

	ParserCursor = CursorBackup;
	CParams Params;
	if (ParseSection(Tokens, Params))
	{
		Output = std::move(Params);
		return true;
	}
	
	Output.Clear();
	ParserCursor = CursorBackup;
	if (pErr)
		*pErr << "Parsing of DATA failed (Ln:" << CurrToken.Ln << ", Col:" << CurrToken.Cl << ")\n";

	return false;
}
//---------------------------------------------------------------------

// ARRAY = '[' [ DATA { ',' DATA } ] ']'
bool CHRDParser::ParseArray(const std::vector<CToken>& Tokens, CDataArray& Output)
{
	if (!Tokens[ParserCursor].IsA(TBL_DLM, DLM_SQ_BR_OPEN)) return false;
	
	if (Tokens[ParserCursor + 1].IsA(TBL_DLM, DLM_SQ_BR_CLOSE))
	{
		ParserCursor += 2;
		return true;
	}

	int CursorBackup = ParserCursor;

	while (++ParserCursor < Tokens.size())
	{
		CData Data;
		if (!ParseData(Tokens, Data))
		{
			ParserCursor = CursorBackup;
			return false;
		}
		Output.push_back(Data);
	
		if (ParserCursor >= Tokens.size()) break;
		
		const CToken& CurrToken = Tokens[ParserCursor];
		if (CurrToken.IsA(TBL_DLM, DLM_COMMA)) continue;
		else if (CurrToken.IsA(TBL_DLM, DLM_SQ_BR_CLOSE))
		{
			ParserCursor++;
			return true;
		}
		else
		{
			ParserCursor = CursorBackup;
			if (pErr)
				*pErr << "',' or ']' expected (Ln:" << CurrToken.Ln << ", Col:" << CurrToken.Cl << ")\n";
			return false;
		}
	}

	ParserCursor = CursorBackup;
	if (pErr)
		*pErr << "Unexpected end of file while parsing ARRAY\n";
	return false;
}
//---------------------------------------------------------------------

// SECTION = '{' { PARAM } '}'
bool CHRDParser::ParseSection(const std::vector<CToken>& Tokens, CParams& Output)
{
	if (!Tokens[ParserCursor].IsA(TBL_DLM, DLM_CUR_BR_OPEN)) return false;
	
	int CursorBackup = ParserCursor;
	
	ParserCursor++;
	while (ParserCursor < Tokens.size())
	{
		const CToken& CurrToken = Tokens[ParserCursor];
		if (CurrToken.IsA(TBL_DLM, DLM_CUR_BR_CLOSE))
		{
			ParserCursor++;
			return true;
		}
		else if (!ParseParam(Tokens, Output))
		{
			ParserCursor = CursorBackup;
			return false;
		}
	}

	ParserCursor = CursorBackup;
	if (pErr)
		*pErr << "Unexpected end of file while parsing SECTION\n";

	return false;
}
//---------------------------------------------------------------------

// VECTOR = '(' [ NUMBER { ',' NUMBER } ] ')'
bool CHRDParser::ParseVector(const std::vector<CToken>& Tokens, CData& Output)
{
	if (!Tokens[ParserCursor].IsA(TBL_DLM, DLM_BR_OPEN)) return false;

	int CursorBackup = ParserCursor;

	std::vector<float> Floats;

	while (++ParserCursor < Tokens.size())
	{
		const CToken* pCurrToken = &Tokens[ParserCursor];

		// Parse data and ensure it is number
		CData Data;
		if (!(ParseData(Tokens, Data) && (Data.IsA<int>() || Data.IsA<float>())))
		{
			if (pErr)
				*pErr << "Numeric constant expected in vector (Ln:" << pCurrToken->Ln << ", Col:" << pCurrToken->Cl << ")\n";
			ParserCursor = CursorBackup;
			return false;
		}
		Floats.push_back(Data.IsA<int>() ? (float)Data.GetValue<int>() : Data.GetValue<float>());
	
		if (ParserCursor >= Tokens.size()) break;
		
		pCurrToken = &Tokens[ParserCursor];
		if (pCurrToken->IsA(TBL_DLM, DLM_COMMA)) continue;
		else if (pCurrToken->IsA(TBL_DLM, DLM_BR_CLOSE))
		{
			switch (Floats.size())
			{
				case 3:
					Output = vector4{ Floats[0], Floats[1], Floats[2], 0.f };
					break;
				case 4:
					Output = vector4{ Floats[0], Floats[1], Floats[2], Floats[3] };
					break;

				//case 16:
				//	Output = matrix44(	Floats[0], Floats[1], Floats[2], Floats[3],
				//						Floats[4], Floats[5], Floats[6], Floats[7],
				//						Floats[8], Floats[9], Floats[10], Floats[11],
				//						Floats[12], Floats[13], Floats[14], Floats[15]);
				//	break;
				
				default:
					ParserCursor = CursorBackup;
					if (pErr)
						*pErr << "Unexpected vector element count: " << Floats.size() << " (Ln:" << pCurrToken->Ln << ", Col:" << pCurrToken->Cl << ")\n";
					return false;
			}

			ParserCursor++;
			return true;
		}
		else
		{
			ParserCursor = CursorBackup;
			if (pErr)
				*pErr << "',' or ')' expected (Ln:" << pCurrToken->Ln << ", Col:" << pCurrToken->Cl << ")\n";
			return false;
		}
	}

	ParserCursor = CursorBackup;
	if (pErr)
		*pErr << "Unexpected end of file while parsing VECTOR\n";

	return false;
}
//---------------------------------------------------------------------

}