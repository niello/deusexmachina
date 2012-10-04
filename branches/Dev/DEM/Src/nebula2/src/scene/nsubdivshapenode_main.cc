//------------------------------------------------------------------------------
//  nsubdivshapenode_main.cc
//  (C) 2003 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "scene/nsubdivshapenode.h"

nNebulaClass(nSubdivShapeNode, "nshapenode");

//------------------------------------------------------------------------------
/**
*/
nSubdivShapeNode::nSubdivShapeNode() :
    segmentSize(1.0f),
    maxDistance(10.0f)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
nSubdivShapeNode::~nSubdivShapeNode()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    This method must return the mesh usage flag combination required by
    this shape node class. Subclasses should override this method
    based on their requirements.

    @return     a combination on nMesh2::Usage flags
*/
int
nSubdivShapeNode::GetMeshUsage() const
{
    return nMesh2::ReadOnly | nMesh2::PointSprite | nMesh2::NeedsVertexShader;
}

//------------------------------------------------------------------------------
/**
*/
bool
nSubdivShapeNode::ApplyGeometry(nSceneServer* sceneServer)
{
    return true;
}

//------------------------------------------------------------------------------
/**
    Apply shader variables.
*/
bool
nSubdivShapeNode::ApplyShader(nSceneServer* sceneServer)
{
    if (nShapeNode::ApplyShader(sceneServer))
    {
        nShader2* shd = nGfxServer2::Instance()->GetShader();
        if (shd->IsParameterUsed(nShaderState::MinDist))
        {
            shd->SetFloat(nShaderState::MinDist, this->maxDistance - (this->maxDistance * 0.3f));
        }
        if (shd->IsParameterUsed(nShaderState::MaxDist))
        {
            shd->SetFloat(nShaderState::MaxDist, this->maxDistance);
        }
        return true;
    }
    return false;
}


//------------------------------------------------------------------------------
/**
    NOTE: expects input geometry with the following vertex components:
    coord | normal | tangent | uv0 | uv1

    - 15-Jan-04     floh    AreResourcesValid()/LoadResource() moved to scene server
*/
bool
nSubdivShapeNode::RenderGeometry(nSceneServer* sceneServer, nRenderContext* renderContext)
{
    n_assert(sceneServer);
    n_assert(renderContext);
    nGfxServer2* gfx = nGfxServer2::Instance();

    // only done on DirectX 9 cards
    if (gfx->GetFeatureSet() >= nGfxServer2::DX9)
    {
        // TODO call geometry manipulators!

        // only render if mesh resource is available (may not be
        // available yet if async resource loading enabled)
        if (!this->refMesh->IsLoaded())
        {
            return false;
        }

        // initialize dynamic mesh on demand
        if (!this->dynMesh.IsValid())
        {
            this->dynMesh.Initialize(nGfxServer2::PointList, nMesh2::Coord | nMesh2::Normal | nMesh2::Tangent | nMesh2::Uv0 | nMesh2::Uv1, nMesh2::PointSprite, false);
            n_assert(this->dynMesh.IsValid());
        }

        // get max distance squared
        float maxDistSquared = this->maxDistance * this->maxDistance;

        // get viewer position in model space
        const vector3& viewerPos = nGfxServer2::Instance()->GetTransform(nGfxServer2::InvModelView).pos_component();

        // get pointers to source geometry
        nMesh2* mesh = this->refMesh.get();
        float* srcVertices = mesh->LockVertices();
        ushort* srcIndices = mesh->LockIndices();
        int vertexWidth = mesh->GetVertexWidth();
        n_assert(13 == vertexWidth);

        // fill the dynamic mesh
        float* dstVertices;
        int maxVertices;

        // subdivide each triangle (FIXME: OPTIMIZE THIS!)
        static vector3 v0, v1, v2, d0, d1, v;
        static vector3 n0, n1, n2, nd0, nd1, n;
        static vector3 t0, t1, t2, td0, td1, t;
        static vector2 uv00, uv01, uv02, uv0d0, uv0d1, uv0;
        static vector2 uv10, uv11, uv12, uv1d0, uv1d1, uv1;
        static vector3 distVec;
        float oneDivSegSize = 1.0f / float(this->segmentSize);
        this->dynMesh.Begin(dstVertices, maxVertices);

        // NOTE: r is a lookup element into a random number table
        // for position permutation
        const nMeshGroup& meshGroup = mesh->Group(this->groupIndex);
        int curSrcIndex  = meshGroup.FirstIndex;
        int curDstVertex = 0;
        int lastSrcIndex = meshGroup.FirstIndex + meshGroup.NumIndices;
        for (; curSrcIndex < lastSrcIndex;)
        {
            // get triangle vertices
            float* v0Ptr = srcVertices + (srcIndices[curSrcIndex++] * vertexWidth);
            float* v1Ptr = srcVertices + (srcIndices[curSrcIndex++] * vertexWidth);
            float* v2Ptr = srcVertices + (srcIndices[curSrcIndex++] * vertexWidth);

            v0.set(v0Ptr[0], v0Ptr[1], v0Ptr[2]);
            v1.set(v1Ptr[0], v1Ptr[1], v1Ptr[2]);
            v2.set(v2Ptr[0], v2Ptr[1], v2Ptr[2]);

            // check if triangle is beyond the maximum distance
            distVec = v0 - viewerPos;
            float dist0 = distVec.lensquared();
            distVec = v1 - viewerPos;
            float dist1 = distVec.lensquared();
            distVec = v2 - viewerPos;
            float dist2 = distVec.lensquared();
            if ((dist0 > maxDistSquared) && (dist1 > maxDistSquared) && (dist2 > maxDistSquared))
            {
                // ignore this triangle
                continue;
            }

            n0.set(v0Ptr[3], v0Ptr[4], v0Ptr[5]);
            uv00.set(v0Ptr[6], v0Ptr[7]);
            uv10.set(v0Ptr[8], v0Ptr[9]);
            t0.set(v0Ptr[10], v0Ptr[11], v0Ptr[12]);

            n1.set(v1Ptr[3], v1Ptr[4], v1Ptr[5]);
            uv01.set(v1Ptr[6], v1Ptr[7]);
            uv11.set(v1Ptr[8], v1Ptr[9]);
            t1.set(v1Ptr[10], v1Ptr[11], v1Ptr[12]);

            n2.set(v2Ptr[3], v2Ptr[4], v2Ptr[5]);
            uv02.set(v2Ptr[6], v2Ptr[7]);
            uv12.set(v2Ptr[8], v2Ptr[9]);
            t2.set(v2Ptr[10], v2Ptr[11], v2Ptr[12]);

            d0 = (v1 - v0);
            d1 = (v2 - v0);
            float d0Len = d0.len();
            float d1Len = d1.len();
            float numSegments0 = d0Len * oneDivSegSize;
            float numSegments1 = d1Len * oneDivSegSize;
            float oneDivSegments0 = 1.0f / numSegments0;
            float oneDivSegments1 = 1.0f / numSegments1;
            float d1OverD0 = d1Len / d0Len;

            d0 *= oneDivSegments0;
            d1 *= oneDivSegments1;
            nd0 = (n1 - n0) * oneDivSegments0;
            nd1 = (n2 - n0) * oneDivSegments1;
            td0 = (t1 - t0) * oneDivSegments0;
            td1 = (t2 - t0) * oneDivSegments1;
            uv0d0 = (uv01 - uv00) * oneDivSegments0;
            uv0d1 = (uv02 - uv00) * oneDivSegments1;
            uv1d0 = (uv11 - uv10) * oneDivSegments0;
            uv1d1 = (uv12 - uv10) * oneDivSegments1;
            float i0;
            for (i0 = 0.0f; i0 < numSegments0; i0 += 1.0f)
            {
                // reset current components
                v = v0; n = n0; t = t0; uv0 = uv00; uv1 = uv10;
                float i1;
                for (i1 = 0.0f; i1 < numSegments1; i1 += 1.0f)
                {
                    int curDstIndex = curDstVertex++ * 13;
                    dstVertices[curDstIndex++] = v.x;
                    dstVertices[curDstIndex++] = v.y;
                    dstVertices[curDstIndex++] = v.z;
                    dstVertices[curDstIndex++] = n.x;
                    dstVertices[curDstIndex++] = n.y;
                    dstVertices[curDstIndex++] = n.z;
                    dstVertices[curDstIndex++] = uv0.x;
                    dstVertices[curDstIndex++] = uv0.y;
                    dstVertices[curDstIndex++] = uv1.x;
                    dstVertices[curDstIndex++] = uv1.y;
                    dstVertices[curDstIndex++] = t.x;
                    dstVertices[curDstIndex++] = t.y;
                    dstVertices[curDstIndex++] = t.z;

                    // render dynamic mesh?
                    if (curDstVertex >= (maxVertices - 3))
                    {
                        this->dynMesh.Swap(curDstVertex, dstVertices);
                        curDstVertex = 0;
                    }

                    // advance components in V direction
                    v += d1; n += nd1; t += td1; uv0 += uv0d1; uv1 += uv1d1;
                }
                // advance components in U direction
                v0 += d0; n0 += nd0; t0 += td0; uv00 += uv0d0; uv10 += uv1d0;
                numSegments1 -= d1OverD0;
            }
        }
        this->dynMesh.End(curDstVertex);
        mesh->UnlockVertices();
        mesh->UnlockIndices();
        return true;
    }
    else
    {
        return false;
    }
}


