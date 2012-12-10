#include "Renderer.h"

//!!!OLD! TMP!
#include <gfx2/nd3d9server.h>

namespace Render
{
ImplementRTTI(Render::CRenderer, Core::CRefCounted);
__ImplementSingleton(CRenderer);

bool CRenderer::Open()
{
	n_assert(!_IsOpen);
	pD3DDevice = nD3D9Server::Instance()->GetD3DDevice();
	pEffectPool = nD3D9Server::Instance()->GetEffectPool();
	_IsOpen = true;
	OK;
}
//---------------------------------------------------------------------

void CRenderer::Close()
{
	n_assert(_IsOpen);
	_IsOpen = false;
}
//---------------------------------------------------------------------

PVertexLayout CRenderer::GetVertexLayout(const nArray<CVertexComponent>& Components)
{
	if (!Components.Size()) return NULL;
	CStrID Signature = CVertexLayout::BuildSignature(Components);
	int Idx = VertexLayouts.FindIndex(Signature);
	if (Idx != INVALID_INDEX) return VertexLayouts.ValueAtIndex(Idx);
	PVertexLayout Layout = n_new(CVertexLayout);
	n_assert(Layout->Create(Components));
	VertexLayouts.Add(Signature, Layout);
	return Layout;
}
//---------------------------------------------------------------------

DWORD CRenderer::ShaderFeatureStringToMask(const nString& FeatureString)
{
	DWORD Mask = 0;

	nArray<nString> Features;
	FeatureString.Tokenize("\t |", Features);
	for (int i = 0; i < Features.Size(); ++i)
	{
		CStrID Feature = CStrID(Features[i].Get());
		int Idx = ShaderFeatures.FindIndex(Feature);
		if (Idx != INVALID_INDEX) Mask |= (1 << ShaderFeatures.ValueAtIndex(Idx));
		else
		{
			int BitIdx = ShaderFeatures.Size();
			if (BitIdx >= MaxShaderFeatureCount)
			{
				n_error("ShaderFeature: more then %d unqiue shader features requested!", MaxShaderFeatureCount);
				return 0;
			}
			ShaderFeatures.Add(Feature, BitIdx);
			Mask |= (1 << BitIdx);
		}
	}
	return Mask;
}
//---------------------------------------------------------------------

}