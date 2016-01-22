#include "D3D9VertexLayout.h"

#include <Core/Factory.h>
#define WIN32_LEAN_AND_MEAN
#define D3D_DISABLE_9EX
#include <d3d9.h>

namespace Render
{
__ImplementClass(Render::CD3D9VertexLayout, 'VL09', Render::CVertexLayout);

//!!!???assert destroyed?!
bool CD3D9VertexLayout::Create(const CVertexComponent* pComponents, DWORD Count, IDirect3DVertexDeclaration9* pD3DDecl)
{
	if (!pD3DDecl || !pComponents || !Count) FAIL;

	// Since user-defined semantic names are not supported for D3D9, values are
	// always NULL and we don't need to allocate memory for that names.
	Components.RawCopyFrom(pComponents, Count);
	UPTR VSize = 0;
	for (UPTR i = 0; i < Count; ++i)
		VSize += Components[i].GetSize();
	VertexSize = VSize;
	pDecl = pD3DDecl;

	OK;
}
//---------------------------------------------------------------------

void CD3D9VertexLayout::InternalDestroy()
{
	SAFE_RELEASE(pDecl);
}
//---------------------------------------------------------------------

}