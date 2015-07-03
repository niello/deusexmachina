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
	d_matrixValid(false)
{
}
//--------------------------------------------------------------------

void CDEMGeometryBuffer::draw() const
{
	if (!d_bufferIsSync) syncHardwareBuffer();
	if (!d_matrixValid) updateMatrix();

	Data::CRect SR;
	SR.X = (int)d_clipRect.left();
	SR.Y = (int)d_clipRect.top();
	SR.W = (unsigned int)(d_clipRect.right() - d_clipRect.left());
	SR.H = (unsigned int)(d_clipRect.bottom() - d_clipRect.top());
	d_owner.getGPUDriver()->SetScissorRect(0, &SR);

	n_assert(false);
/*
	d_worldMatrixVariable->SetMatrix(reinterpret_cast<float*>(&d_matrix));

    // set our buffer as the vertex source.
    const UINT stride = sizeof(D3DVertex);
    const UINT offset = 0;
    d_device.d_context->IASetVertexBuffers(0, 1, &d_vertexBuffer, &stride, &offset);
*/

	const int pass_count = d_effect ? d_effect->getPassCount() : 1;
	for (int pass = 0; pass < pass_count; ++pass)
	{
		if (d_effect) d_effect->performPreRenderFunctions(pass);

		size_t pos = 0;
		for (CArray<BatchInfo>::CIterator i = d_batches.Begin(); i != d_batches.End(); ++i)
		{
			if (d_blendMode == BM_RTT_PREMULTIPLIED)
			{
	n_assert(false);
				/* 
				if (i->clip)
					d_premultipliedClippedTechnique->GetPassByIndex(0)->Apply(0, d_device.d_context);
				else
					d_premultipliedUnclippedTechnique->GetPassByIndex(0)->Apply(0, d_device.d_context);
				*/
			}
			else
			{
	n_assert(false);
				/* 
				if (i->clip)
					d_normalClippedTechnique->GetPassByIndex(0)->Apply(0, d_device.d_context);
				else
					d_normalUnclippedTechnique->GetPassByIndex(0)->Apply(0, d_device.d_context);
				*/
			}
 
	n_assert(false);
			/* 
			d_boundTextureVariable->SetResource(const_cast<ID3D11ShaderResourceView*>(i->texture));

			d_device.d_context->Draw(i->vertexCount, pos);
			*/
			pos += i->vertexCount;
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
	for (uint i = 0; i < vertex_count; ++i, ++vs)
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
	n_assert(false);
/*
    D3DXMatrixTransformation(&d_matrix, 0, 0, 0, &d_pivot, &d_rotation, &d_translation);
    d_matrixValid = true;
*/
}
//--------------------------------------------------------------------

const matrix44* CDEMGeometryBuffer::getMatrix() const
{
	if (!d_matrixValid) updateMatrix();
	return &d_matrix;
}
//--------------------------------------------------------------------

void CDEMGeometryBuffer::syncHardwareBuffer() const
{
	const DWORD vertex_count = (DWORD)d_vertices.GetCount();
	if (!vertex_count)
	{
		d_vertexBuffer = NULL;
		return;
	}

	if (vertex_count > d_bufferSize) d_vertexBuffer = NULL;

	if (d_vertexBuffer.IsNullPtr())
	{
		n_assert(false);
		//d_vertexBuffer = d_owner.createVertexBuffer(vertex_count, d_vertices.Begin());
		//or even
		//d_vertexBuffer = d_owner.createVertexBuffer(d_vertices); //store the only vertex layout inside
		//d_vertexBuffer = d_owner.getGPUDriver()->CreateVertexBuffer(THE_ONLY_VLAYOUT, vertex_count, Render::Access_GPU_Read | Render::Access_CPU_Write, d_vertices.Begin());
		d_bufferSize = vertex_count;
	}
	else
	{
	n_assert(false);
		//D3D11_MAPPED_SUBRESOURCE SubRes;
		//n_assert(SUCCEEDED(d_device.d_context->Map(d_vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &SubRes)));
		//memcpy(SubRes.pData, &d_vertices[0], sizeof(D3DVertex) * vertex_count);
		//d_device.d_context->Unmap(d_vertexBuffer, 0);
	}

	d_bufferIsSync = true;
}
//--------------------------------------------------------------------

}