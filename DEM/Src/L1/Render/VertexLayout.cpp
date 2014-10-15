#include "VertexLayout.h"

namespace Render
{

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

CStrID CVertexLayout::BuildSignature(const CArray<CVertexComponent>& Components)
{
	if (!Components.GetCount()) return CStrID::Empty;
	CString UID;
	for (int i = 0; i < Components.GetCount(); ++i)
	{
		const CVertexComponent& Cmp = Components[i];
		UID += Cmp.GetSemanticString();
		if (Cmp.Index > 0) UID.AppendInt(Cmp.Index);
		UID += Cmp.GetFormatString();
		if (Cmp.Stream > 0)
		{
			UID += "s";
			UID.AppendInt(Cmp.Stream);
		}
	}
	return CStrID(UID.CStr());
}
//---------------------------------------------------------------------

}