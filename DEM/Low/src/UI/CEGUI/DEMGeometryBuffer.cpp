#include "DEMGeometryBuffer.h"
#include <Render/GPUDriver.h>
#include <Render/VertexBuffer.h>
#include <UI/CEGUI/DEMRenderer.h>
#include <UI/CEGUI/DEMShaderWrapper.h>
#include <UI/UIFwd.h>
#include <CEGUI/RenderMaterial.h>
#include <CEGUI/RenderEffect.h>

namespace CEGUI
{

CDEMGeometryBuffer::CDEMGeometryBuffer(CDEMRenderer& owner, RefCounted<RenderMaterial> renderMaterial)
	: GeometryBuffer(std::move(renderMaterial))
	, d_owner(owner)
{
}
//--------------------------------------------------------------------

void CDEMGeometryBuffer::setVertexLayout(Render::PVertexLayout newLayout)
{
	if (d_vertexLayout != newLayout)
	{
		d_vertexLayout = newLayout;
		d_vertexBuffer = nullptr;
		d_bufferSize = 0;
		d_bufferIsSync = false;
	}
}
//--------------------------------------------------------------------

void CDEMGeometryBuffer::draw(std::uint32_t drawModeMask) const
{
	if (d_vertexData.empty() || !d_vertexLayout || !d_owner.getGPUDriver()) return;

	auto pShaderWrapper = static_cast<const CDEMShaderWrapper*>(d_renderMaterial->getShaderWrapper());
	if (!pShaderWrapper)
	{
		::Sys::Error("CDEMGeometryBuffer::draw() > there is no shader in the material");
		return;
	}

	Render::CGPUDriver* pGPU = d_owner.getGPUDriver();

	if (d_clippingActive)
	{
		// Skip completely clipped geometry
		const auto W = static_cast<UPTR>(d_preparedClippingRegion.getWidth());
		const auto H = static_cast<UPTR>(d_preparedClippingRegion.getHeight());
		if (!W || !H) return;

		Data::CRect SR
		(
			static_cast<IPTR>(d_preparedClippingRegion.left()),
			static_cast<IPTR>(d_preparedClippingRegion.top()),
			W,
			H
		);
		pGPU->SetScissorRect(0, &SR);
	}

	if (!d_bufferIsSync)
	{
		d_bufferIsSync = true;

		const auto DataSize = d_vertexData.size() * sizeof(float);
		if (d_bufferSize < DataSize)
		{
			d_vertexBuffer = pGPU->CreateVertexBuffer(*d_vertexLayout, d_vertexCount, Render::Access_GPU_Read | Render::Access_CPU_Write, d_vertexData.data());
			d_bufferSize = DataSize;
		}
		else pGPU->WriteToResource(*d_vertexBuffer, d_vertexData.data(), DataSize);
	}

	pGPU->SetVertexBuffer(0, d_vertexBuffer.Get());
	pGPU->SetVertexLayout(d_vertexBuffer->GetVertexLayout());

	if (!d_matrixValid || !isRenderTargetDataValid(d_owner.getActiveRenderTarget()))
	{
		d_matrix = d_owner.getViewProjectionMatrix() * getModelMatrix();
		d_matrixValid = true;
	}

	// Set the uniform variables for this GeometryBuffer in the Shader
	d_renderMaterial->getShaderParamBindings()->setParameter("WVP", d_matrix);

	if (drawModeMask & (DrawModeFlagWindowRegular | DrawModeFlagMouseCursor))
	{
		d_renderMaterial->getShaderParamBindings()->setParameter("AlphaPercentage", d_alpha);
		const_cast<CDEMShaderWrapper*>(pShaderWrapper)->setInputSet(d_blendMode, d_clippingActive, false);
	}
	else if (drawModeMask & UI::DrawModeFlagWindowOpaque)
	{
		const_cast<CDEMShaderWrapper*>(pShaderWrapper)->setInputSet(d_blendMode, d_clippingActive, true);
	}
	else
	{
		::Sys::Error("CDEMGeometryBuffer::draw() > no supported draw mode is specified");
		return;
	}

	d_renderMaterial->prepareForRendering();

	// Render geometry

	Render::CPrimitiveGroup	primGroup;
	primGroup.FirstVertex = 0;
	primGroup.FirstIndex = 0;
	primGroup.VertexCount = d_vertexCount;
	primGroup.IndexCount = 0;
	primGroup.Topology = Render::Prim_TriList;
	// We don't use AABB

	const int passCount = d_effect ? d_effect->getPassCount() : 1;
	for (int pass = 0; pass < passCount; ++pass)
	{
		if (d_effect)
			d_effect->performPreRenderFunctions(pass);

		pGPU->Draw(primGroup);
	}

	if (d_effect)
		d_effect->performPostRenderFunctions();

	updateRenderTargetData(d_owner.getActiveRenderTarget());
}
//--------------------------------------------------------------------

}
