//------------------------------------------------------------------------------
//  nmeshbuilder_loadsave.cc
//  (C) 2002 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "nmeshbuilder.h"
#include <Data/DataServer.h>
#include "Data/Streams/FileStream.h"
#include "util/nstring.h"
#include "gfx2/nnvx2loader.h"
#include "gfx2/nn3d2loader.h"

//------------------------------------------------------------------------------
/**
    Load a mesh file. The file format will be determined by looking at
    the filename extension:

     - @c .obj    -> wavefront object file (ascii)
     - @c .n3d    -> legacy n3d files (ascii)
     - @c .n3d2   -> new n3d2 files (ascii)
     - @c .nvx    -> legacy nvx files (binary)
     - @c .nvx2   -> new nvx2 files (binary)
*/
bool
nMeshBuilder::Load(const char* filename)
{
    n_assert(filename);
    

    nString path(filename);
    if (path.CheckExtension("n3d"))
    {
        return this->LoadN3d(filename);
    }
    else if (path.CheckExtension("nvx"))
    {
        return this->LoadNvx(filename);
    }
    else if (path.CheckExtension("obj"))
    {
        return this->LoadObj(filename);
    }
    else
    {
        nMeshLoader* meshLoader = 0;
        if (path.CheckExtension("n3d2"))
        {
            meshLoader = n_new(nN3d2Loader);
        }
        else if (path.CheckExtension("nvx2"))
        {
            meshLoader = n_new(nNvx2Loader);
        }

        if (0 != meshLoader)
        {
            bool retval = this->LoadFile(meshLoader, filename);
            n_delete(meshLoader);
            return retval;
        }
        else
        {
            n_printf("nMeshBuilder::Load(): unsupported file extension in '%s'\n", filename);
            return false;
        }
    }
}

//------------------------------------------------------------------------------
/**
    Save into mesh file. The file format will be determined by looking
    at the file extension:

    - .n3d2   n3d2 file format (ascii)
    - .nvx2   nvx2 file format (binary)
    - .n3d    legacy Nebula1 ascii format
*/
bool
nMeshBuilder::Save(const char* filename)
{
    n_assert(filename);
    

    // make sure all vertices have consistent vertex components
    this->ExtendVertexComponents();

    nString path(filename);
    if (path.CheckExtension("n3d2"))
    {
        return this->SaveN3d2(filename);
    }
    else if (path.CheckExtension("nvx2"))
    {
        return this->SaveNvx2(filename);
    }
    else if (path.CheckExtension("n3d"))
    {
        return this->SaveN3d(filename);
    }
    else
    {
        n_printf("nMeshBuilder::Save(): unsupported file extension in '%s'\n", filename);
        return false;
    }
}

//------------------------------------------------------------------------------
/**
    Save the mesh as binary to an open file handle.
*/
bool
nMeshBuilder::SaveNvx2(Data::CFileStream* file)
{
    // sort triangles by group id and create a group map
    this->SortTriangles();
    nArray<Group> groupMap;
    this->BuildGroupMap(groupMap);

    const int numGroups = groupMap.Size();
    const int vertexWidth = this->GetVertexAt(0).GetWidth();
    const int numVertices = this->GetNumVertices();
    const int numTriangles = this->GetNumTriangles();
    const int numEdges = this->GetNumEdges();

    // write header
    file->Put<int>('NVX2');
    file->Put<int>(numGroups);
    file->Put<int>(numVertices);
    file->Put<int>(vertexWidth);
    file->Put<int>(numTriangles);
    file->Put<int>(numEdges);
    file->Put<int>(this->GetVertexAt(0).GetComponentMask());

    // write groups
    int curGroupIndex;
    for (curGroupIndex = 0; curGroupIndex < groupMap.Size(); curGroupIndex++)
    {
        const Group& curGroup = groupMap[curGroupIndex];
        int firstTriangle = curGroup.GetFirstTriangle();
        int numTriangles  = curGroup.GetNumTriangles();
        int minVertexIndex, maxVertexIndex;
        this->GetGroupVertexRange(curGroup.GetId(), minVertexIndex, maxVertexIndex);
        int minEdgeIndex, maxEdgeIndex;
        this->GetGroupEdgeRange(curGroup.GetId(), minEdgeIndex, maxEdgeIndex);
        file->Put<int>(minVertexIndex);
        file->Put<int>((maxVertexIndex - minVertexIndex) + 1);
        file->Put<int>(firstTriangle);
        file->Put<int>(numTriangles);
        file->Put<int>(minEdgeIndex);
        file->Put<int>((maxEdgeIndex - minEdgeIndex) + 1);
    }

    // write mesh block
    float* floatBuffer = n_new_array(float, this->GetNumVertices() * vertexWidth);
    float* floatPtr = floatBuffer;
    int curVertexIndex;
    for (curVertexIndex = 0; curVertexIndex < numVertices; curVertexIndex++)
    {
        const Vertex& curVertex = this->GetVertexAt(curVertexIndex);
        if (curVertex.HasComponent(Vertex::COORD))
        {
            const vector3& v = curVertex.GetCoord();
            *floatPtr++ = v.x;
            *floatPtr++ = v.y;
            *floatPtr++ = v.z;
        }
        if (curVertex.HasComponent(Vertex::NORMAL))
        {
            const vector3& v = curVertex.GetNormal();
            *floatPtr++ = v.x;
            *floatPtr++ = v.y;
            *floatPtr++ = v.z;
        }
        int curUvSet;
        for (curUvSet = 0; curUvSet < Vertex::MAX_TEXTURE_LAYERS; curUvSet++)
        {
            if (curVertex.HasComponent((Vertex::Component) (Vertex::UV0 << curUvSet)))
            {
                const vector2& v = curVertex.GetUv(curUvSet);
                *floatPtr++ = v.x;
                *floatPtr++ = v.y;
            }
        }
        if (curVertex.HasComponent(Vertex::COLOR))
        {
            const vector4& v = curVertex.GetColor();
            *floatPtr++ = v.x;
            *floatPtr++ = v.y;
            *floatPtr++ = v.z;
            *floatPtr++ = v.w;
        }
        if (curVertex.HasComponent(Vertex::TANGENT))
        {
            const vector3& v = curVertex.GetTangent();
            *floatPtr++ = v.x;
            *floatPtr++ = v.y;
            *floatPtr++ = v.z;
        }
        if (curVertex.HasComponent(Vertex::BINORMAL))
        {
            const vector3& v = curVertex.GetBinormal();
            *floatPtr++ = v.x;
            *floatPtr++ = v.y;
            *floatPtr++ = v.z;
        }
        if (curVertex.HasComponent(Vertex::WEIGHTS))
        {
            const vector4& v = curVertex.GetWeights();
            *floatPtr++ = v.x;
            *floatPtr++ = v.y;
            *floatPtr++ = v.z;
            *floatPtr++ = v.w;
        }
        if (curVertex.HasComponent(Vertex::JINDICES))
        {
            const vector4& v = curVertex.GetJointIndices();
            *floatPtr++ = v.x;
            *floatPtr++ = v.y;
            *floatPtr++ = v.z;
            *floatPtr++ = v.w;
        }
    }
    file->Write(floatBuffer, this->GetNumVertices() * vertexWidth * sizeof(float));
    n_delete_array(floatBuffer);
    floatBuffer = 0;

    // write index block
    ushort* ushortBuffer = n_new_array(ushort, this->GetNumTriangles() * 3);
    ushort* ushortPtr = ushortBuffer;
    int curTriangleIndex;
    for (curTriangleIndex = 0; curTriangleIndex < numTriangles; curTriangleIndex++)
    {
        const Triangle& curTriangle = this->GetTriangleAt(curTriangleIndex);
        int i0, i1, i2;
        curTriangle.GetVertexIndices(i0, i1, i2);
        *ushortPtr++ = (ushort) i0;
        *ushortPtr++ = (ushort) i1;
        *ushortPtr++ = (ushort) i2;
    }
    file->Write(ushortBuffer, this->GetNumTriangles() * 3 * sizeof(ushort));
    n_delete_array(ushortBuffer);
    ushortBuffer = 0;

    return true;
}

//------------------------------------------------------------------------------
/**
    Save the mesh as binary nvx2 file. See inc/gfx2/nmesh2.h for
    format specification.
*/
bool
nMeshBuilder::SaveNvx2(const char* filename)
{
    
    n_assert(filename);

    bool retval = false;
    Data::CFileStream file;
	if (file.Open(filename, Data::SAM_WRITE))
    {
        retval = this->SaveNvx2(&file);
        file.Close();
        retval = true;
    }
    return retval;
}

//------------------------------------------------------------------------------
/**
    Load mesh data with the provided nMeshLoader.

    WARNING: the edge data is not loaded from file, because it will be invalid when
    data changes. Use CreateEdges() to rebuild edge data.
*/
bool
nMeshBuilder::LoadFile(nMeshLoader* meshLoader, const char* filename)
{
    
    n_assert(meshLoader);
    n_assert(filename);

    meshLoader->SetFilename(filename);
    meshLoader->SetIndexType(nMeshLoader::Index16);
    if (meshLoader->Open())
    {
        int numGroups        = meshLoader->GetNumGroups();
        int numVertices      = meshLoader->GetNumVertices();
        int vertexWidth      = meshLoader->GetVertexWidth();
        int numIndices       = meshLoader->GetNumIndices();
        int vertexComponents = meshLoader->GetVertexComponents();

        // read vertices and indices into temporary buffers
        int vertexBufferSize = numVertices * vertexWidth * sizeof(float);
        int indexBufferSize  = numIndices * sizeof(ushort);
        float* vertexBuffer = (float*) n_malloc(vertexBufferSize);
        ushort* indexBuffer = (ushort*) n_malloc(indexBufferSize);

        if (!meshLoader->ReadVertices(vertexBuffer, vertexBufferSize))
        {
            return false;
        }

        if (!meshLoader->ReadIndices(indexBuffer, indexBufferSize))
        {
            return false;
        }

        // add vertices to mesh builder
        int vertexIndex;
        float* vertexPtr = vertexBuffer;
        for (vertexIndex = 0; vertexIndex < numVertices; vertexIndex++)
        {
            Vertex vertex;
            if (vertexComponents & nMesh2::Coord)
            {
                vertex.SetCoord(vector3(vertexPtr[0], vertexPtr[1], vertexPtr[2]));
                vertexPtr += 3;
            }
            if (vertexComponents & nMesh2::Normal)
            {
                vertex.SetNormal(vector3(vertexPtr[0], vertexPtr[1], vertexPtr[2]));
                vertexPtr += 3;
            }
            if (vertexComponents & nMesh2::Uv0)
            {
                vertex.SetUv(0, vector2(vertexPtr[0], vertexPtr[1]));
                vertexPtr += 2;
            }
            if (vertexComponents & nMesh2::Uv1)
            {
                vertex.SetUv(1, vector2(vertexPtr[0], vertexPtr[1]));
                vertexPtr += 2;
            }
            if (vertexComponents & nMesh2::Uv2)
            {
                vertex.SetUv(2, vector2(vertexPtr[0], vertexPtr[1]));
                vertexPtr += 2;
            }
            if (vertexComponents & nMesh2::Uv3)
            {
                vertex.SetUv(3, vector2(vertexPtr[0], vertexPtr[1]));
                vertexPtr += 2;
            }
            if (vertexComponents & nMesh2::Color)
            {
                vertex.SetColor(vector4(vertexPtr[0], vertexPtr[1], vertexPtr[2], vertexPtr[3]));
                vertexPtr += 4;
            }
            if (vertexComponents & nMesh2::Tangent)
            {
                vertex.SetTangent(vector3(vertexPtr[0], vertexPtr[1], vertexPtr[2]));
                vertexPtr += 3;
            }
            if (vertexComponents & nMesh2::Binormal)
            {
                vertex.SetBinormal(vector3(vertexPtr[0], vertexPtr[1], vertexPtr[2]));
                vertexPtr += 3;
            }
            if (vertexComponents & nMesh2::Weights)
            {
                vertex.SetWeights(vector4(vertexPtr[0], vertexPtr[1], vertexPtr[2], vertexPtr[3]));
                vertexPtr += 4;
            }
            if (vertexComponents & nMesh2::JIndices)
            {
                vertex.SetJointIndices(vector4(vertexPtr[0], vertexPtr[1], vertexPtr[2], vertexPtr[3]));
                vertexPtr += 4;
            }
            this->AddVertex(vertex);
        }

        // add triangles to mesh builder
        int groupIndex;
        ushort* indexPtr = indexBuffer;
        for (groupIndex = 0; groupIndex < numGroups; groupIndex++)
        {
            const nMeshGroup& meshGroup = meshLoader->GetGroupAt(groupIndex);
            int groupFirstIndex = meshGroup.FirstIndex;
            int groupNumIndices = meshGroup.NumIndices;
            int triIndex = 0;
            int numTris = groupNumIndices / 3;
            for (triIndex = 0; triIndex < numTris; triIndex++)
            {
                Triangle triangle;
                triangle.SetVertexIndices(indexPtr[groupFirstIndex + (triIndex * 3) + 0],
                                          indexPtr[groupFirstIndex + (triIndex * 3) + 1],
                                          indexPtr[groupFirstIndex + (triIndex * 3) + 2]);
                triangle.SetGroupId(groupIndex);
                this->AddTriangle(triangle);
            }
        }

        // cleanup
        n_free(vertexBuffer);
        n_free(indexBuffer);
        meshLoader->Close();
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
    Save as .n3d2 file.
*/
bool
nMeshBuilder::SaveN3d2(const char* filename)
{
    bool retval = false;
	/*
    
    n_assert(filename);

    // sort triangles by group id and create a group map
    this->SortTriangles();
    nArray<Group> groupMap;
    this->BuildGroupMap(groupMap);

    Data::CFileStream file;

    char lineBuffer[1024];
    if (file->Open(filename, "w"))
    {
        // write header
        file->PutS("type n3d2\n");
        sprintf(lineBuffer, "numgroups %d\n", groupMap.Size());
        file->PutS(lineBuffer);
        sprintf(lineBuffer, "numvertices %d\n", this->GetNumVertices());
        file->PutS(lineBuffer);
        sprintf(lineBuffer, "vertexwidth %d\n", this->GetVertexAt(0).GetWidth());
        file->PutS(lineBuffer);
        sprintf(lineBuffer, "numtris %d\n", this->GetNumTriangles());
        file->PutS(lineBuffer);
        sprintf(lineBuffer, "numedges %d\n", this->GetNumEdges());
        file->PutS(lineBuffer);

        // write vertex components
        sprintf(lineBuffer, "vertexcomps ");
        const Vertex& v = this->GetVertexAt(0);
        if (v.HasComponent(Vertex::COORD))
        {
            strcat(lineBuffer, "coord ");
        }
        if (v.HasComponent(Vertex::NORMAL))
        {
            strcat(lineBuffer, "normal ");
        }
        if (v.HasComponent(Vertex::UV0))
        {
            strcat(lineBuffer, "uv0 ");
        }
        if (v.HasComponent(Vertex::UV1))
        {
            strcat(lineBuffer, "uv1 ");
        }
        if (v.HasComponent(Vertex::UV2))
        {
            strcat(lineBuffer, "uv2 ");
        }
        if (v.HasComponent(Vertex::UV3))
        {
            strcat(lineBuffer, "uv3 ");
        }
        if (v.HasComponent(Vertex::COLOR))
        {
            strcat(lineBuffer, "color ");
        }
        if (v.HasComponent(Vertex::TANGENT))
        {
            strcat(lineBuffer, "tangent ");
        }
        if (v.HasComponent(Vertex::BINORMAL))
        {
            strcat(lineBuffer, "binormal ");
        }
        if (v.HasComponent(Vertex::WEIGHTS))
        {
            strcat(lineBuffer, "weights ");
        }
        if (v.HasComponent(Vertex::JINDICES))
        {
            strcat(lineBuffer, "jindices");
        }
        strcat(lineBuffer, "\n");
        file->PutS(lineBuffer);

        // write groups
        int i;
        for (i = 0; i < groupMap.Size(); i++)
        {
            const Group& group = groupMap[i];
            int firstTriangle = group.GetFirstTriangle();
            int numTriangles  = group.GetNumTriangles();
            //vertices
            int minVertexIndex, maxVertexIndex;
            this->GetGroupVertexRange(group.GetId(), minVertexIndex, maxVertexIndex);
            //edges
            int minEdgeIndex, maxEdgeIndex;
            this->GetGroupEdgeRange(group.GetId(), minEdgeIndex, maxEdgeIndex);

            sprintf(lineBuffer, "g %d %d %d %d %d %d\n",
                    minVertexIndex, (maxVertexIndex - minVertexIndex) + 1,
                    firstTriangle, numTriangles,
                    minEdgeIndex, (maxEdgeIndex - minEdgeIndex) + 1);
            file->PutS(lineBuffer);
        }

        // write vertices
        int numVertices = this->GetNumVertices();
        for (i = 0; i < numVertices; i++)
        {
            char charBuffer[128];
            sprintf(lineBuffer, "v ");
            Vertex& curVertex = this->GetVertexAt(i);
            if (curVertex.HasComponent(Vertex::COORD))
            {
                sprintf(charBuffer, "%f %f %f ", curVertex.coord.x, curVertex.coord.y, curVertex.coord.z);
                strcat(lineBuffer, charBuffer);
            }
            if (curVertex.HasComponent(Vertex::NORMAL))
            {
                sprintf(charBuffer, "%f %f %f ", curVertex.normal.x, curVertex.normal.y, curVertex.normal.z);
                strcat(lineBuffer, charBuffer);
            }
            if (curVertex.HasComponent(Vertex::UV0))
            {
                sprintf(charBuffer, "%f %f ", curVertex.uv[0].x, curVertex.uv[0].y);
                strcat(lineBuffer, charBuffer);
            }
            if (curVertex.HasComponent(Vertex::UV1))
            {
                sprintf(charBuffer, "%f %f ", curVertex.uv[1].x, curVertex.uv[1].y);
                strcat(lineBuffer, charBuffer);
            }
            if (curVertex.HasComponent(Vertex::UV2))
            {
                sprintf(charBuffer, "%f %f ", curVertex.uv[2].x, curVertex.uv[2].y);
                strcat(lineBuffer, charBuffer);
            }
            if (curVertex.HasComponent(Vertex::UV3))
            {
                sprintf(charBuffer, "%f %f ", curVertex.uv[3].x, curVertex.uv[3].y);
                strcat(lineBuffer, charBuffer);
            }
            if (curVertex.HasComponent(Vertex::COLOR))
            {
                sprintf(charBuffer, "%f %f %f %f ", curVertex.color.x, curVertex.color.y, curVertex.color.z, curVertex.color.w);
                strcat(lineBuffer, charBuffer);
            }
            if (curVertex.HasComponent(Vertex::TANGENT))
            {
                sprintf(charBuffer, "%f %f %f ", curVertex.tangent.x, curVertex.tangent.y, curVertex.tangent.z);
                strcat(lineBuffer, charBuffer);
            }
            if (curVertex.HasComponent(Vertex::BINORMAL))
            {
                sprintf(charBuffer, "%f %f %f ", curVertex.binormal.x, curVertex.binormal.y, curVertex.binormal.z);
                strcat(lineBuffer, charBuffer);
            }
            if (curVertex.HasComponent(Vertex::WEIGHTS))
            {
                sprintf(charBuffer, "%f %f %f %f ", curVertex.weights.x, curVertex.weights.y, curVertex.weights.z, curVertex.weights.w);
                strcat(lineBuffer, charBuffer);
            }
            if (curVertex.HasComponent(Vertex::JINDICES))
            {
                sprintf(charBuffer, "%f %f %f %f ", curVertex.jointIndices.x, curVertex.jointIndices.y, curVertex.jointIndices.z, curVertex.jointIndices.w);
                strcat(lineBuffer, charBuffer);
            }
            strcat(lineBuffer, "\n");
            file->PutS(lineBuffer);
        }

        // write triangles
        int numTriangles = this->GetNumTriangles();
        for (i = 0; i < numTriangles; i++)
        {
            Triangle& curTriangle = this->GetTriangleAt(i);
            int i0, i1, i2;
            curTriangle.GetVertexIndices(i0, i1, i2);
            sprintf(lineBuffer, "t %d %d %d\n", i0, i1, i2);
            file->PutS(lineBuffer);
        }

        //write edges
        int numEdges = this->GetNumEdges();
        for (i = 0; i < numEdges; i++)
        {
            GroupedEdge& curEdge = this->GetEdgeAt(i);
            sprintf(lineBuffer, "e %d %d %d %d\n", curEdge.fIndex[0], curEdge.fIndex[1], curEdge.vIndex[0], curEdge.vIndex[1]);
            file->PutS(lineBuffer);
        }

        file->Close();
        retval = true;
    }
    file->Release();
	*/
    return retval;
}

//------------------------------------------------------------------------------
/**
    Save the mesh as one or more n3d files (one n3d file per group).
*/
bool
nMeshBuilder::SaveN3d(const char* filename)
{
    
    n_assert(filename);
    bool retval = true;
	/*

    // sort triangles by group id and create a group map
    this->SortTriangles();
    nArray<Group> groupMap;
    this->BuildGroupMap(groupMap);

    // for each group...
    int curGroup;
    for (curGroup = 0; curGroup < groupMap.Size(); curGroup++)
    {
        const Group& group = groupMap[curGroup];
        Data::CFileStream* file = DataSrv->NewFileObject();
        n_assert(file);

        nString path = filename;
        path.StripExtension();
        char buffer[1024];
        sprintf(buffer, "%s_%d.n3d", path.Get(), curGroup);
        if (file->Open(buffer, "w"))
        {
            int firstTriangle = group.GetFirstTriangle();
            int numTriangles  = group.GetNumTriangles();
            int minVertexIndex, maxVertexIndex;
            this->GetGroupVertexRange(group.GetId(), minVertexIndex, maxVertexIndex);

            // write vertex coordinates
            int index;
            if (this->GetVertexAt(0).HasComponent(Vertex::COORD))
            {
                for (index = minVertexIndex; index <= maxVertexIndex; index++)
                {
                    const Vertex& vertex = this->GetVertexAt(index);
                    sprintf(buffer, "v %f %f %f\n", vertex.GetCoord().x, vertex.GetCoord().y, vertex.GetCoord().z);
                    file->PutS(buffer);
                }
            }

            // write normals
            if (this->GetVertexAt(0).HasComponent(Vertex::NORMAL))
            {
                for (index = minVertexIndex; index <= maxVertexIndex; index++)
                {
                    const Vertex& vertex = this->GetVertexAt(index);
                    sprintf(buffer, "vn %f %f %f\n", vertex.GetNormal().x, vertex.GetNormal().y, vertex.GetNormal().z);
                    file->PutS(buffer);
                }
            }

            // write texture coordinates layer 0
            if (this->GetVertexAt(0).HasComponent(Vertex::UV0))
            {
                for (index = minVertexIndex; index <= maxVertexIndex; index++)
                {
                    const Vertex& vertex = this->GetVertexAt(index);
                    sprintf(buffer, "vt %f %f\n", vertex.GetUv(0).x, vertex.GetUv(0).y);
                    file->PutS(buffer);
                }
            }

            // write texture coordinates layer 1
            if (this->GetVertexAt(0).HasComponent(Vertex::UV1))
            {
                for (index = minVertexIndex; index <= maxVertexIndex; index++)
                {
                    const Vertex& vertex = this->GetVertexAt(index);
                    sprintf(buffer, "vt1 %f %f\n", vertex.GetUv(1).x, vertex.GetUv(1).y);
                    file->PutS(buffer);
                }
            }

            // write faces
            for (index = firstTriangle; index < (firstTriangle + numTriangles); index++)
            {
                const Triangle& tri = this->GetTriangleAt(index);
                int i0, i1, i2;
                tri.GetVertexIndices(i0, i1, i2);
                i0 = (i0 - minVertexIndex) + 1;
                i1 = (i1 - minVertexIndex) + 1;
                i2 = (i2 - minVertexIndex) + 1;
                sprintf(buffer, "f %d %d %d\n", i0, i1, i2);
                file->PutS(buffer);
            }

            // close file and proceed to next group
            file->Close();
            file->Release();
        }
        else
        {
            retval = false;
        }
    }*/
    return retval;
}

//------------------------------------------------------------------------------
/**
    Load an old-style n3d file. Since n3d has no concept of triangle groups,
    one group will be created for all triangles.
*/
bool
nMeshBuilder::LoadN3d(const char* filename)
{
    /*
    n_assert(filename);

    Data::CFileStream* file = DataSrv->NewFileObject();
    n_assert(file);

    if (file->Open(filename, "r"))
    {
        int act_vn   = 0;
        int act_rgba = 0;
        int act_vt   = 0;
        int act_vt1  = 0;
        int act_vt2  = 0;
        int act_vt3  = 0;
        char line[1024];
        while (file->GetValue<nString>(line, sizeof(line)))
        {
            char *kw = strtok(line, N_WHITESPACE);
            if (kw)
            {
                if (strcmp(kw, "v") == 0)
                {
                    char *xs = strtok(NULL, N_WHITESPACE);
                    char *ys = strtok(NULL, N_WHITESPACE);
                    char *zs = strtok(NULL, N_WHITESPACE);
                    if (xs && ys && zs)
                    {
                        Vertex vertex;
                        vector3 v((float)atof(xs), (float)atof(ys), (float)atof(zs));
                        vertex.SetCoord(v);
                        this->AddVertex(vertex);
                    }
                    else
                    {
                        n_printf("Broken 'v' line in '%s'!\n", filename);
                        file->Close();
                        file->Release();
                        return false;
                    }
                }
                else if (strcmp(kw, "vn") == 0)
                {
                    char *nxs = strtok(NULL, N_WHITESPACE);
                    char *nys = strtok(NULL, N_WHITESPACE);
                    char *nzs = strtok(NULL, N_WHITESPACE);
                    if (nxs && nys && nzs)
                    {
                        vector3 v((float)atof(nxs), (float)atof(nys), (float)atof(nzs));
                        this->GetVertexAt(act_vn++).SetNormal(v);
                    }
                    else
                    {
                        n_printf("Broken 'vn' line in '%s'!\n", filename);
                        file->Close();
                        file->Release();
                        return false;
                    }
                }
                else if (strcmp(kw, "rgba")== 0)
                {
                    char *rs = strtok(NULL, N_WHITESPACE);
                    char *gs = strtok(NULL, N_WHITESPACE);
                    char *bs = strtok(NULL, N_WHITESPACE);
                    char *as = strtok(NULL, N_WHITESPACE);
                    if (rs && gs && bs && as)
                    {
                        vector4 v((float)atof(rs), (float)atof(gs), (float)atof(bs), (float)atof(as));
                        this->GetVertexAt(act_rgba++).SetColor(v);
                    }
                    else
                    {
                        n_printf("Broken 'rgba' line in '%s'!\n", filename);
                        file->Close();
                        file->Release();
                        return false;
                    }
                }
                else if (strcmp(kw, "vt") == 0)
                {
                    char *us = strtok(NULL, N_WHITESPACE);
                    char *vs = strtok(NULL, N_WHITESPACE);
                    if (us && vs)
                    {
                        vector2 v((float)atof(us), (float)atof(vs));
                        this->GetVertexAt(act_vt++).SetUv(0, v);
                    }
                    else
                    {
                        n_printf("Broken 'vt' line in '%s'!\n", filename);
                        file->Close();
                        file->Release();
                        return false;
                    }
                }
                else if (strcmp(kw, "vt1") == 0)
                {
                    char *us = strtok(NULL, N_WHITESPACE);
                    char *vs = strtok(NULL, N_WHITESPACE);
                    if (us && vs)
                    {
                        vector2 v((float)atof(us), (float)atof(vs));
                        this->GetVertexAt(act_vt1++).SetUv(1, v);
                    }
                    else
                    {
                        n_printf("Broken 'vt1' line in '%s'!\n", filename);
                        file->Close();
                        file->Release();
                        return false;
                    }
                }
                else if (strcmp(kw, "vt2") == 0)
                {
                    char *us = strtok(NULL, N_WHITESPACE);
                    char *vs = strtok(NULL, N_WHITESPACE);
                    if (us && vs)
                    {
                        vector2 v((float)atof(us), (float)atof(vs));
                        this->GetVertexAt(act_vt2++).SetUv(2, v);
                    }
                    else
                    {
                        n_printf("Broken 'vt2' line in '%s'!\n", filename);
                        file->Close();
                        file->Release();
                        return false;
                    }
                }
                else if (strcmp(kw, "vt3") == 0)
                {
                    char *us = strtok(NULL, N_WHITESPACE);
                    char *vs = strtok(NULL, N_WHITESPACE);
                    if (us && vs)
                    {
                        vector2 v((float)atof(us), (float)atof(vs));
                        this->GetVertexAt(act_vt3++).SetUv(3, v);
                    }
                    else
                    {
                        n_printf("Broken 'vt3' line in '%s'!\n", filename);
                        file->Close();
                        file->Release();
                        return false;
                    }
                }
                else if (strcmp(kw, "f") == 0)
                {
                    char* t0s = strtok(0, N_WHITESPACE);
                    char* t1s = strtok(0, N_WHITESPACE);
                    char* t2s = strtok(0, N_WHITESPACE);
                    if (t0s && t1s && t2s)
                    {
                        char *slash;
                        if ((slash=strchr(t0s, '/')))
                        {
                            *slash=0;
                        }
                        if ((slash=strchr(t1s, '/')))
                        {
                            *slash=0;
                        }
                        if ((slash=strchr(t2s, '/')))
                        {
                            *slash=0;
                        }
                        Triangle triangle;
                        triangle.SetVertexIndices(atoi(t0s) - 1, atoi(t1s) - 1, atoi(t2s) - 1);
                        triangle.SetGroupId(0);
                        this->AddTriangle(triangle);
                    }
                    else
                    {
                        n_printf("Broken 'f' line in '%s'!\n", filename);
                        file->Close();
                        file->Release();
                        return false;
                    }
                }
            }
        }

        file->Close();
        file->Release();
        return true;
    }

    file->Release();
	*/
    return false;
}

//-- vertex components ----------------------------------------------
enum nVertexType {
    N_VT_VOID  = 0,         // undefined
    N_VT_COORD = (1<<0),    // has xyz
    N_VT_NORM  = (1<<1),    // has normals
    N_VT_RGBA  = (1<<2),    // has color

    N_VT_UV0   = (1<<3),    // has texcoord set 0
    N_VT_UV1   = (1<<4),    // has texcoord set 1
    N_VT_UV2   = (1<<5),    // has texcoord set 2
    N_VT_UV3   = (1<<6),    // has texcoord set 3

    N_VT_JW    = (1<<7),    // has up to 4 joint weights per vertex
};

//------------------------------------------------------------------------------
/**
    Load .nvx file.

    -21-Jul-05    kims    created
*/
bool
nMeshBuilder::LoadNvx(const char* filename)
{
    Data::CFileStream file;

	if (!file.Open(filename, Data::SAM_READ))
    {
        n_printf("nMeshBuilder: Failed to load '%s'\n", filename);
    }

    int magicNumber;
    file.Read(&magicNumber, sizeof(int));
    if (magicNumber != 'NVX1')
    {
        n_printf("nMeshBulider: '%s' is not a NVX1 file!\n", filename);
        file.Close();
        return false;
    }

    int numVertices, numIndices, numEdges, vType, dataStart, dataSize;
    nVertexType vertexType;

    file.Read(&numVertices, sizeof(int));
    file.Read(&numIndices,  sizeof(int));
    file.Read(&numEdges,    sizeof(int));
    file.Read(&vType,       sizeof(int));
    file.Read(&dataStart,   sizeof(int));
    file.Read(&dataSize,    sizeof(int));

    vertexType = (nVertexType)vType;

    void* buffer = n_malloc(dataSize);
    file.Seek(dataStart, Data::SSO_BEGIN);
    int num = file.Read(buffer, dataSize);
    file.Close();

    char* ptr = (char*) buffer;
    vector3 vec3;
    vector2 vec2;
    int i;

#define read_elm(type) *(type*)ptr; ptr += sizeof(type);

    for (i=0; i<numVertices; i++)
    {
        Vertex vertex;

        // read vertex position
        if (vertexType & N_VT_COORD)
        {
            float x = read_elm(float);
            float y = read_elm(float);
            float z = read_elm(float);
            vec3.set(x, y, z);
            vertex.SetCoord(vec3);
        }

        // read vertex normal
        if (vertexType & N_VT_NORM)
        {
            float x = read_elm(float);
            float y = read_elm(float);
            float z = read_elm(float);
            vec3.set(x, y, z);
            vertex.SetNormal(vec3);
        }

        // read vertex color
        if (vertexType & N_VT_RGBA)
        {
            unsigned int color = read_elm(unsigned int);
            float b = ((color & 0xff) >> 24) / 255.0f;
            float g = ((color & 0xff) >> 16) / 255.0f;
            float r = ((color & 0xff) >> 8) / 255.0f;
            float a = ((color & 0xff)) / 255.0f;

            //TODO: need to check rgba?

            vertex.SetColor(vector4(b, g, r, a));
        }

        // read texture coordinates
        if (vertexType & N_VT_UV0)
        {
            float x = read_elm(float);
            float y = read_elm(float);
            vec2.set(x, y);
            vertex.SetUv(0, vec2);
        }
        if (vertexType & N_VT_UV1)
        {
            float x = read_elm(float);
            float y = read_elm(float);
            vec2.set(x, y);
            vertex.SetUv(1, vec2);
        }
        if (vertexType & N_VT_UV2)
        {
            float x = read_elm(float);
            float y = read_elm(float);
            vec2.set(x, y);
            vertex.SetUv(2, vec2);
        }
        if (vertexType & N_VT_UV3)
        {
            float x = read_elm(float);
            float y = read_elm(float);
            vec2.set(x, y);
            vertex.SetUv(3, vec2);
        }

        // read joint indices and vertex weights for skinning
        if (vertexType & N_VT_JW)
        {
            short ji0 = read_elm(short);
            short ji1 = read_elm(short);
            short ji2 = read_elm(short);
            short ji3 = read_elm(short);

            vertex.SetJointIndices(vector4(ji0, ji1, ji2, ji3));

            float w0  = read_elm(float);
            float w1  = read_elm(float);
            float w2  = read_elm(float);
            float w3  = read_elm(float);

            vertex.SetWeights(vector4(w0, w1, w2, w3));
        }

        this->AddVertex(vertex);
    }

    // skip edges
    for (i=0; i<numEdges; i++)
    {
        ushort we0 = read_elm(ushort);
        ushort we1 = read_elm(ushort);
        ushort we2 = read_elm(ushort);
        ushort we3 = read_elm(ushort);
    }

    // read triangle indices
    if (numIndices > 0)
    {
        int numTriangles = numIndices / 3;

        for (i=0; i<numTriangles; i++)
        {
            ushort i0 = read_elm(ushort);
            ushort i1 = read_elm(ushort);
            ushort i2 = read_elm(ushort);

            Triangle triangle;
            triangle.SetVertexIndices(i0, i1, i2);

            this->AddTriangle(triangle);
        }
    }

    return true;
}

//------------------------------------------------------------------------------
/**
    Load a .obj file.
    NOTE: doesn't work with lightwave exported obj files
    NOTE: works only with convex polys
*/
bool
nMeshBuilder::LoadObj(const char* filename)
{
   /* 
    n_assert(filename);

    bool retval = false;
    Data::CFileStream* file = DataSrv->NewFileObject();
    n_assert(file);

    nArray<Group> groupMap;
    int numGroups = 0;
    int curGroup = 0;
    int numVertices = 0;
    int firstTriangle = 0;
    int numTriangles = 0;
    int vertexComponents = 0;
    nArray<vector3> coordArray;
    nArray<vector3> normalArray;
    nArray<vector2> uvArray;

    Group group;
    Vertex vertex;
    if (file->Open(filename, "r"))
    {
        char line[1024];
        while (file->GetValue<nString>(line, sizeof(line)))
        {
            // get keyword
            char* keyWord = strtok(line, N_WHITESPACE);
            if (0 == keyWord)
            {
                continue;
            }
            else if ((0 == strcmp(keyWord, "g")) || (0 == strcmp(keyWord, "usemtl")))
            {
                // a triangle group
                const char* groupName = strtok(0, N_WHITESPACE);

                if (groupName)
                {
                    // Check that name is different from default
                    // Maya obj exporter adds default groups
                    // for some stupid reason.
                    if (0 != strcmp(groupName, "default"))
                    {
                        if (numGroups > 0)
                        {
                            group.SetId(curGroup++);
                            group.SetFirstTriangle(firstTriangle);
                            group.SetNumTriangles(numTriangles-firstTriangle);
                            groupMap.Append(group);
                        }

                        numGroups++;
                        firstTriangle = numTriangles;
                    }
                }
            }
            else if (0 == strcmp(keyWord, "v"))
            {
                vertexComponents |= Vertex::COORD;
                const char* xStr = strtok(0, N_WHITESPACE);
                const char* yStr = strtok(0, N_WHITESPACE);
                const char* zStr = strtok(0, N_WHITESPACE);
                n_assert(xStr && yStr && zStr);
                coordArray.Append(vector3((float) atof(xStr), (float) atof(yStr), (float) atof(zStr)));
            }
            else if (0 == strcmp(keyWord, "vn"))
            {
                vertexComponents |= Vertex::NORMAL;
                const char* iStr = strtok(0, N_WHITESPACE);
                const char* jStr = strtok(0, N_WHITESPACE);
                const char* kStr = strtok(0, N_WHITESPACE);
                n_assert(iStr && jStr && kStr);
                normalArray.Append(vector3((float) atof(iStr), (float) atof(jStr), (float) atof(kStr)));
            }
            else if (0 == strcmp(keyWord, "vt"))
            {
                vertexComponents |= Vertex::UV0;
                const char* uStr = strtok(0, N_WHITESPACE);
                const char* vStr = strtok(0, N_WHITESPACE);
                n_assert(uStr && vStr);
                uvArray.Append(vector2((float) atof(uStr), (float) atof(vStr)));
            }
            else if (0 == strcmp(keyWord, "f"))
            {
                int coordIndex;
                int normalIndex = 0;
                int uvIndex;

                char* vertexStr = strtok(0, N_WHITESPACE);
                int vertexNo;
                for (vertexNo = 0; vertexStr != NULL; vertexNo++, vertexStr=strtok(0, N_WHITESPACE))
                {
                    // unpack poly vertex info (coord/uv/normal)

                    // check if uv info exists
                    char* uvStr = strchr(vertexStr, '/');
                    if (uvStr != NULL)
                    {
                        uvStr[0] = 0;
                        uvStr++;
                        coordIndex = atoi(vertexStr);

                        // check if normal info exits
                        char* normalStr = strchr(uvStr, '/');
                        if (normalStr != NULL)
                        {
                            normalStr[0] = 0;
                            normalStr++;

                            normalIndex = 0;
                            if (normalStr[0] != 0)
                            {
                                normalIndex = atoi(normalStr);
                            }
                        }

                        uvIndex = 0;
                        if (uvStr[0] != 0)
                            uvIndex = atoi(uvStr);
                    }
                    else
                    {
                        // no uv and normal info found so presume
                        // that the value is used for all of them
                        coordIndex = atoi(vertexStr);
                        normalIndex = 100;//coordIndex;
                        uvIndex = coordIndex;
                    }

                    // if index is negative then calculate positive index
                    if (coordIndex < 0)
                    {
                        coordIndex += coordArray.Size() + 1;
                    }
                    if (uvIndex < 0)
                    {
                        uvIndex += uvArray.Size() + 1;
                    }
                    if (normalIndex < 0)
                    {
                        normalIndex += normalArray.Size() + 1;
                    }

                    // obj files first index is 1 when in c++ it's 0
                    coordIndex--;
                    uvIndex--;
                    normalIndex--;

                    Vertex vertex;
                    if (vertexComponents & Vertex::COORD)
                    {
                        vertex.SetCoord(coordArray[coordIndex]);
                    }
                    if (vertexComponents & Vertex::NORMAL)
                    {
                        vertex.SetNormal(normalArray[normalIndex]);
                    }
                    if (vertexComponents & Vertex::UV0)
                    {
                        vertex.SetUv(0, uvArray[uvIndex]);
                    }
                    this->AddVertex(vertex);
                    numVertices++;
                }

                // create triangles from convex polygon
                int firstTriangleVertex = numVertices - vertexNo;
                int lastTriangleVertex = firstTriangleVertex + 2;
                for (; lastTriangleVertex < (numVertices); lastTriangleVertex++)
                {
                    Triangle triangle;
                    triangle.SetVertexIndices(firstTriangleVertex, lastTriangleVertex-1, lastTriangleVertex);
                    this->AddTriangle(triangle);
                    numTriangles++;
                }
            }
        }

        if (firstTriangle < numTriangles)
        {
            group.SetId(curGroup++);
            group.SetFirstTriangle(firstTriangle);
            group.SetNumTriangles(numTriangles-firstTriangle);
            groupMap.Append(group);
            numGroups++;
        }

        file->Close();
        retval = true;
    }
    file->Release();

    this->BuildTriangleNormals();

    // update the triangle group ids from the group map
    this->UpdateTriangleIds(groupMap);
	*/
    return false; //retval;
}

//------------------------------------------------------------------------------
/**
    The old n3d2 load code, so you can convert your data.
*/
bool
nMeshBuilder::LoadOldN3d2(const char* filename)
{
    /*
    n_assert(filename);

    bool retval = false;
    Data::CFileStream* file = DataSrv->NewFileObject();
    n_assert(file);

    nArray<Group> groupMap;
    int numGroups = 0;
    int numVertices = 0;
    int vertexWidth = 0;
    int numTriangles = 0;
    int curTriangle = 0;
    int curGroup = 0;
    int vertexComponents = 0;
    if (file->Open(filename, "r"))
    {
        char line[1024];
        while (file->GetValue<nString>(line, sizeof(line)))
        {
            // get keyword
            char* keyWord = strtok(line, N_WHITESPACE);
            if (0 == keyWord)
            {
                continue;
            }
            else if (0 == strcmp(keyWord, "type"))
            {
                // type must be 'n3d2'
                char* typeString = strtok(0, N_WHITESPACE);
                n_assert(typeString);
                if (0 != strcmp(typeString, "n3d2"))
                {
                    n_printf("nMeshBuilder::Load(%s): Invalid type '%s', must be 'n3d2'\n", filename, typeString);
                    file->Close();
                    file->Release();
                    return false;
                }
            }
            else if (0 == strcmp(keyWord, "numgroups"))
            {
                char* numGroupsString = strtok(0, N_WHITESPACE);
                n_assert(numGroupsString);
                numGroups = atoi(numGroupsString);
            }
            else if (0 == strcmp(keyWord, "numvertices"))
            {
                char* numVerticesString = strtok(0, N_WHITESPACE);
                n_assert(numVerticesString);
                numVertices = atoi(numVerticesString);
            }
            else if (0 == strcmp(keyWord, "vertexwidth"))
            {
                char* vertexWidthString = strtok(0, N_WHITESPACE);
                n_assert(vertexWidthString);
                vertexWidth = atoi(vertexWidthString);
            }
            else if (0 == strcmp(keyWord, "numtris"))
            {
                char* numTrianglesString = strtok(0, N_WHITESPACE);
                n_assert(numTrianglesString);
                numTriangles = atoi(numTrianglesString);
            }
            else if (0 == strcmp(keyWord, "vertexcomps"))
            {
                char* str;
                while ((str = strtok(0, N_WHITESPACE)))
                {
                    if (0 == strcmp(str, "coord"))
                    {
                        vertexComponents |= Vertex::COORD;
                    }
                    else if (0 == strcmp(str, "normal"))
                    {
                        vertexComponents |= Vertex::NORMAL;
                    }
                    else if (0 == strcmp(str, "tangent"))
                    {
                        vertexComponents |= Vertex::TANGENT;
                    }
                    else if (0 == strcmp(str, "binormal"))
                    {
                        vertexComponents |= Vertex::BINORMAL;
                    }
                    else if (0 == strcmp(str, "color"))
                    {
                        vertexComponents |= Vertex::COLOR;
                    }
                    else if (0 == strcmp(str, "uv0"))
                    {
                        vertexComponents |= Vertex::UV0;
                    }
                    else if (0 == strcmp(str, "uv1"))
                    {
                        vertexComponents |= Vertex::UV1;
                    }
                    else if (0 == strcmp(str, "uv2"))
                    {
                        vertexComponents |= Vertex::UV2;
                    }
                    else if (0 == strcmp(str, "uv3"))
                    {
                        vertexComponents |= Vertex::UV3;
                    }
                    else if (0 == strcmp(str, "weights"))
                    {
                        vertexComponents |= Vertex::WEIGHTS;
                    }
                    else if (0 == strcmp(str, "jindices"))
                    {
                        vertexComponents |= Vertex::JINDICES;
                    }
                    else
                    {
                        n_printf("nMeshBuilder::Load(%s): Invalid vertex component '%s'\n", filename, str);
                        file->Close();
                        file->Release();
                        return false;
                    }
                }
            }
            else if (0 == strcmp(keyWord, "g"))
            {
                // a triangle group
                strtok(0, N_WHITESPACE);    // firstVertex
                strtok(0, N_WHITESPACE);    // numVertices
                const char* firstTriangleString = strtok(0, N_WHITESPACE);
                const char* numTriangleString   = strtok(0, N_WHITESPACE);

                n_assert(firstTriangleString);
                n_assert(numTriangleString);

                Group group;
                group.SetId(curGroup++);
                group.SetFirstTriangle(atoi(firstTriangleString));
                group.SetNumTriangles(atoi(numTriangleString));
                groupMap.Append(group);
            }
            else if (0 == strcmp(keyWord, "v"))
            {
                // a vertex
                n_assert(vertexComponents != 0);
                Vertex vertex;
                if (vertexComponents & Vertex::COORD)
                {
                    const char* xStr = strtok(0, N_WHITESPACE);
                    const char* yStr = strtok(0, N_WHITESPACE);
                    const char* zStr = strtok(0, N_WHITESPACE);
                    n_assert(xStr && yStr && zStr);
                    vertex.SetCoord(vector3((float) atof(xStr), (float) atof(yStr), (float) atof(zStr)));
                }
                if (vertexComponents & Vertex::NORMAL)
                {
                    const char* xStr = strtok(0, N_WHITESPACE);
                    const char* yStr = strtok(0, N_WHITESPACE);
                    const char* zStr = strtok(0, N_WHITESPACE);
                    n_assert(xStr && yStr && zStr);
                    vertex.SetNormal(vector3((float) atof(xStr), (float) atof(yStr), (float) atof(zStr)));
                }
                if (vertexComponents & Vertex::TANGENT)
                {
                    const char* xStr = strtok(0, N_WHITESPACE);
                    const char* yStr = strtok(0, N_WHITESPACE);
                    const char* zStr = strtok(0, N_WHITESPACE);
                    n_assert(xStr && yStr && zStr);
                    vertex.SetTangent(vector3((float) atof(xStr), (float) atof(yStr), (float) atof(zStr)));
                }
                if (vertexComponents & Vertex::BINORMAL)
                {
                    const char* xStr = strtok(0, N_WHITESPACE);
                    const char* yStr = strtok(0, N_WHITESPACE);
                    const char* zStr = strtok(0, N_WHITESPACE);
                    n_assert(xStr && yStr && zStr);
                    vertex.SetBinormal(vector3((float) atof(xStr), (float) atof(yStr), (float) atof(zStr)));
                }
                if (vertexComponents & Vertex::COLOR)
                {
                    const char* rStr = strtok(0, N_WHITESPACE);
                    const char* gStr = strtok(0, N_WHITESPACE);
                    const char* bStr = strtok(0, N_WHITESPACE);
                    const char* aStr = strtok(0, N_WHITESPACE);
                    n_assert(rStr && gStr && bStr && aStr);
                    vertex.SetColor(vector4((float) atof(aStr), (float) atof(gStr), (float) atof(bStr), (float) atof(aStr)));
                }
                if (vertexComponents & Vertex::UV0)
                {
                    const char* uStr = strtok(0, N_WHITESPACE);
                    const char* vStr = strtok(0, N_WHITESPACE);
                    n_assert(uStr && vStr);
                    vertex.SetUv(0, vector2((float) atof(uStr), (float) atof(vStr)));
                }
                if (vertexComponents & Vertex::UV1)
                {
                    const char* uStr = strtok(0, N_WHITESPACE);
                    const char* vStr = strtok(0, N_WHITESPACE);
                    n_assert(uStr && vStr);
                    vertex.SetUv(1, vector2((float) atof(uStr), (float) atof(vStr)));
                }
                if (vertexComponents & Vertex::UV2)
                {
                    const char* uStr = strtok(0, N_WHITESPACE);
                    const char* vStr = strtok(0, N_WHITESPACE);
                    n_assert(uStr && vStr);
                    vertex.SetUv(2, vector2((float) atof(uStr), (float) atof(vStr)));
                }
                if (vertexComponents & Vertex::UV3)
                {
                    const char* uStr = strtok(0, N_WHITESPACE);
                    const char* vStr = strtok(0, N_WHITESPACE);
                    n_assert(uStr && vStr);
                    vertex.SetUv(3, vector2((float) atof(uStr), (float) atof(vStr)));
                }
                if (vertexComponents & Vertex::WEIGHTS)
                {
                    const char* xStr = strtok(0, N_WHITESPACE);
                    const char* yStr = strtok(0, N_WHITESPACE);
                    const char* zStr = strtok(0, N_WHITESPACE);
                    const char* wStr = strtok(0, N_WHITESPACE);
                    n_assert(xStr && yStr && zStr && wStr);
                    vertex.SetWeights(vector4(float(atof(xStr)), float(atof(yStr)), float(atof(zStr)), float(atof(wStr))));
                }
                if (vertexComponents & Vertex::JINDICES)
                {
                    const char* xStr = strtok(0, N_WHITESPACE);
                    const char* yStr = strtok(0, N_WHITESPACE);
                    const char* zStr = strtok(0, N_WHITESPACE);
                    const char* wStr = strtok(0, N_WHITESPACE);
                    n_assert(xStr && yStr && zStr && wStr);
                    vertex.SetJointIndices(vector4(float(atof(xStr))/3, float(atof(yStr))/3, float(atof(zStr))/3, float(atof(wStr))/3));
                }
                this->AddVertex(vertex);
            }
            else if (0 == strcmp(keyWord, "t"))
            {
                // a triangle
                const char* i0Str = strtok(0, N_WHITESPACE);
                const char* i1Str = strtok(0, N_WHITESPACE);
                const char* i2Str = strtok(0, N_WHITESPACE);
                Triangle triangle;
                triangle.SetVertexIndices(atoi(i0Str), atoi(i1Str), atoi(i2Str));
                curTriangle++;
                this->AddTriangle(triangle);
            }
        }
        file->Close();
        retval = true;
    }
    file->Release();

    // update the triangle group ids from the group map
    this->UpdateTriangleIds(groupMap);
    return retval;
	*/
return false;
}

