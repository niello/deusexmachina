#include <StdCfg.h>
#include "DEMGeometryBuffer.h"

#include <Data/Regions.h>
#include <Render/GPUDriver.h>
#include <Render/VertexBuffer.h>
#include <Render/Texture.h>
#include <UI/CEGUI/DEMTexture.h>
#include <UI/CEGUI/DEMShaderWrapper.h>
#include <UI/UIFwd.h>

#include <CEGUI/RenderEffect.h>
#include <CEGUI/Vertex.h>
#include "CEGUI/ShaderParameterBindings.h"

namespace CEGUI
{

CDEMGeometryBuffer::CDEMGeometryBuffer(CDEMRenderer& owner, RefCounted<RenderMaterial> renderMaterial)
	: GeometryBuffer(renderMaterial)
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
	if (d_vertexData.empty()) return;

	Render::CGPUDriver* pGPU = d_owner.getGPUDriver();
	n_assert_dbg(pGPU);
	if (!pGPU) return;

	if (d_clippingActive)
	{
		// Skip completely clipped geometry
		const UPTR W = static_cast<UPTR>(d_preparedClippingRegion.getWidth());
		const UPTR H = static_cast<UPTR>(d_preparedClippingRegion.getHeight());
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

		const UPTR DataSize = d_vertexData.size() * sizeof(float);
		if (!DataSize || !d_vertexLayout)
		{
			d_vertexBuffer = nullptr;
			d_bufferSize = 0;
			return; // Nothing to draw
		}

		const float* pVertexData = d_vertexData.empty() ? nullptr : &d_vertexData[0];

		if (d_bufferSize < DataSize)
		{
			d_vertexBuffer = pGPU->CreateVertexBuffer(*d_vertexLayout, d_vertexCount, Render::Access_GPU_Read | Render::Access_CPU_Write, pVertexData);
			d_bufferSize = DataSize;
		}
		else pGPU->WriteToResource(*d_vertexBuffer, pVertexData, DataSize);
	}

	pGPU->SetVertexBuffer(0, d_vertexBuffer.Get());
	pGPU->SetVertexLayout(d_vertexBuffer->GetVertexLayout());

	if (!d_matrixValid || !isRenderTargetDataValid(d_owner.getActiveRenderTarget()))
	{
		// Apply the view projection matrix to the model matrix and save the result as cached matrix
		d_matrix = d_owner.getViewProjectionMatrix() * getModelMatrix();
		d_matrixValid = true;
	}

	CEGUI::ShaderParameterBindings* shaderParameterBindings = (*d_renderMaterial).getShaderParamBindings();

	// Set the uniform variables for this GeometryBuffer in the Shader
	shaderParameterBindings->setParameter("WVP", d_matrix);

	// Prepare for the rendering process according to the used render material
	const CDEMShaderWrapper& ShaderWrapper = *static_cast<const CDEMShaderWrapper*>(d_renderMaterial->getShaderWrapper());

	if (drawModeMask & (DrawModeFlagWindowRegular | DrawModeFlagMouseCursor))
	{
		shaderParameterBindings->setParameter("AlphaPercentage", d_alpha);
		const_cast<CDEMShaderWrapper&>(ShaderWrapper).setInputSet(d_blendMode, d_clippingActive, false);
	}
	else if (drawModeMask & UI::DrawModeFlagWindowOpaque)
	{
		const_cast<CDEMShaderWrapper&>(ShaderWrapper).setInputSet(d_blendMode, d_clippingActive, true);
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

	const int pass_count = d_effect ? d_effect->getPassCount() : 1;
	for (int pass = 0; pass < pass_count; ++pass)
	{
		if (d_effect) d_effect->performPreRenderFunctions(pass);

		pGPU->Draw(primGroup);
	}

	if (d_effect) d_effect->performPostRenderFunctions();

	updateRenderTargetData(d_owner.getActiveRenderTarget());
}
//--------------------------------------------------------------------

void CDEMGeometryBuffer::appendGeometry(const float* vertex_data, std::size_t array_size)
{
	GeometryBuffer::appendGeometry(vertex_data, array_size);
	d_bufferIsSync = false;
}
//--------------------------------------------------------------------

}
