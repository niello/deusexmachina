//------------------------------------------------------------------------------
//  nd3d9mesharray_main.cc
//  (C) 2004 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "gfx2/nd3d9mesharray.h"
#include "kernel/nkernelserver.h"
#include "gfx2/nd3d9server.h"
#include "gfx2/nd3d9mesh.h"

nNebulaClass(nD3D9MeshArray, "nmesharray");

//------------------------------------------------------------------------------
/**
*/
nD3D9MeshArray::nD3D9MeshArray() :
    vertexDeclaration(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
nD3D9MeshArray::~nD3D9MeshArray()
{
    if (this->IsLoaded())
    {
        this->Unload();
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
nD3D9MeshArray::LoadResource()
{
    n_assert(0 == this->vertexDeclaration);
    if (nMeshArray::LoadResource())
    {
        this->CreateVertexDeclaration();
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void
nD3D9MeshArray::UnloadResource()
{
    if (this->vertexDeclaration)
    {
        this->vertexDeclaration->Release();
        this->vertexDeclaration = 0;
    }
    nMeshArray::UnloadResource();
}

//------------------------------------------------------------------------------
/**
    Creates a vertex declaration based on the vertex components of all valid
    meshes.
*/
void
nD3D9MeshArray::CreateVertexDeclaration()
{
    n_assert(0 == this->vertexDeclaration);

    nD3D9Server* gfxServer = (nD3D9Server*)nGfxServer2::Instance();
    n_assert(gfxServer && gfxServer->GetD3DDevice());

    const int maxElements = 11; //the maximum number of vertex components
    D3DVERTEXELEMENT9 decl[maxElements * nGfxServer2::MaxVertexStreams];

    int meshIndex;
    int curElement = 0;
    int postionUsageIndex = 0;
    int normalUsageIndex = 0;
    int tangentUsageIndex = 0;
    int binormalUsageIndex = 0;
    int colorUsageIndex = 0;
    int weightsUsageIndex = 0;
    int jindciesUsageIndex = 0;
    for (meshIndex = 0; meshIndex < nGfxServer2::MaxVertexStreams; meshIndex++)
    {
        nMesh2* mesh = this->GetMeshAt(meshIndex);
        if (0 != mesh)
        {
            int curOffset  = 0;
            int vertexComponentMask = ((nD3D9Mesh*)mesh)->GetVertexComponents();
            int index;
            for (index = 0; index < maxElements; index++)
            {
                int mask = 1 << index;
                if (vertexComponentMask & mask)
                {
                    bool ignoreElement = false;
                    decl[curElement].Stream = meshIndex; // to which vertex stream it referred
                    decl[curElement].Offset = curOffset;
                    decl[curElement].Method = D3DDECLMETHOD_DEFAULT;
                    switch (mask)
                    {
                        case nMesh2::Coord:
                            decl[curElement].Type       = D3DDECLTYPE_FLOAT3;
                            decl[curElement].Usage      = D3DDECLUSAGE_POSITION;
                            // usageIndex specifies the index in the vertex shader semantic, i.e.
                            // UsageIndex = 3 results in POSITION3
                            decl[curElement].UsageIndex = postionUsageIndex++;
                            curOffset += 3 * sizeof(float);
                            break;

                        case nMesh2::Normal:
                            decl[curElement].Type       = D3DDECLTYPE_FLOAT3;
                            decl[curElement].Usage      = D3DDECLUSAGE_NORMAL;
                            decl[curElement].UsageIndex = normalUsageIndex++;
                            curOffset += 3 * sizeof(float);
                            break;

                        case nMesh2::Tangent:
                            decl[curElement].Type       = D3DDECLTYPE_FLOAT3;
                            decl[curElement].Usage      = D3DDECLUSAGE_TANGENT;
                            decl[curElement].UsageIndex = tangentUsageIndex++;
                            curOffset += 3 * sizeof(float);
                            break;

                        case nMesh2::Binormal:
                            decl[curElement].Type       = D3DDECLTYPE_FLOAT3;
                            decl[curElement].Usage      = D3DDECLUSAGE_BINORMAL;
                            decl[curElement].UsageIndex = binormalUsageIndex++;
                            curOffset += 3 * sizeof(float);
                            break;

                        case nMesh2::Color:
                            decl[curElement].Type       = D3DDECLTYPE_FLOAT4;
                            decl[curElement].Usage      = D3DDECLUSAGE_COLOR;
                            decl[curElement].UsageIndex = colorUsageIndex++;
                            curOffset += 4 * sizeof(float);
                            break;

                        case nMesh2::Uv0:
                            if (meshIndex == 0)
                            {
                                decl[curElement].Type       = D3DDECLTYPE_FLOAT2;
                                decl[curElement].Usage      = D3DDECLUSAGE_TEXCOORD;
                                decl[curElement].UsageIndex = 0;
                                curOffset += 2 * sizeof(float);
                            }
                            else
                            {
                                ignoreElement = true;
                            }
                            break;

                        case nMesh2::Uv1:
                            if (meshIndex == 0)
                            {
                                decl[curElement].Type       = D3DDECLTYPE_FLOAT2;
                                decl[curElement].Usage      = D3DDECLUSAGE_TEXCOORD;
                                decl[curElement].UsageIndex = 1;
                                curOffset += 2 * sizeof(float);
                            }
                            else
                            {
                                ignoreElement = true;
                            }
                            break;

                        case nMesh2::Uv2:
                            if (meshIndex == 0)
                            {
                                decl[curElement].Type       = D3DDECLTYPE_FLOAT2;
                                decl[curElement].Usage      = D3DDECLUSAGE_TEXCOORD;
                                decl[curElement].UsageIndex = 2;
                                curOffset += 2 * sizeof(float);
                            }
                            else
                            {
                                ignoreElement = true;
                            }
                            break;

                        case nMesh2::Uv3:
                            if (meshIndex == 0)
                            {
                                decl[curElement].Type       = D3DDECLTYPE_FLOAT2;
                                decl[curElement].Usage      = D3DDECLUSAGE_TEXCOORD;
                                decl[curElement].UsageIndex = 3;
                                curOffset += 2 * sizeof(float);
                            }
                            else
                            {
                                ignoreElement = true;
                            }
                            break;

                        case nMesh2::Weights:
                            decl[curElement].Type       = D3DDECLTYPE_FLOAT4;
                            decl[curElement].Usage      = D3DDECLUSAGE_BLENDWEIGHT;
                            decl[curElement].UsageIndex = weightsUsageIndex++;
                            curOffset += 4 * sizeof(float);
                            break;

                        case nMesh2::JIndices:
                            decl[curElement].Type       = D3DDECLTYPE_FLOAT4;
                            decl[curElement].Usage      = D3DDECLUSAGE_BLENDINDICES;
                            decl[curElement].UsageIndex = jindciesUsageIndex++;
                            curOffset += 4 * sizeof(float);
                            break;

                        default:
                            n_error("Unknown vertex component in vertex component mask");
                            break;
                    }
                    if (!ignoreElement)
                    {
                        curElement++;
                    }
                }
            }
        }
    }

    // write vertex declaration terminator element, see D3DDECL_END() macro in d3d9types.h for details
    decl[curElement].Stream = 0xff;
    decl[curElement].Offset = 0;
    decl[curElement].Type   = D3DDECLTYPE_UNUSED;
    decl[curElement].Method = 0;
    decl[curElement].Usage  = 0;
    decl[curElement].UsageIndex = 0;

    HRESULT hr = gfxServer->GetD3DDevice()->CreateVertexDeclaration(decl, &this->vertexDeclaration);
    n_dxtrace(hr, "CreateVertexDeclaration() failed in nD3D9MeshArray");
    n_assert(this->vertexDeclaration);
}
