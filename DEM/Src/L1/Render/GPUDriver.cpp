#include "GPUDriver.h"

#include <Render/VertexLayout.h>

namespace Render
{

PVertexLayout CGPUDriver::GetVertexLayout(const CArray<CVertexComponent>& Components)
{
	if (!Components.GetCount()) return NULL;
	CStrID Signature = CVertexLayout::BuildSignature(Components);
	int Idx = VertexLayouts.FindIndex(Signature);
	if (Idx != INVALID_INDEX) return VertexLayouts.ValueAt(Idx);
	PVertexLayout Layout = CreateVertexLayout();
	if (!Layout->Create(Components)) return NULL;
	VertexLayouts.Add(Signature, Layout);
	return Layout;
}
//---------------------------------------------------------------------

}