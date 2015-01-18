#include "GPUDriver.h"

#include <Render/VertexLayout.h>
#include <Data/Params.h>

namespace Render
{

//!!!D3D11 needs a shader signature!
PVertexLayout CGPUDriver::CreateVertexLayout(const CArray<CVertexComponent>& Components)
{
	if (!Components.GetCount()) return NULL;
	CStrID Signature = CVertexLayout::BuildSignature(Components);
	int Idx = VertexLayouts.FindIndex(Signature);
	if (Idx != INVALID_INDEX) return VertexLayouts.ValueAt(Idx);
	PVertexLayout Layout = InternalCreateVertexLayout();
	if (!Layout.IsValid() || !Layout->Create(Components)) return NULL;
	VertexLayouts.Add(Signature, Layout);
	return Layout;
}
//---------------------------------------------------------------------

}