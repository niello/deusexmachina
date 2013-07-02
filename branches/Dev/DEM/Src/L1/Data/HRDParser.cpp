#include "HRDParser.h"

#include "DataArray.h"
#include "Params.h"

namespace Data
{
__ImplementClassNoFactory(Data::CHRDParser, Core::CRefCounted);
	
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

CHRDParser::CHRDParser()
{
	// NB: Keep sorted and keep enum above updated
	TableRW.Append("false");
	TableRW.Append("null");
	TableRW.Append("true");
	
	TableDlm.Append("=");
	TableDlm.Append("[");
	TableDlm.Append(",");
	TableDlm.Append("]");
	TableDlm.Append("{");
	TableDlm.Append("}");
	TableDlm.Append("(");
	TableDlm.Append(")");
}
//---------------------------------------------------------------------

bool CHRDParser::ParseBuffer(LPCSTR Buffer, DWORD Length, PParams& Result)
{
	if (!Buffer) FAIL;

	ZeroIdx = INVALID_INDEX;

	// Allow empty file to be parsed
	if (!Length)
	{
		Result = n_new(CParams);
		OK;
	}
	
	// Check for UTF-8 BOM signature (0xEF 0xBB 0xBF)
	if (Length >= 3 && Buffer[0] == (char)0xEF && Buffer[1] == (char)0xBB && Buffer[2] == (char)0xBF)
		LexerCursor = Buffer + 3;
	else LexerCursor = Buffer;
	
	EndOfBuffer = Buffer + Length;

	if (LexerCursor == EndOfBuffer) FAIL;

	Line = Col = 1;

	nArray<CToken> Tokens;
	if (!Tokenize(Tokens))
	{
		n_printf("Lexical analysis of HRD failed\n");
		TableID.Clear();
		TableConst.Clear(); //???always keep "0" const?
		FAIL;
	}
	
	if (!Result.IsValid()) Result = n_new(CParams);
	if (!ParseTokenStream(Tokens, Result))
	{
		n_printf("Syntax analysis of HRD failed\n");
		Result = NULL;
		TableID.Clear();
		TableConst.Clear(); //???always keep "0" const?
		//Tokens.Clear();
		FAIL;
	}
	
	TableID.Clear();
	TableConst.Clear(); //???always keep "0" const?
	//Tokens.Clear();

	OK;
}
//---------------------------------------------------------------------

bool CHRDParser::Tokenize(nArray<CToken>& Tokens)
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
				ZeroIdx = TableConst.GetCount();
				TableConst.Append((int)0); //!!!preallocate & set inplace!
			}
			Tokens.Append(CToken(TBL_CONST, ZeroIdx, Line, Col));
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

bool CHRDParser::LexProcessID(nArray<CToken>& Tokens)
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
		n_printf("'_' allowed only in IDs, but ID should contain at least one letter (Ln:%u, Col:%u)\n",
			Line, Col);
		FAIL;
	}
	
	nString NewID;
	NewID.Set(TokenStart, LexerCursor - TokenStart);
	
	int Idx = TableRW.BinarySearchIndex(NewID);
	if (Idx != INVALID_INDEX)
	{
		Tokens.Append(CToken(TBL_RW, Idx, Line, Col));
		OK;
	}
	
	Idx = TableID.FindIndex(NewID);
	if (Idx == INVALID_INDEX)
	{
		Idx = TableID.GetCount();
		TableID.Append(NewID);
	}
	Tokens.Append(CToken(TBL_ID, Idx, Line, Col));
	OK;
}
//---------------------------------------------------------------------

bool CHRDParser::LexProcessHex(nArray<CToken>& Tokens)
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
		n_printf("Hexadecimal constant should contain at least one hex digit after '0x' (Ln:%u, Col:%u)\n",
			Line, Col);
		FAIL;
	}
	
	nString Token;
	Token.Set(TokenStart, TokenLength);
	AddConst(Tokens, Token, T_INT_HEX);
	OK;
}
//---------------------------------------------------------------------

bool CHRDParser::LexProcessFloat(nArray<CToken>& Tokens)
{
	// Dot and at least one digit were read

	do
	{
		LexerCursor++;
		Col++;
	}
	while (LexerCursor < EndOfBuffer && isdigit(*LexerCursor));
		
	nString Token;
	Token.Set(TokenStart, LexerCursor - TokenStart);
	AddConst(Tokens, Token, T_FLOAT);
	OK;
}
//---------------------------------------------------------------------

bool CHRDParser::LexProcessNumeric(nArray<CToken>& Tokens)
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

	nString Token;
	Token.Set(TokenStart, LexerCursor - TokenStart);
	AddConst(Tokens, Token, T_INT);
	OK;
}
//---------------------------------------------------------------------

bool CHRDParser::LexProcessString(nArray<CToken>& Tokens, char QuoteChar)
{
	nString NewConst; //!!!preallocate and grow like array!
	
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
					case 't': NewConst += "\t"; break;	//!!!NEED nString += char!
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
							//!!!NEED nString += char!
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
			//!!!NEED nString += char! or better pre-allocate+grow & simply set characters by NewConst[Len] = c
			char Tmp[2];
			Tmp[0] = CurrChar;
			Tmp[1] = 0;
			NewConst += Tmp;
			LexerCursor++;
			Col++;
		}
	}
	
	n_printf("Unexpected end of file while parsing string constant (Ln:%u, Col:%u)\n", Line, Col);
	FAIL;
}
//---------------------------------------------------------------------

// <" STRING CONTENTS ">
bool CHRDParser::LexProcessBigString(nArray<CToken>& Tokens)
{
	nString NewConst; //!!!preallocate and grow like array!
	
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

		//!!!NEED nString += char! or better pre-allocate+grow & simply set characters by NewConst[Len] = c
		char Tmp[2];
		Tmp[0] = CurrChar;
		Tmp[1] = 0;
		NewConst += Tmp;
		LexerCursor++;
		Col++;
	}
	
	n_printf("Unexpected end of file while parsing big string constant (Ln:%u, Col:%u)\n", Line, Col);
	FAIL;
}
//---------------------------------------------------------------------

bool CHRDParser::LexProcessDlm(nArray<CToken>& Tokens)
{
	int	Start = 0,
		End = TableDlm.GetCount(),
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
			n_printf("Unknown delimiter (Ln:%u, Col:%u)\n", Line, Col);
			FAIL;
		}

		n_assert(MatchStart + 1 == MatchEnd); // Assert there are no duplicates
		Tokens.Append(CToken(TBL_DLM, MatchStart, Line, Col));
		OK;
	}
	
	// No matching dlm at all, invalid character in stream
	n_printf("Invalid character '%c' (%u) (Ln:%u, Col:%u)\n", CurrChar, CurrChar, Line, Col);
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
	
	n_printf("Unexpected end of file while parsing comment block\n");
	FAIL;
}
//---------------------------------------------------------------------

void CHRDParser::DlmMatchChar(char Char, int Index, int Start, int End, int& MatchStart, int& MatchEnd)
{
	MatchStart = INVALID_INDEX;
	int i = Start;
	for (; i < End; i++)
	{
		if (TableDlm[i].Length() < Index) continue;

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

void CHRDParser::AddConst(nArray<CToken>& Tokens, const nString& Const, EType Type)
{
	CData Data;
	switch (Type)
	{
		case T_INT:		Data = Const.AsInt(); break;
		case T_INT_HEX:	Data = (int)strtoul(Const.CStr(), NULL, 16); break; //!!!can use 0 base to autodetect radix
		case T_FLOAT:	Data = Const.AsFloat(); break;
		case T_STRING:	Data = Const; break;
		case T_STRID:	Data = CStrID(Const.CStr()); break;
		default:		n_error("Unknown data type\n");
	}
	
	int Idx = TableConst.FindIndex(Data);
	if (Idx == INVALID_INDEX)
	{
		Idx = TableConst.GetCount();
		TableConst.Append(Data);
	}
	Tokens.Append(CToken(TBL_CONST, Idx, Line, Col));
}
//---------------------------------------------------------------------

// ROOT = { PARAM }
bool CHRDParser::ParseTokenStream(const nArray<CToken>& Tokens, PParams Output)
{
	ParserCursor = 0;
	while (ParserCursor < Tokens.GetCount() && ParseParam(Tokens, Output))
		/*do nothing, or scream with joy if you want :)*/;
	return ParserCursor == Tokens.GetCount();
}
//---------------------------------------------------------------------

// PARAM = ID [ '=' ] DATA
bool CHRDParser::ParseParam(const nArray<CToken>& Tokens, PParams Output)
{
	CToken& CurrToken = Tokens[ParserCursor];
	if (CurrToken.Table != TBL_ID)
	{
		n_printf("ID expected (Ln:%u, Col:%u)\n", CurrToken.Ln, CurrToken.Cl);
		FAIL;
	}
	
	if (++ParserCursor < Tokens.GetCount())
	{
		int CursorBackup = ParserCursor;
		if (Tokens[ParserCursor].IsA(TBL_DLM, DLM_EQUAL) && ++ParserCursor >= Tokens.GetCount())
		{
			ParserCursor = CursorBackup;
			n_printf("Unexpected end of file after ID in PARAM\n");
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
		Output->Set(CStrID(TableID[CurrToken.Index].CStr()), Data);
		OK;
	}
	
	ParserCursor--;
	n_printf("Unexpected end of file while parsing PARAM\n");
	FAIL;
}
//---------------------------------------------------------------------

// DATA = CONST | ARRAY | SECTION
bool CHRDParser::ParseData(const nArray<CToken>& Tokens, CData& Output)
{
	CToken& CurrToken = Tokens[ParserCursor];

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

	Output = PDataArray(n_new(CDataArray));
	if (ParseArray(Tokens, Output)) OK;

	ParserCursor = CursorBackup;
	Output = PParams(n_new(CParams));
	if (ParseSection(Tokens, Output)) OK;
	
	Output.Clear();
	ParserCursor = CursorBackup;
	n_printf("Parsing of DATA failed (Ln:%u, Col:%u)\n", CurrToken.Ln, CurrToken.Cl);

#ifdef _DEBUG
	nString TokenValue;
	if (CurrToken.Table == TBL_RW) TokenValue = TableRW[CurrToken.Index];
	else if (CurrToken.Table == TBL_ID) TokenValue = TableID[CurrToken.Index];
	else if (CurrToken.Table == TBL_DLM) TokenValue = TableDlm[CurrToken.Index];
	else if (TableConst[CurrToken.Index].IsA<nString>())
		TokenValue = TableConst[CurrToken.Index].GetValue<nString>();
	else TokenValue = nString("Non-string (numeric) constant");
	n_printf("Current token: %s\n", TokenValue.CStr());
	if (ParserCursor > 0)
	{
		CurrToken = Tokens[ParserCursor - 1];
		if (CurrToken.Table == TBL_RW) TokenValue = TableRW[CurrToken.Index];
		else if (CurrToken.Table == TBL_ID) TokenValue = TableID[CurrToken.Index];
		else if (CurrToken.Table == TBL_DLM) TokenValue = TableDlm[CurrToken.Index];
		else if (TableConst[CurrToken.Index].IsA<nString>())
			TokenValue = TableConst[CurrToken.Index].GetValue<nString>();
		else TokenValue = nString("Non-string (numeric) constant");
		n_printf("Previous token: %s\n", TokenValue.CStr());
	}
#endif

	FAIL;
}
//---------------------------------------------------------------------

// ARRAY = '[' [ DATA { ',' DATA } ] ']'
bool CHRDParser::ParseArray(const nArray<CToken>& Tokens, Ptr<CDataArray> Output)
{
	if (!Tokens[ParserCursor].IsA(TBL_DLM, DLM_SQ_BR_OPEN)) FAIL;
	
	if (Tokens[ParserCursor + 1].IsA(TBL_DLM, DLM_SQ_BR_CLOSE))
	{
		ParserCursor += 2;
		OK;
	}

	int CursorBackup = ParserCursor;

	while (++ParserCursor < Tokens.GetCount())
	{
		CData Data;
		if (!ParseData(Tokens, Data))
		{
			ParserCursor = CursorBackup;
			FAIL;
		}
		Output->Append(Data);
	
		if (ParserCursor >= Tokens.GetCount()) break;
		
		CToken& CurrToken = Tokens[ParserCursor];
		if (CurrToken.IsA(TBL_DLM, DLM_COMMA)) continue;
		else if (CurrToken.IsA(TBL_DLM, DLM_SQ_BR_CLOSE))
		{
			ParserCursor++;
			OK;
		}
		else
		{
			ParserCursor = CursorBackup;
			n_printf("',' or ']' expected (Ln:%u, Col:%u)\n", CurrToken.Ln, CurrToken.Cl);
			FAIL;
		}
	}

	ParserCursor = CursorBackup;
	n_printf("Unexpected end of file while parsing ARRAY\n");
	FAIL;
}
//---------------------------------------------------------------------

// SECTION = '{' { PARAM } '}'
bool CHRDParser::ParseSection(const nArray<CToken>& Tokens, PParams Output)
{
	if (!Tokens[ParserCursor].IsA(TBL_DLM, DLM_CUR_BR_OPEN)) FAIL;
	
	int CursorBackup = ParserCursor;
	
	ParserCursor++;
	while (ParserCursor < Tokens.GetCount())
	{
		CToken& CurrToken = Tokens[ParserCursor];
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
	n_printf("Unexpected end of file while parsing SECTION\n");
	FAIL;
}
//---------------------------------------------------------------------

// VECTOR = '(' [ NUMBER { ',' NUMBER } ] ')'
bool CHRDParser::ParseVector(const nArray<CToken>& Tokens, CData& Output)
{
	if (!Tokens[ParserCursor].IsA(TBL_DLM, DLM_BR_OPEN)) FAIL;

	int CursorBackup = ParserCursor;

	nArray<float> Floats;

	while (++ParserCursor < Tokens.GetCount())
	{
		CToken& CurrToken = Tokens[ParserCursor];

		// Parse data and ensure it is number
		CData Data;
		if (!(ParseData(Tokens, Data) && (Data.IsA<int>() || Data.IsA<float>())))
		{
			n_printf("Numeric constant expected in vector (Ln:%u, Col:%u)\n", CurrToken.Ln, CurrToken.Cl);
			ParserCursor = CursorBackup;
			FAIL;
		}
		Floats.Append(Data.IsA<int>() ? (float)Data.GetValue<int>() : Data.GetValue<float>());
	
		if (ParserCursor >= Tokens.GetCount()) break;
		
		CurrToken = Tokens[ParserCursor];
		if (CurrToken.IsA(TBL_DLM, DLM_COMMA)) continue;
		else if (CurrToken.IsA(TBL_DLM, DLM_BR_CLOSE))
		{
			switch (Floats.GetCount())
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
					n_printf("Unexpected vector element count: %d (Ln:%u, Col:%u)\n", Floats.GetCount(), CurrToken.Ln, CurrToken.Cl);
					FAIL;
			}

			ParserCursor++;
			OK;
		}
		else
		{
			ParserCursor = CursorBackup;
			n_printf("',' or ']' expected (Ln:%u, Col:%u)\n", CurrToken.Ln, CurrToken.Cl);
			FAIL;
		}
	}

	ParserCursor = CursorBackup;
	n_printf("Unexpected end of file while parsing VECTOR\n");
	FAIL;
}
//---------------------------------------------------------------------

}