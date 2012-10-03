//------------------------------------------------------------------------------
//  nd3d9server_lines.cc
//  (C) 2004 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "gfx2/nd3d9server.h"

//------------------------------------------------------------------------------
/**
    Begin rendering lines.
*/
void
nD3D9Server::BeginLines()
{
    n_assert(this->d3dxLine);
    nGfxServer2::BeginLines();
    HRESULT hr = this->d3dxLine->Begin();
    n_dxtrace(hr, "ID3DXLine::Begin() failed!");
}

//------------------------------------------------------------------------------
/**
    Render a 3d line strip using the current transformations.
*/
void
nD3D9Server::DrawLines3d(const vector3* vertexList, int numVertices, const vector4& color)
{
    n_assert(vertexList);
    n_assert(numVertices > 0);
    n_assert(this->inBeginLines);
    n_assert(this->d3dxLine);
    D3DCOLOR d3dColor = N_COLORVALUE(color.x, color.y, color.z, color.w);
    const matrix44& modelViewProj = this->GetTransform(nGfxServer2::ModelViewProjection);
    HRESULT hr = this->d3dxLine->DrawTransform((CONST D3DXVECTOR3*)vertexList, numVertices, (CONST D3DXMATRIX*)&modelViewProj, d3dColor);
    n_dxtrace(hr, "ID3DXLine::DrawTransform() failed!");
}

//------------------------------------------------------------------------------
/**
    Render a 3d line strip using the current transformations. Careful:
    Clipping doesn't work correctly!
*/
void
nD3D9Server::DrawLines2d(const vector2* vertexList, int numVertices, const vector4& color)
{
    n_assert(vertexList);
    n_assert(numVertices > 0);
    n_assert(this->inBeginLines);
    n_assert(this->d3dxLine);
    D3DCOLOR d3dColor = N_COLORVALUE(color.x, color.y, color.z, color.w);

    // convert to d3d screen space
    vector2 rtSize = this->GetCurrentRenderTargetSize();
    D3DXVECTOR2* d3dxVectors = n_new_array(D3DXVECTOR2, numVertices);
    int i;
    for (i = 0; i < numVertices; i++)
    {
        d3dxVectors[i].x = vertexList[i].x * rtSize.x;
        d3dxVectors[i].y = vertexList[i].y * rtSize.y;
    }

    HRESULT hr = this->d3dxLine->Draw(d3dxVectors, numVertices, d3dColor);
    n_dxtrace(hr, "ID3DXLine::Draw() failed!");
    n_delete(d3dxVectors);
}

//------------------------------------------------------------------------------
/**
    Finish rendering lines.
*/
void
nD3D9Server::EndLines()
{
    n_assert(this->d3dxLine);
    nGfxServer2::EndLines();
    HRESULT hr = this->d3dxLine->End();
    n_dxtrace(hr, "ID3DXLine::End() failed!");
}
