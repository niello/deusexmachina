#include "D3D9VertexLayout.h"

#include <Core/Factory.h>
#define WIN32_LEAN_AND_MEAN
#define D3D_DISABLE_9EX
#include <d3d9.h>

namespace Render
{
__ImplementClass(Render::CD3D9VertexLayout, 'VL09', Render::CVertexLayout);

//!!!???assert destroyed?!
bool CD3D9VertexLayout::Create(const CVertexComponent* pComponents, UPTR Count, IDirect3DVertexDeclaration9* pD3DDecl)
{
	if (!pComponents || !Count) FAIL;

	// Since user-defined semantic names are not supported for D3D9, values are
	// always nullptr and we don't need to allocate memory for that names.
	Components.RawCopyFrom(pComponents, Count);
	UPTR VSize = 0;
	for (UPTR i = 0; i < Count; ++i)
		VSize += pComponents[i].GetSize();
	VertexSize = VSize;
	pDecl = pD3DDecl;	// May be nullptr if this layout is per-instance only

	if (pD3DDecl)
	{
		InstanceStreamFlags = 0;
		for (UPTR i = 0; i < Count; ++i)
			if (pComponents[i].PerInstanceData)
				InstanceStreamFlags |= (1 << pComponents[i].Stream);
	}

	OK;
}
//---------------------------------------------------------------------

void CD3D9VertexLayout::InternalDestroy()
{
	SAFE_RELEASE(pDecl);
}
//---------------------------------------------------------------------

}