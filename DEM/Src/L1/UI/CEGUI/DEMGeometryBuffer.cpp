#include <StdCfg.h>
#include "DEMGeometryBuffer.h"
#include <UI/CEGUI/DEMTexture.h>
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

/*
void CDEMGeometryBuffer::draw() const
{
    // setup clip region
    D3D11_RECT clip;
    clip.left   = static_cast<LONG>(d_clipRect.left());
    clip.top    = static_cast<LONG>(d_clipRect.top());
    clip.right  = static_cast<LONG>(d_clipRect.right());
    clip.bottom = static_cast<LONG>(d_clipRect.bottom());
    d_device.d_context->RSSetScissorRects(1, &clip);

    if (!d_bufferSynched)
        syncHardwareBuffer();

    // apply the transformations we need to use.
    if (!d_matrixValid)
        updateMatrix();

    d_owner.setWorldMatrix(d_matrix);

    // set our buffer as the vertex source.
    const UINT stride = sizeof(D3DVertex);
    const UINT offset = 0;
    d_device.d_context->IASetVertexBuffers(0, 1, &d_vertexBuffer, &stride, &offset);

    const int pass_count = d_effect ? d_effect->getPassCount() : 1;
    for (int pass = 0; pass < pass_count; ++pass)
    {
        // set up RenderEffect
        if (d_effect)
            d_effect->performPreRenderFunctions(pass);

        // draw the batches
        size_t pos = 0;
        BatchList::const_iterator i = d_batches.begin();
        for ( ; i != d_batches.end(); ++i)
        {
            // Set Texture
            d_owner.setCurrentTextureShaderResource(
                    const_cast<ID3D11ShaderResourceView*>(i->texture));
            // Draw this batch
            d_owner.bindTechniquePass(d_blendMode, i->clip);
            d_device.d_context->Draw(i->vertexCount, pos);
            pos += i->vertexCount;
        }
    }

    // clean up RenderEffect
    if (d_effect)
        d_effect->performPostRenderFunctions();
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
    const ID3D11ShaderResourceView* srv =
        d_activeTexture ? d_activeTexture->getDirect3DShaderResourceView() : 0;

    // create a new batch if there are no batches yet, or if the active texture
    // differs from that used by the current batch.
    if (d_batches.empty() ||
        srv != d_batches.back().texture ||
        d_clippingActive != d_batches.back().clip)
    {
        BatchInfo batch = {srv, 0, d_clippingActive};
        d_batches.push_back(batch);
    }

    // update size of current batch
    d_batches.back().vertexCount += vertex_count;

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
        d_vertices.push_back(vd);
    }

    d_bufferSynched = false;
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
    const D3DXVECTOR3 p(d_pivot.d_x, d_pivot.d_y, d_pivot.d_z);
    const D3DXVECTOR3 t(d_translation.d_x,
                        d_translation.d_y,
                        d_translation.d_z);

    D3DXQUATERNION r;
    r.x = d_rotation.d_x;
    r.y = d_rotation.d_y;
    r.z = d_rotation.d_z;
    r.w = d_rotation.d_w;

    D3DXMatrixTransformation(&d_matrix, 0, 0, 0, &p, &r, &t);

    d_matrixValid = true;
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
    const size_t vertex_count = d_vertices.size();

    if (vertex_count > d_bufferSize)
    {
        d_vertexBuffer = NULL;
		//d_bufferSize = 0;
        allocateVertexBuffer(vertex_count);
    }

    if (vertex_count > 0)
    {
        void* buff;
		D3D11_MAPPED_SUBRESOURCE SubRes;
        if (FAILED(d_device.d_context->Map(d_vertexBuffer,0,D3D11_MAP_WRITE_DISCARD, 0, &SubRes)))
            CEGUI_THROW(RendererException("failed to map buffer."));
		buff=SubRes.pData;

        std::memcpy(buff, &d_vertices[0], sizeof(D3DVertex) * vertex_count);
        d_device.d_context->Unmap(d_vertexBuffer,0);
    }

    d_bufferSynched = true;
}
//--------------------------------------------------------------------

void CDEMGeometryBuffer::allocateVertexBuffer(const size_t count) const
{
	D3D11_BUFFER_DESC buffer_desc;
	buffer_desc.Usage          = D3D11_USAGE_DYNAMIC;
	buffer_desc.ByteWidth      = count * sizeof(D3DVertex);
	buffer_desc.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
	buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	buffer_desc.MiscFlags      = 0;

	if (FAILED(d_device.d_device->CreateBuffer(&buffer_desc, 0, &d_vertexBuffer)))
		CEGUI_THROW(RendererException("failed to allocate vertex buffer."));

	d_bufferSize = count;
}
//--------------------------------------------------------------------
*/
}