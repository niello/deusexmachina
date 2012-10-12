//------------------------------------------------------------------------------
//  nd3d9server_shapes.cc
//  (C) 2004 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "gfx2/nd3d9server.h"
#include "gfx2/nd3d9shader.h"
#include "gfx2/nmesh2.h"

//------------------------------------------------------------------------------
/**
    Begin rendering shape primitives. Shape primitives are handy for
    quickly rendering debug visualizations.
*/
void
nD3D9Server::BeginShapes()
{
    nGfxServer2::BeginShapes();

    // render shader with state backup
    nShader2* shd = this->refShapeShader;
    int numPasses = shd->Begin(true);
    n_assert(1 == numPasses);
}

//------------------------------------------------------------------------------
/**
    Render a standard shape using the provided model matrix and color.
*/
void
nD3D9Server::DrawShape(ShapeType type, const matrix44& model, const vector4& color)
{
    n_assert(0 != this->shapeMeshes[type]);
    n_assert(this->inBeginShapes);

    // update color in shader
    nShader2* shd = this->refShapeShader;
    if (shd->IsParameterUsed(nShaderState::MatDiffuse))
    {
        shd->SetVector4(nShaderState::MatDiffuse, color);
    }

    // update model matrix
    this->PushTransform(nGfxServer2::Model, model);
    shd->BeginPass(0);
    HRESULT hr = this->shapeMeshes[type]->DrawSubset(0);
    n_dxtrace(hr, "DrawSubset() failed in nD3D9Server::DrawShape()");
    shd->EndPass();
    this->PopTransform(nGfxServer2::Model);
}

//------------------------------------------------------------------------------
/**
    Render a standard shape using the provided model matrix without
    any shader management.
*/
void
nD3D9Server::DrawShapeNS(ShapeType type, const matrix44& model)
{
    n_assert(0 != this->shapeMeshes[type]);
    this->PushTransform(nGfxServer2::Model, model);
    this->refShader->CommitChanges();
    HRESULT hr = this->shapeMeshes[type]->DrawSubset(0);
    n_dxtrace(hr, "DrawSubset() failed in nD3D9Server::DrawShape()");
    this->PopTransform(nGfxServer2::Model);
}

//------------------------------------------------------------------------------
/**
    Draw non-indexed primitives. This is slow, so it should only be used for
    debug visualizations. Vertex width is number of float's!
*/
void
nD3D9Server::DrawShapePrimitives(PrimitiveType type, int numPrimitives, const vector3* vertexList, int vertexWidth, const matrix44& model, const vector4& color, float Size)
{
	D3DPRIMITIVETYPE d3dPrimType;
	switch (type)
	{
		case PointList:
			d3dPrimType = D3DPT_POINTLIST;
			pD3D9Device->SetRenderState(D3DRS_POINTSIZE, *((DWORD*)&Size));
			break;
		case LineList:      d3dPrimType = D3DPT_LINELIST; break;
		case LineStrip:     d3dPrimType = D3DPT_LINESTRIP; break;
		case TriangleList:  d3dPrimType = D3DPT_TRIANGLELIST; break;
		case TriangleStrip: d3dPrimType = D3DPT_TRIANGLESTRIP; break;
		case TriangleFan:   d3dPrimType = D3DPT_TRIANGLEFAN; break;
	}

    // update color in shader
    nShader2* shd = this->refShapeShader;
    if (shd->IsParameterUsed(nShaderState::MatDiffuse))
    {
        shd->SetVector4(nShaderState::MatDiffuse, color);
    }

    // perform rendering
    this->PushTransform(nGfxServer2::Model, model);
    shd->BeginPass(0);
    HRESULT hr = pD3D9Device->DrawPrimitiveUP(d3dPrimType, numPrimitives, vertexList, vertexWidth * 4);
    n_assert(SUCCEEDED(hr));
    shd->EndPass();
    this->PopTransform(nGfxServer2::Model);

	switch (type)
	{
		case PointList:
			Size = 1.f;
			pD3D9Device->SetRenderState(D3DRS_POINTSIZE, *((DWORD*)&Size));
			break;
	}
}

//------------------------------------------------------------------------------
/**
    Directly Draw indexed primitives.  This is slow, so it should only be used for
    debug visualizations. Vertex width is number of float's!
*/
void
nD3D9Server::DrawShapeIndexedPrimitives(PrimitiveType type,
                                        int numPrimitives,
                                        const vector3* vertices,
                                        int numVertices,
                                        int vertexWidth,
                                        void* indices,
                                        IndexType indexType,
                                        const matrix44& model,
                                        const vector4& color)
{
    D3DPRIMITIVETYPE d3dPrimType;
    switch (type)
    {
    case PointList:     d3dPrimType = D3DPT_POINTLIST; break;
    case LineList:      d3dPrimType = D3DPT_LINELIST; break;
    case LineStrip:     d3dPrimType = D3DPT_LINESTRIP; break;
    case TriangleList:  d3dPrimType = D3DPT_TRIANGLELIST; break;
    case TriangleStrip: d3dPrimType = D3DPT_TRIANGLESTRIP; break;
    case TriangleFan:   d3dPrimType = D3DPT_TRIANGLEFAN; break;
    }

    D3DFORMAT d3dIndexType;
    switch (indexType)
    {
    case Index16:   d3dIndexType = D3DFMT_INDEX16; break;
    case Index32:   d3dIndexType = D3DFMT_INDEX32; break;
    }

    // update color in shader
    nShader2* shd = this->refShapeShader;
    if (shd->IsParameterUsed(nShaderState::MatDiffuse))
    {
        shd->SetVector4(nShaderState::MatDiffuse, color);
    }

    // perform rendering
    this->PushTransform(nGfxServer2::Model, model);
    shd->BeginPass(0);
    HRESULT hr = this->pD3D9Device->DrawIndexedPrimitiveUP(d3dPrimType,      // PrimitiveType
                                                          0,                // MinVertexIndex
                                                          numVertices,      // NumVertices
                                                          numPrimitives,    // PrimitiveCount
                                                          indices,          // pIndexData
                                                          d3dIndexType,     // IndexDataFormat
                                                          vertices,         // pVertexStreamZeroData
                                                          vertexWidth * 4); // VertexStreamZeroStride
    n_assert(SUCCEEDED(hr));
    shd->EndPass();
    this->PopTransform(nGfxServer2::Model);
}

//------------------------------------------------------------------------------
/**
    Finish rendering shapes.
*/
void
nD3D9Server::EndShapes()
{
    nGfxServer2::EndShapes();
    this->refShapeShader->End();

    // clear current mesh, otherwise Nebula2's redundancy mechanism may be fooled!
    this->SetMesh(0, 0);
}
