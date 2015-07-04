#include "VertexLayout.h"

namespace Render
{
__ImplementClassNoFactory(Render::CVertexLayout, Core::CObject);

LPCSTR CVertexComponent::SemanticNames[] =
{
	"Pos",
	"Nrm",
	"Tgt",
	"Bnm",
	"Tex",        
	"Clr",
	"Bwh",
	"Bix"
};

LPCSTR CVertexComponent::FormatNames[] =
{
	"F",
	"F2",
	"F3",
	"F4",
	"UB4",        
	"S2",        
	"S4",        
	"UB4N",
	"S2N",
	"S4N"
};

static LPCSTR IndexStrings[] =
{
	"0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
	"10", "11", "12", "13", "14", "15", "16", "17", "18", "19",
	"20", "21", "22", "23", "24", "25", "26", "27", "28", "29",
	"30", "31"
};

CStrID CVertexLayout::BuildSignature(const CVertexComponent* pComponents, DWORD Count)
{
	if (!pComponents || !Count) return CStrID::Empty;

	// Provide buffer large enough to store any reasonable layout ID
	//???to some wrapper class like a string tokenizer? May write StringBuilder!
	const DWORD BUFFER_SIZE = 1024;
	char pBuf[BUFFER_SIZE];
	char* pCurr = pBuf;
	char* pEnd = pBuf + BUFFER_SIZE - 1; // Store 1 guaranteed byte for a terminating 0
	for (DWORD i = 0; i < Count; ++i)
	{
		const CVertexComponent& Cmp = pComponents[i];

		const char* pSrc = Cmp.GetSemanticString();
		DWORD Len = strlen(pSrc);
		if (memcpy_s(pCurr, pEnd - pCurr, pSrc, Len) != 0) break;
		pCurr += Len;

		DWORD CmpIdx = Cmp.Index;
		if (CmpIdx > 0)
		{
			n_assert_dbg(CmpIdx < sizeof_array(IndexStrings));
			pSrc = IndexStrings[CmpIdx];
			Len = strlen(pSrc);
			if (memcpy_s(pCurr, pEnd - pCurr, pSrc, Len) != 0) break;
			pCurr += Len;
		}

		pSrc = Cmp.GetFormatString();
		Len = strlen(pSrc);
		if (memcpy_s(pCurr, pEnd - pCurr, pSrc, Len) != 0) break;
		pCurr += Len;

		DWORD CmpStream = Cmp.Stream;
		if (CmpStream > 0)
		{
			if (memcpy_s(pCurr, pEnd - pCurr, "s", 1) != 0) break;
			++pCurr;
			pSrc = IndexStrings[CmpStream];
			Len = strlen(pSrc);
			if (memcpy_s(pCurr, pEnd - pCurr, pSrc, Len) != 0) break;
			pCurr += Len;
		}
	}

	*pCurr = 0;

	return CStrID(pBuf);
}
//---------------------------------------------------------------------

}