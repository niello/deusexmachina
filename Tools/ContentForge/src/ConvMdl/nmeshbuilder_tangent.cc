//------------------------------------------------------------------------------
//  nmeshbuilder_tangent.cc
//  (C) 2002 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "nmeshbuilder.h"

//------------------------------------------------------------------------------
/**
    Build triangle normals and tangents. The tangents require a valid
    uv-mapping in texcoord layer 0.

    02-Sep-03   floh    no longer generates Binormals
*/
void
nMeshBuilder::BuildTriangleNormals()
{
    // compute face normals and tangents
    int triangleIndex;
    int numTriangles = this->GetNumTriangles();
    vector3 v0, v1;
    vector2 uv0, uv1;
    vector3 n, t, b;
    for (triangleIndex = 0; triangleIndex < numTriangles; triangleIndex++)
    {
        Triangle& tri = this->GetTriangleAt(triangleIndex);
        int index[3];
        tri.GetVertexIndices(index[0], index[1], index[2]);

        const Vertex& vertex0 = this->GetVertexAt(index[0]);
        const Vertex& vertex1 = this->GetVertexAt(index[1]);
        const Vertex& vertex2 = this->GetVertexAt(index[2]);

        // compute the face normal
        v0 = vertex1.GetCoord() - vertex0.GetCoord();
        v1 = vertex2.GetCoord() - vertex0.GetCoord();
        n = v0 * v1;
        n.norm();
        tri.SetNormal(n);

        // compute the tangents
        float x1 = vertex1.GetCoord().x - vertex0.GetCoord().x;
        float x2 = vertex2.GetCoord().x - vertex0.GetCoord().x;
        float y1 = vertex1.GetCoord().y - vertex0.GetCoord().y;
        float y2 = vertex2.GetCoord().y - vertex0.GetCoord().y;
        float z1 = vertex1.GetCoord().z - vertex0.GetCoord().z;
        float z2 = vertex2.GetCoord().z - vertex0.GetCoord().z;

        float s1 = vertex1.GetUv(0).x - vertex0.GetUv(0).x;
        float s2 = vertex2.GetUv(0).x - vertex0.GetUv(0).x;
        float t1 = vertex1.GetUv(0).y - vertex0.GetUv(0).y;
        float t2 = vertex2.GetUv(0).y - vertex0.GetUv(0).y;

        float l = (s1 * t2 - s2 * t1);
        // catch singularity
        if (l == 0.0f)
        {
            l = 0.0001f;
        }
        float r = 1.0f / l;
        vector3 sdir((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
        vector3 tdir((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r);

        // Gram-Schmidt orthogonalize
        t = (sdir - n * (n % sdir));
        t.norm();

        // calculate handedness
        float h;
        if (((n * sdir) % tdir) < 0.0f)
        {
            h = -1.0f;
        }
        else
        {
            h = +1.0f;
        }
        b = (n * t) * h;

        // FIXME: invert binormal for Nebula2 (also need to invert green channel in normalmaps)
        b = -b;

        tri.SetTangent(t);
        tri.SetBinormal(b);
    }
}

//------------------------------------------------------------------------------
/**
    Fixes the existing tangent and binormal directions. If tangents and
    binormals are delivered by Maya, Maya insist on consistent handedness
    of the tangent space matrices, even if the resulting directions are
    no longer conform with the original u/v directions. This method
    checks the triangle tangents/binormals against the existing
    vertex tangents/binormals, and if they are opposite, fixes the
    direction of the vertex tangents and binormals.
*/
void
nMeshBuilder::FixVertexTangentDirections()
{
    // inflate the mesh, this generates 3 (possibly redundant) vertices
    // for each triangle
    this->Inflate();

    // for each triangle...
    int triIndex;
    for (triIndex = 0; triIndex < this->GetNumTriangles(); triIndex++)
    {
        // get current triangle
        Triangle& curTriangle = this->GetTriangleAt(triIndex);

        // for each vertex in the triangle...
        int triVertexIndex;
        for (triVertexIndex = 0; triVertexIndex < 3; triVertexIndex++)
        {
            // get the vertex...
            Vertex& curVertex = this->GetVertexAt(curTriangle.vertexIndex[triVertexIndex]);

            // fix tangent
            vector3 vtxTangent = curVertex.GetTangent();
            const vector3& triTangent = curTriangle.GetTangent();
            if (vtxTangent.dot(triTangent) < 0.0f)
            {
                vtxTangent = -vtxTangent;
            }
            curVertex.SetTangent(vtxTangent);

            // fix binormal
            vector3 vtxBinormal = curVertex.GetBinormal();
            const vector3& triBinormal = curTriangle.GetBinormal();
            if (vtxBinormal.dot(triBinormal) < 0.0f)
            {
                vtxBinormal = -vtxBinormal;
            }
            curVertex.SetBinormal(vtxBinormal);
        }
    }

    // do a final cleanup, removing redundant vertices
    this->Cleanup(0);
}

//------------------------------------------------------------------------------
/**
    *** OBSOLETE OBSOLETE OBSOLETE ***

    This method is used by the Maya plugin if the tangents are not
    extracted from Maya. The tangent smoothing in here is very
    crude, so this doesn't give very good results...


    Generates the per-vertex tangents by averaging the
    per-triangle tangents and binormals which must be computed
    beforehand. Note that the vertex normals will not be touched!
    Internally, the method will create a clean mesh which contains
    only vertex coordinates and normals, and computes connectivity
    information from the resulting mesh. The result is that
    tangents and binormals are averaged for smooth edges, as defined
    by the existing normal set.

    @param  allowVertexSplits   true if vertex splits are allowed, this
                                will give better results at mirrored UV
                                edges, but the number of vertices may
                                change, this may be problematic for
                                blendshapes

    02-Sep-03   floh    no longer generates Binormals
*/
void
nMeshBuilder::BuildVertexTangents(bool allowVertexSplits)
{
    if (allowVertexSplits)
        this->BuildVertexTangentsWithSplits();
    else
        this->BuildVertexTangentsWithoutSplits();
}

//------------------------------------------------------------------------------
/**
*/
void
nMeshBuilder::BuildVertexTangentsWithSplits()
{
    // inflate the mesh, this generates 3 (possibly redundant) vertices
    // for each triangle
    this->Inflate();

    // create a pure coord mesh to get correct triangle connectivity
    nMeshBuilder cleanMesh = *this;
    nArray< nArray<int> > collapsMap(0, 0);
    collapsMap.SetFixedSize(this->GetNumVertices());
    cleanMesh.ForceVertexComponents(Vertex::COORD | Vertex::NORMAL);
    cleanMesh.Cleanup(&collapsMap);

    // create a connectivity map, which contains for each vertex
    // the triangle indices which use this vertex
    nArray< nArray<int> > vertexTriangleMap(0, 0);
    cleanMesh.BuildVertexTriangleMap(vertexTriangleMap);

    // compute averaged vertex tangents
    int vertexIndex = 0;
    int numVertices = cleanMesh.GetNumVertices();
    vector3 avgTangent;
    vector3 avgBinormal;
    nFixedArray<vector3> averagedTangents(this->GetNumVertices());
    nFixedArray<vector3> averagedBinormals(this->GetNumVertices());
    for (vertexIndex = 0; vertexIndex < numVertices; vertexIndex++)
    {
        avgTangent.set(0.0f, 0.0f, 0.0f);
        avgBinormal.set(0.0f, 0.0f, 0.0f);
        int numVertexTris = vertexTriangleMap[vertexIndex].Size();
        int vertexTriIndex;
        for (vertexTriIndex = 0; vertexTriIndex < numVertexTris; vertexTriIndex++)
        {
            const Triangle& tri = cleanMesh.GetTriangleAt(vertexTriangleMap[vertexIndex][vertexTriIndex]);
            avgTangent += tri.GetTangent();
            avgBinormal += tri.GetBinormal();
        }
        avgTangent.norm();
        avgBinormal.norm();

        int i;
        for (i = 0; i < collapsMap[vertexIndex].Size(); i++)
        {
            averagedTangents[collapsMap[vertexIndex][i]] = avgTangent;
            averagedBinormals[collapsMap[vertexIndex][i]] = avgBinormal;
        }
    }

    // fill tangents by deciding for each vertex whether to
    // use the triangle tangent or the averaged tangent this is
    // done by comparing the averaged and the triangle vertex,
    // if they are close enough to each other, the averaged tangent
    // is used
    int triangleIndex = 0;
    int numTriangles = this->GetNumTriangles();
    for (triangleIndex = 0; triangleIndex < numTriangles; triangleIndex++)
    {
        const vector3& triTangent = this->GetTriangleAt(triangleIndex).GetTangent();
        const vector3& triBinormal = this->GetTriangleAt(triangleIndex).GetBinormal();
        int i;
        for (i = 0; i < 3; i++)
        {
            vertexIndex = triangleIndex * 3 + i;
            const vector3& avgTangent = averagedTangents[vertexIndex];
            const vector3& avgBinormal = averagedBinormals[vertexIndex];

            if ((0 == avgTangent.compare(triTangent, 1.0f)) && (0 == avgBinormal.compare(triBinormal, 1.0f)))
            {
                // use averaged tangent for this vertex
                this->GetVertexAt(vertexIndex).SetTangent(avgTangent);
                this->GetVertexAt(vertexIndex).SetBinormal(avgBinormal);
            }
            else
            {
                // use triangle tangent for this vertex
                this->GetVertexAt(vertexIndex).SetTangent(triTangent);
                this->GetVertexAt(vertexIndex).SetBinormal(triBinormal);
            }
        }
    }

    // do a final cleanup, removing redundant vertices
    this->Cleanup(0);
}

//------------------------------------------------------------------------------
/**
*/
void
nMeshBuilder::BuildVertexTangentsWithoutSplits()
{
    // NOTE: this is the traditional method to generate UVs

    // create a clean coord/normal-only mesh, record the cleanup operation
    // in a collaps map so that we can inflate-copy the new vertex
    // components into the original mesh afterwards
    nArray< nArray<int> > collapsMap(0, 0);
    collapsMap.SetFixedSize(this->GetNumVertices());
    nMeshBuilder cleanMesh = *this;
    cleanMesh.ForceVertexComponents(Vertex::COORD | Vertex::NORMAL | Vertex::BINORMAL);
    cleanMesh.Cleanup(&collapsMap);

    // create a connectivity map which contains for each vertex
    // the triangle indices which share the vertex
    nArray< nArray<int> > vertexTriangleMap(0, 0);
    cleanMesh.BuildVertexTriangleMap(vertexTriangleMap);

    // for each vertex...
    int vertexIndex = 0;
    int numVertices = cleanMesh.GetNumVertices();
    vector3 avgTangent;
    vector3 avgBinormal;
    for (vertexIndex = 0; vertexIndex < numVertices; vertexIndex++)
    {
        avgTangent.set(0.0f, 0.0f, 0.0f);
        avgBinormal.set(0.0f, 0.0f, 0.0f);

        // for each triangle sharing this vertex...
        int numVertexTris = vertexTriangleMap[vertexIndex].Size();
        n_assert(numVertexTris > 0);
        int vertexTriIndex;
        for (vertexTriIndex = 0; vertexTriIndex < numVertexTris; vertexTriIndex++)
        {
            const Triangle& tri = cleanMesh.GetTriangleAt(vertexTriangleMap[vertexIndex][vertexTriIndex]);
            avgTangent += tri.GetTangent();
            avgBinormal += tri.GetBinormal();
        }

        // renormalize averaged tangent and binormal
        avgTangent.norm();
        avgBinormal.norm();

        cleanMesh.GetVertexAt(vertexIndex).SetTangent(avgTangent);
        cleanMesh.GetVertexAt(vertexIndex).SetBinormal(avgBinormal);
    }

    // inflate-copy the generated vertex tangents and binormals to the original mesh
    this->InflateCopyComponents(cleanMesh, collapsMap, Vertex::TANGENT | Vertex::BINORMAL);
}

//------------------------------------------------------------------------------
/**
    Generates the per-vertex normals by averaging the
    per-triangle normals which must be computed or exist
    beforehand. Note that only the vertex normals will be touched!

    29-Mar-2004 Johannes  added for nmax
*/
void
nMeshBuilder::BuildVertexNormals()
{
    // create a clean coord/normal-only mesh, record the cleanup operation
    // in a collaps map so that we can inflate-copy the new vertex
    // components into the original mesh afterwards
    nArray< nArray<int> > collapsMap(0, 0);
    collapsMap.SetFixedSize(this->GetNumVertices());
    nMeshBuilder cleanMesh = *this;
    cleanMesh.ForceVertexComponents(Vertex::COORD | Vertex::NORMAL);
    cleanMesh.Cleanup(&collapsMap);

    // create a connectivity map which contains for each vertex
    // the triangle indices which share the vertex
    nArray< nArray<int> > vertexTriangleMap(0, 0);
    cleanMesh.BuildVertexTriangleMap(vertexTriangleMap);

    // for each vertex...
    int vertexIndex = 0;
    const int numVertices = cleanMesh.GetNumVertices();
    vector3 avgNormal;
    for (vertexIndex = 0; vertexIndex < numVertices; vertexIndex++)
    {
        avgNormal.set(0.0f, 0.0f, 0.0f);

        // for each triangle sharing this vertex...
        int numVertexTris = vertexTriangleMap[vertexIndex].Size();
        n_assert(numVertexTris > 0);
        int vertexTriIndex;
        for (vertexTriIndex = 0; vertexTriIndex < numVertexTris; vertexTriIndex++)
        {
            const Triangle& tri = cleanMesh.GetTriangleAt(vertexTriangleMap[vertexIndex][vertexTriIndex]);
            avgNormal += tri.GetNormal();
        }

        // renormalize averaged normal
        avgNormal.norm();

        cleanMesh.GetVertexAt(vertexIndex).SetNormal(avgNormal);
    }

    // inflate-copy the generated vertex normals to the original mesh
    this->InflateCopyComponents(cleanMesh, collapsMap, Vertex::NORMAL);
}
