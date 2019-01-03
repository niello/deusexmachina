#include <StdCfg.h>
#include "DEMGeometryBuffer.h"

#include <UI/CEGUI/DEMTexture.h>
#include <Render/GPUDriver.h>
#include <Render/VertexBuffer.h>
#include <Data/Regions.h>

#include <CEGUI/RenderEffect.h>
#include <CEGUI/Vertex.h>

namespace CEGUI
{

CDEMGeometryBuffer::CDEMGeometryBuffer(CDEMRenderer& owner):
	d_owner(owner),
	d_activeTexture(0),
	d_vertexBuffer(0),
	d_bufferSize(0),
	d_bufferIsSync(false),
	d_clipRect(0, 0, 0, 0),
	d_clippingActive(true),
	d_effect(NULL),
	d_matrixValid(false),
	d_translation(0, 0, 0),
	d_rotation(1, 0, 0, 0),
	d_pivot(0, 0, 0)
{
}
//--------------------------------------------------------------------

void CDEMGeometryBuffer::draw(uint32 drawModeMask) const
{
	Render::CGPUDriver* pGPU = d_owner.getGPUDriver();
	n_assert_dbg(pGPU);

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

	pGPU->SetVertexBuffer(0, d_vertexBuffer.GetUnsafe());

	Data::CRect SR;
	SR.X = (int)d_clipRect.left();
	SR.Y = (int)d_clipRect.top();
	SR.W = (unsigned int)(d_clipRect.right() - d_clipRect.left());
	SR.H = (unsigned int)(d_clipRect.bottom() - d_clipRect.top());
	pGPU->SetScissorRect(0, &SR);

	if (!d_matrixValid) updateMatrix();
	d_owner.setWorldMatrix(d_matrix);

	Render::CPrimitiveGroup	primGroup;
	primGroup.FirstVertex = 0;
	primGroup.FirstIndex = 0;
	primGroup.IndexCount = 0;
	primGroup.Topology = Render::Prim_TriList;
	// We don't use AABB

	const int pass_count = d_effect ? d_effect->getPassCount() : 1;
	for (int pass = 0; pass < pass_count; ++pass)
	{
		//!!!Docs: performPreRenderFunctions() must be called AFTER all state changes!
		if (d_effect) d_effect->performPreRenderFunctions(pass);

		for (CArray<BatchInfo>::CIterator i = d_batches.Begin(); i != d_batches.End(); ++i)
		{
			primGroup.VertexCount = i->vertexCount;
			d_owner.setRenderState(d_blendMode, i->clip);
			d_owner.commitChangedConsts();
			pGPU->BindResource(Render::ShaderType_Pixel, d_owner.getTextureHandle(), i->texture.GetUnsafe());
			pGPU->Draw(primGroup);
			primGroup.FirstVertex += primGroup.VertexCount;
		}
	}

	if (d_effect) d_effect->performPostRenderFunctions();
}
//--------------------------------------------------------------------

void CDEMGeometryBuffer::setClippingRegion(const Rectf& region)
{
	d_clipRect.top(n_max(0.0f, region.top()));
	d_clipRect.bottom(n_max(0.0f, region.bottom()));
	d_clipRect.left(n_max(0.0f, region.left()));
	d_clipRect.right(n_max(0.0f, region.right()));
}
//--------------------------------------------------------------------

void CDEMGeometryBuffer::appendGeometry(const Vertex* const vbuff, uint vertex_count)
{
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

void CDEMGeometryBuffer::setActiveTexture(Texture* texture)
{
	d_activeTexture = static_cast<CDEMTexture*>(texture);
}
//--------------------------------------------------------------------

Texture* CDEMGeometryBuffer::getActiveTexture() const
{
	return d_activeTexture;
}
//--------------------------------------------------------------------

void CDEMGeometryBuffer::reset()
{
	d_batches.Clear();
	d_vertices.Clear();
	d_activeTexture = NULL;
}
//--------------------------------------------------------------------

void CDEMGeometryBuffer::updateMatrix() const
{
	d_matrix.set(1.f, 0.f, 0.f, 0.f,
				0.f, 1.f, 0.f, 0.f,
				0.f, 0.f, 1.f, 0.f,
				-d_pivot.d_x, -d_pivot.d_y, -d_pivot.d_z, 1.f);
	d_matrix.mult_simple(matrix44(quaternion(d_rotation.d_x, d_rotation.d_y, d_rotation.d_z, d_rotation.d_w)));
	matrix44 ToFinalPos(1.f, 0.f, 0.f, 0.f,
						0.f, 1.f, 0.f, 0.f,
						0.f, 0.f, 1.f, 0.f,
						d_pivot.d_x + d_translation.d_x, d_pivot.d_y + d_translation.d_y, d_pivot.d_z + d_translation.d_z, 1.f);
	d_matrix.mult_simple(ToFinalPos);

	d_matrixValid = true;
}
//--------------------------------------------------------------------

const matrix44* CDEMGeometryBuffer::getMatrix() const
{
	if (!d_matrixValid) updateMatrix();
	return &d_matrix;
}
//--------------------------------------------------------------------

}