//------------------------------------------------------------------------------
//  nskinmeshdeformer.cc
//  (C) 2004 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "deformers/nskinmeshdeformer.h"

//------------------------------------------------------------------------------
/**
*/
nSkinMeshDeformer::nSkinMeshDeformer() :
    skeleton(0),
    jointPalette(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
nSkinMeshDeformer::~nSkinMeshDeformer()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Perform the actual skin deformation.
*/
void
nSkinMeshDeformer::Compute()
{
    n_assert(this->refInputMesh.isvalid());
    n_assert(this->refOutputMesh.isvalid());
    n_assert(this->skeleton);
    n_assert(this->jointPalette);

    nMesh2* srcMesh = this->refInputMesh;
    nMesh2* dstMesh = this->refOutputMesh;
    int numVertices = srcMesh->GetNumVertices();
    n_assert(srcMesh->HasAllVertexComponents(nMesh2::Weights | nMesh2::JIndices));
    n_assert(dstMesh->HasAllVertexComponents(srcMesh->GetVertexComponents() & ~(nMesh2::Weights | nMesh2::JIndices)));
    n_assert(dstMesh->GetNumVertices() == numVertices);
    n_assert(srcMesh->GetVertexWidth() == (dstMesh->GetVertexWidth() + 8));

    int srcCompMask  = srcMesh->GetVertexComponents();
    int weightOffset = srcMesh->GetVertexComponentOffset(nMesh2::Weights);
    int jindexOffset = srcMesh->GetVertexComponentOffset(nMesh2::JIndices);
    float* srcPtr = srcMesh->LockVertices() + this->startVertex * srcMesh->GetVertexWidth();
    float* dstPtr = dstMesh->LockVertices() + this->startVertex * dstMesh->GetVertexWidth();
    int jointIndex[4];
    int vertexIndex;
    for (vertexIndex = 0; vertexIndex < this->numVertices; vertexIndex++)
    {
        int i;

        // get joint indices and weights
        float* weights = &srcPtr[weightOffset];
        float* indices = &srcPtr[jindexOffset];

        // get absolute joint indices
        for (i = 0; i < 4; i++)
        {
            jointIndex[i] = this->jointPalette->GetJointIndexAt(int(indices[i]));
        }

        vector3* srcVec = (vector3*)srcPtr;
        vector3* dstVec = (vector3*)dstPtr;
        dstVec->set(0.0f, 0.0f, 0.0f);
        for (i = 0; i < 4; i++)
        {
            const matrix44& m = this->skeleton->GetJointAt(jointIndex[i]).GetSkinMatrix44();
            if (weights[i] > 0.0f)
            {
                *dstVec += (m * (*srcVec)) * weights[i];
            }
        }
        srcPtr += 3;
        dstPtr += 3;

        if (srcCompMask & nMesh2::Normal)
        {
            srcVec = (vector3*)srcPtr;
            dstVec = (vector3*)dstPtr;
            dstVec->set(0.0f, 0.0f, 0.0f);
            for (i = 0; i < 4; i++)
            {
                const matrix33& m = this->skeleton->GetJointAt(jointIndex[i]).GetSkinMatrix33();
                if (weights[i] > 0.0f)
                {
                    *dstVec += (m * (*srcVec)) * weights[i];
                }
            }
            srcPtr += 3;
            dstPtr += 3;
        }
        if (srcCompMask & nMesh2::Uv0)
        {
            *dstPtr++ = *srcPtr++; *dstPtr++ = *srcPtr++;
        }
        if (srcCompMask & nMesh2::Uv1)
        {
            *dstPtr++ = *srcPtr++; *dstPtr++ = *srcPtr++;
        }
        if (srcCompMask & nMesh2::Uv2)
        {
            *dstPtr++ = *srcPtr++; *dstPtr++ = *srcPtr++;
        }
        if (srcCompMask & nMesh2::Uv3)
        {
            *dstPtr++ = *srcPtr++; *dstPtr++ = *srcPtr++;
        }
        if (srcCompMask & nMesh2::Color)
        {
            *dstPtr++ = *srcPtr++; *dstPtr++ = *srcPtr++; *dstPtr++ = *srcPtr++; *dstPtr++ = *srcPtr++;

        }
        if (srcCompMask & nMesh2::Tangent)
        {
            srcVec = (vector3*)srcPtr;
            dstVec = (vector3*)dstPtr;
            dstVec->set(0.0f, 0.0f, 0.0f);
            for (i = 0; i < 4; i++)
            {
                const matrix33& m = this->skeleton->GetJointAt(jointIndex[i]).GetSkinMatrix33();
                if (weights[i] > 0.0f)
                {
                    *dstVec += (m * (*srcVec)) * weights[i];
                }
            }
            srcPtr += 3;
            dstPtr += 3;
        }
        if (srcCompMask & nMesh2::Binormal)
        {
            srcVec = (vector3*)srcPtr;
            dstVec = (vector3*)dstPtr;
            dstVec->set(0.0f, 0.0f, 0.0f);
            for (i = 0; i < 4; i++)
            {
                const matrix33& m = this->skeleton->GetJointAt(jointIndex[i]).GetSkinMatrix33();
                if (weights[i] > 0.0f)
                {
                    *dstVec += (m * (*srcVec)) * weights[i];
                }
            }
            srcPtr += 3;
            dstPtr += 3;
        }

        // skip source weights and joint indices
        srcPtr += 8;
    }
    dstMesh->UnlockVertices();
    srcMesh->UnlockVertices();
}
