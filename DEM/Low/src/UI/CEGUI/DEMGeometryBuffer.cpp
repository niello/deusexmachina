#include <StdCfg.h>
#include "DEMGeometryBuffer.h"

#include <UI/CEGUI/DEMTexture.h>
#include <Render/GPUDriver.h>
#include <Render/VertexBuffer.h>
#include <Render/Texture.h>
#include <Data/Regions.h>

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

void CDEMGeometryBuffer::draw(/*uint32 drawModeMask*/) const
{
	if (d_vertexData.empty()) return;

	Render::CGPUDriver* pGPU = d_owner.getGPUDriver();
	n_assert_dbg(pGPU);
	if (!pGPU) return;

	if (d_clippingActive)
	{
		Data::CRect SR;
		SR.X = (int)d_preparedClippingRegion.left();
		SR.Y = (int)d_preparedClippingRegion.top();
		SR.W = (unsigned int)(d_preparedClippingRegion.right() - d_preparedClippingRegion.left());
		SR.H = (unsigned int)(d_preparedClippingRegion.bottom() - d_preparedClippingRegion.top());
		pGPU->SetScissorRect(0, &SR);
	}

	if (!d_matrixValid || !isRenderTargetDataValid(d_owner.getActiveRenderTarget()))
	{
		// Apply the view projection matrix to the model matrix and save the result as cached matrix
		d_matrix = d_owner.getViewProjectionMatrix() * getModelMatrix();
		d_matrixValid = true;
	}

	CEGUI::ShaderParameterBindings* shaderParameterBindings = (*d_renderMaterial).getShaderParamBindings();

	// Set the uniform variables for this GeometryBuffer in the Shader
	shaderParameterBindings->setParameter("modelViewProjMatrix", d_matrix);
	shaderParameterBindings->setParameter("alphaPercentage", d_alpha);

	//???
	if (!d_bufferIsSync)
	{
		const UPTR vertex_count = d_vertices.GetCount();
		if (!vertex_count)
		{
			d_vertexBuffer = NULL;
			return; // Nothing to draw
		}

		if (vertex_count > d_bufferSize) d_vertexBuffer = NULL;

		if (d_vertexBuffer.IsNullPtr())
		{
			d_vertexBuffer = d_owner.createVertexBuffer(d_vertices.Begin(), vertex_count);
			d_bufferSize = vertex_count;
		}
		else pGPU->WriteToResource(*d_vertexBuffer, d_vertices.Begin(), sizeof(D3DVertex) * vertex_count);

		d_bufferIsSync = true;
	}

	pGPU->SetVertexBuffer(0, d_vertexBuffer.Get());

	d_owner.bindBlendMode(d_blendMode);
	d_owner.bindRasterizerState(d_clippingActive);

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
		// set up RenderEffect
		if (d_effect) d_effect->performPreRenderFunctions(pass);

		//Prepare for the rendering process according to the used render material
		d_renderMaterial->prepareForRendering();

		// draw the geometry
		//???d_owner.commitChangedConsts();
		pGPU->Draw(primGroup);
	}

	// clean up RenderEffect
	if (d_effect) d_effect->performPostRenderFunctions();

	updateRenderTargetData(d_owner.getActiveRenderTarget());
}
//--------------------------------------------------------------------

void CDEMGeometryBuffer::appendGeometry(const float* vertex_data, std::size_t array_size)
{
	GeometryBuffer::appendGeometry(vertex_data, array_size);

	//////////////

	Render::CTexture* pTex = d_activeTexture ? d_activeTexture->getTexture() : NULL;

    // create a new batch if there are no batches yet, or if the active texture
    // differs from that used by the current batch.
    if (!d_batches.GetCount() ||
        pTex != d_batches.Back().texture ||
        d_clippingActive != d_batches.Back().clip)
    {
        BatchInfo batch = {pTex, 0, d_clippingActive};
        d_batches.Add(batch);
    }

	// update size of current batch
	d_batches.Back().vertexCount += vertex_count;

	// buffer these vertices
	D3DVertex vd;
	const Vertex* vs = vbuff;
	for (UPTR i = 0; i < vertex_count; ++i, ++vs)
	{
		// copy vertex info the buffer, converting from CEGUI::Vertex to
		// something directly usable by D3D as needed.
		vd.x       = vs->position.d_x;
		vd.y       = vs->position.d_y;
		vd.z       = vs->position.d_z;
		vd.diffuse = vs->colour_val.getARGB();
		vd.tu      = vs->tex_coords.d_x;
		vd.tv      = vs->tex_coords.d_y;
		d_vertices.Add(vd);
	}

	d_bufferIsSync = false;
}
//--------------------------------------------------------------------

}