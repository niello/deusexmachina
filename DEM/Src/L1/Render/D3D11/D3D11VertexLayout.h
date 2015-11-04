#pragma once
#ifndef __DEM_L1_RENDER_D3D11_VERTEX_LAYOUT_H__
#define __DEM_L1_RENDER_D3D11_VERTEX_LAYOUT_H__

#include <Render/VertexLayout.h>
#include <Data/Dictionary.h>

// Direct3D11 implementation of the vertex layout class

struct ID3D11InputLayout;
struct D3D11_INPUT_ELEMENT_DESC;

namespace Render
{

class CD3D11VertexLayout: public CVertexLayout
{
	__DeclareClass(CD3D11VertexLayout);

protected:

	CDict<UPTR, ID3D11InputLayout*>	ShaderSignatureToLayout;	// Actual layouts indexed by a shader input signature
	D3D11_INPUT_ELEMENT_DESC*		pD3DDesc;					// Is reused for each actual input layout creation
	char*							pSemanticNames;

	void InternalDestroy();

public:

	CD3D11VertexLayout(): pD3DDesc(NULL), pSemanticNames(NULL) {}
	virtual ~CD3D11VertexLayout() { InternalDestroy(); }

	bool							Create(const CVertexComponent* pComponents, DWORD Count, D3D11_INPUT_ELEMENT_DESC* pD3DElementDesc);
	virtual void					Destroy() { InternalDestroy(); CVertexLayout::InternalDestroy(); }
	virtual bool					IsValid() const { return !!pD3DDesc; }

	bool							AddLayoutObject(UPTR ShaderInputSignatureID, ID3D11InputLayout* pD3DLayout);
	ID3D11InputLayout*				GetD3DInputLayout(UPTR ShaderInputSignatureID) const;
	const D3D11_INPUT_ELEMENT_DESC*	GetCachedD3DLayoutDesc() const { return pD3DDesc; }
};

typedef Ptr<CD3D11VertexLayout> PD3D11VertexLayout;

inline bool CD3D11VertexLayout::AddLayoutObject(UPTR ShaderInputSignatureID, ID3D11InputLayout* pD3DLayout)
{
	if (!pD3DLayout) FAIL; //???or destroy if found existing?
	int Idx = ShaderSignatureToLayout.FindIndex(ShaderInputSignatureID);
	if (Idx == INVALID_INDEX) ShaderSignatureToLayout.Add(ShaderInputSignatureID, pD3DLayout);
	OK;
}
//---------------------------------------------------------------------

inline ID3D11InputLayout* CD3D11VertexLayout::GetD3DInputLayout(UPTR ShaderInputSignatureID) const
{
	int Idx = ShaderSignatureToLayout.FindIndex(ShaderInputSignatureID);
	return Idx == INVALID_INDEX ? NULL : ShaderSignatureToLayout.ValueAt(Idx);
}
//---------------------------------------------------------------------

}

#endif
