//------------------------------------------------------------------------------
//  nd3d9server_resource.cc
//  (C) 2003 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "gfx2/nd3d9server.h"
#include "resource/nresourceserver.h"
#include "gfx2/nmesh2.h"
#include "gfx2/nd3d9shader.h"
#include "gfx2/nd3d9occlusionquery.h"

//------------------------------------------------------------------------------
/**
    Create a new shared mesh object. If the object already exists, its refcount
    is increment.

    @param  RsrcName    a resource name (used for resource sharing)
    @return             pointer to a nD3D9Mesh2 object
*/
nMesh2*
nD3D9Server::NewMesh(const nString& RsrcName)
{
    return (nMesh2*)nResourceServer::Instance()->NewResource("nd3d9mesh", RsrcName, nResource::Mesh);
}

//------------------------------------------------------------------------------
/**
    Create a new shared texture object. If the object already exists, its
    refcount is incremented.

    @param  RsrcName    a resource name (used for resource sharing)
    @return             pointer to a nD3D9Texture2 object
*/
nTexture2*
nD3D9Server::NewTexture(const nString& RsrcName)
{
    return (nTexture2*)nResourceServer::Instance()->NewResource("nd3d9texture", RsrcName, nResource::Texture);
}

//------------------------------------------------------------------------------
/**
    Create a new shared shader object. If the object already exists, its
    refcount is incremented.

    @param  RsrcName    a resource name (used for resource sharing)
    @return             pointer to a nD3D9Shader2 object
*/
nShader2*
nD3D9Server::NewShader(const nString& RsrcName)
{
    return (nShader2*)nResourceServer::Instance()->NewResource("nd3d9shader", RsrcName, nResource::Shader);
}

//------------------------------------------------------------------------------
/**
    Create a new render target object.

    @param  RsrcName    a resource name for resource sharing
    @param  width       width of render target
    @param  height      height of render target
    @param  format      pixel format of render target
    @param  usageFlags  a combination of nTexture2::Usage flags
*/
nTexture2*
nD3D9Server::NewRenderTarget(const nString& RsrcName,
                             int width,
                             int height,
                             nTexture2::Format format,
                             int usageFlags)
{
    nTexture2* renderTarget = (nTexture2*)nResourceServer::Instance()->NewResource("nd3d9texture", RsrcName, nResource::Texture);
    n_assert(renderTarget);
    if (!renderTarget->IsLoaded())
    {
        n_assert(0 != (usageFlags & (nTexture2::RenderTargetColor | nTexture2::RenderTargetDepth | nTexture2::RenderTargetStencil)));
        renderTarget->SetUsage(usageFlags);
        renderTarget->SetWidth(width);
        renderTarget->SetHeight(height);
        renderTarget->SetDepth(1);
        renderTarget->SetFormat(format);
        renderTarget->SetType(nTexture2::TEXTURE_2D);
        bool success = renderTarget->Load();
        if (!success)
        {
            renderTarget->Release();
            return 0;
        }
    }
    return renderTarget;
}

//------------------------------------------------------------------------------
/**
    Create a new occlusion query object.

    @return     pointer to a new occlusion query object
*/
nOcclusionQuery*
nD3D9Server::NewOcclusionQuery()
{
    return n_new(nD3D9OcclusionQuery);
}

//------------------------------------------------------------------------------
/**
    Create a new occlusion query object.

    -19-April-07  kims  Fixed not to create a new vertex declaration if the declaration 
                        is already created one. It was moved from nd3d9mesh to nd3d9server.
                        The patch from Trignarion.

    @return     pointer to a new d3d vertex declaration.

*/
IDirect3DVertexDeclaration9*
nD3D9Server::NewVertexDeclaration(const int vertexComponentMask)
{
    IDirect3DVertexDeclaration9* d3d9vdecl(0);
    if (this->vertexDeclarationCache.HasKey(vertexComponentMask))
    {
        d3d9vdecl = this->vertexDeclarationCache.GetElement(vertexComponentMask);
        d3d9vdecl->AddRef();
        return d3d9vdecl;
    }

    const int maxElements = nMesh2::NumVertexComponents;
    D3DVERTEXELEMENT9 decl[maxElements];
    int curElement = 0;
    int curOffset  = 0;
    int index;
    for (index = 0; index < maxElements; index++)
    {
        int mask = (1<<index);
        if (vertexComponentMask & mask)
        {
            decl[curElement].Stream = 0;
            n_assert( curOffset <= int( 0xffff ) );
            decl[curElement].Offset = static_cast<WORD>( curOffset );
            decl[curElement].Method = D3DDECLMETHOD_DEFAULT;
            switch (mask)
            {
                case nMesh2::Coord:
                    decl[curElement].Type       = D3DDECLTYPE_FLOAT3;
                    decl[curElement].Usage      = D3DDECLUSAGE_POSITION;
                    decl[curElement].UsageIndex = 0;
                    curOffset += 3 * sizeof(float);
                    break;

                case nMesh2::Coord4:
                    decl[curElement].Type       = D3DDECLTYPE_FLOAT4;
                    decl[curElement].Usage      = D3DDECLUSAGE_POSITION;
                    decl[curElement].UsageIndex = 0;
                    curOffset += 4 * sizeof(float);
                    break;

                case nMesh2::Normal:
                    decl[curElement].Type       = D3DDECLTYPE_FLOAT3;
                    decl[curElement].Usage      = D3DDECLUSAGE_NORMAL;
                    decl[curElement].UsageIndex = 0;
                    curOffset += 3 * sizeof(float);
                    break;

                case nMesh2::Tangent:
                    decl[curElement].Type       = D3DDECLTYPE_FLOAT3;
                    decl[curElement].Usage      = D3DDECLUSAGE_TANGENT;
                    decl[curElement].UsageIndex = 0;
                    curOffset += 3 * sizeof(float);
                    break;

                case nMesh2::Binormal:
                    decl[curElement].Type       = D3DDECLTYPE_FLOAT3;
                    decl[curElement].Usage      = D3DDECLUSAGE_BINORMAL;
                    decl[curElement].UsageIndex = 0;
                    curOffset += 3 * sizeof(float);
                    break;

                case nMesh2::Color:
                    decl[curElement].Type       = D3DDECLTYPE_FLOAT4;
                    decl[curElement].Usage      = D3DDECLUSAGE_COLOR;
                    decl[curElement].UsageIndex = 0;
                    curOffset += 4 * sizeof(float);
                    break;

                case nMesh2::Uv0:
                    decl[curElement].Type       = D3DDECLTYPE_FLOAT2;
                    decl[curElement].Usage      = D3DDECLUSAGE_TEXCOORD;
                    decl[curElement].UsageIndex = 0;
                    curOffset += 2 * sizeof(float);
                    break;

                case nMesh2::Uv1:
                    decl[curElement].Type       = D3DDECLTYPE_FLOAT2;
                    decl[curElement].Usage      = D3DDECLUSAGE_TEXCOORD;
                    decl[curElement].UsageIndex = 1;
                    curOffset += 2 * sizeof(float);
                    break;

                case nMesh2::Uv2:
                    decl[curElement].Type       = D3DDECLTYPE_FLOAT2;
                    decl[curElement].Usage      = D3DDECLUSAGE_TEXCOORD;
                    decl[curElement].UsageIndex = 2;
                    curOffset += 2 * sizeof(float);
                    break;

                case nMesh2::Uv3:
                    decl[curElement].Type       = D3DDECLTYPE_FLOAT2;
                    decl[curElement].Usage      = D3DDECLUSAGE_TEXCOORD;
                    decl[curElement].UsageIndex = 3;
                    curOffset += 2 * sizeof(float);
                    break;

                case nMesh2::Weights:
                    decl[curElement].Type       = D3DDECLTYPE_FLOAT4;
                    decl[curElement].Usage      = D3DDECLUSAGE_BLENDWEIGHT;
                    decl[curElement].UsageIndex = 0;
                    curOffset += 4 * sizeof(float);
                    break;

                case nMesh2::JIndices:
                    decl[curElement].Type       = D3DDECLTYPE_FLOAT4;
                    decl[curElement].Usage      = D3DDECLUSAGE_BLENDINDICES;
                    decl[curElement].UsageIndex = 0;
                    curOffset += 4 * sizeof(float);
                    break;

                default:
                    n_error("Unknown vertex component in vertex component mask");
                    break;
            }
            curElement++;
        }
    }

    // write vertex declaration terminator element, see D3DDECL_END() macro in d3d9types.h for details
    decl[curElement].Stream = 0xff;
    decl[curElement].Offset = 0;
    decl[curElement].Type   = D3DDECLTYPE_UNUSED;
    decl[curElement].Method = 0;
    decl[curElement].Usage  = 0;
    decl[curElement].UsageIndex = 0;

    //n_dxverify(//gfxServer->pD3D9Device->CreateVertexDeclaration(decl, &(this->vertexDeclaration)),
    HRESULT hr = this->pD3D9Device->CreateVertexDeclaration(decl, &d3d9vdecl);
    d3d9vdecl->AddRef();
    this->vertexDeclarationCache.Add(vertexComponentMask, d3d9vdecl);

    return d3d9vdecl;
}
