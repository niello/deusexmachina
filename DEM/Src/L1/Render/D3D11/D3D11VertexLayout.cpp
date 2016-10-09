#include "D3D11VertexLayout.h"

#include <Core/Factory.h>
#define WIN32_LEAN_AND_MEAN
#include <d3d11.h>

namespace Render
{
__ImplementClass(Render::CD3D11VertexLayout, 'VL11', Render::CVertexLayout);

//!!!???assert destroyed?!
bool CD3D11VertexLayout::Create(const CVertexComponent* pComponents, UPTR Count, D3D11_INPUT_ELEMENT_DESC* pD3DElementDesc)
{
	if (!pComponents || !Count || !pD3DElementDesc) FAIL;

	SAFE_DELETE_ARRAY(pD3DDesc);
	SAFE_FREE(pSemanticNames);

	Components.RawCopyFrom(pComponents, Count);
	UPTR VSize = 0;
	UPTR SemanticNamesLen = Count; // Reserve bytes for terminating NULL-characters
	for (UPTR i = 0; i < Count; ++i)
	{
		//???what about D3D11_APPEND_ALIGNED_ELEMENT?
		VSize += Components[i].GetSize();
		SemanticNamesLen += strlen(pD3DElementDesc[i].SemanticName);
	}
	VertexSize = VSize;

	pD3DDesc = n_new_array(D3D11_INPUT_ELEMENT_DESC, Count);
	memcpy(pD3DDesc, pD3DElementDesc, Count * sizeof(D3D11_INPUT_ELEMENT_DESC));

	pSemanticNames = (char*)n_malloc(SemanticNamesLen);
	char* pCurr = pSemanticNames;
	char* pEnd = pSemanticNames + SemanticNamesLen;
	for (UPTR i = 0; i < Count; ++i)
	{
		const char* pSemName = pD3DDesc[i].SemanticName;
		UPTR Len = strlen(pSemName);
		if (memcpy_s(pCurr, pEnd - pCurr, pSemName, Len) != 0)
		{
			SAFE_DELETE_ARRAY(pD3DDesc);
			SAFE_FREE(pSemanticNames);
			FAIL;
		}
		pD3DDesc[i].SemanticName = pCurr;
		Components[i].UserDefinedName = pCurr;
		pCurr += Len;
		*pCurr = 0;
		++pCurr;
	}

	OK;
}
//---------------------------------------------------------------------

void CD3D11VertexLayout::InternalDestroy()
{
	SAFE_DELETE_ARRAY(pD3DDesc);
	SAFE_FREE(pSemanticNames);
	for (UPTR i = 0; i < ShaderSignatureToLayout.GetCount(); ++i)
	{
		ID3D11InputLayout* pLayout = ShaderSignatureToLayout.ValueAt(i);
		if (pLayout) pLayout->Release();
	}
	ShaderSignatureToLayout.Clear();
}
//---------------------------------------------------------------------

}