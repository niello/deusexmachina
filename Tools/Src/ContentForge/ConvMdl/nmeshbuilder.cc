//------------------------------------------------------------------------------
//  nmeshbuilder.cc
//  (C) 2002 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "nmeshbuilder.h"

//------------------------------------------------------------------------------
/**
*/
nMeshBuilder::nMeshBuilder() :
    vertexArray(1 << 15, 1 << 15),
    triangleArray(1 << 15, 1 << 15),
    edgeArray(1 << 15, 1 << 15)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
nMeshBuilder::~nMeshBuilder()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Count the number of triangles matching a group id and material id starting
    at a given triangle index. Will stop on first triangle which doesn't
    match the group id.
*/
int
nMeshBuilder::GetNumGroupTriangles(int groupId, int materialId, int usageFlags, int startTriangleIndex) const
{
    int triIndex;
    int maxTriIndex = this->triangleArray.Size();
    int numTris = 0;
    for (triIndex = startTriangleIndex; triIndex < maxTriIndex; triIndex++)
    {
        if ((this->triangleArray[triIndex].GetGroupId() == groupId) &&
            (this->triangleArray[triIndex].GetMaterialId() == materialId) &&
            (this->triangleArray[triIndex].GetUsageFlags() == usageFlags))
        {
            numTris++;
        }
    }
    return numTris;
}

//------------------------------------------------------------------------------
/**
    Get the first triangle matching a group id and material id.
*/
int
nMeshBuilder::GetFirstGroupTriangle(int groupId, int materialId, int usageFlags) const
{
    int triIndex;
    int maxTriIndex = this->triangleArray.Size();
    for (triIndex = 0; triIndex < maxTriIndex; triIndex++)
    {
        if ((this->triangleArray[triIndex].GetGroupId() == groupId) &&
            (this->triangleArray[triIndex].GetMaterialId() == materialId) &&
            (this->triangleArray[triIndex].GetUsageFlags() == usageFlags))
        {
            return triIndex;
        }
    }
    // fallthrough: can't happen
    n_assert(false);
    return -1;
}

//------------------------------------------------------------------------------
/**
    Build a group map. The triangle array must be sorted for this method
    to work. For each distinctive group id, a map entry will be
    created which contains the group id, the first triangle and
    the number of triangles in the group.
*/
void
nMeshBuilder::BuildGroupMap(nArray<Group>& groupArray)
{
    int triIndex = 0;
    int numTriangles = this->triangleArray.Size();
    Group newGroup;
    while (triIndex < numTriangles)
    {
        const Triangle& tri = this->GetTriangleAt(triIndex);
        int groupId    = tri.GetGroupId();
        int matId      = tri.GetMaterialId();
        int usageFlags = tri.GetUsageFlags();
        int numTrisInGroup = this->GetNumGroupTriangles(groupId, matId, usageFlags, triIndex);
        n_assert(numTrisInGroup > 0);
        newGroup.SetId(groupId);
        newGroup.SetMaterialId(matId);
        newGroup.SetUsageFlags(usageFlags);
        newGroup.SetFirstTriangle(triIndex);
        newGroup.SetNumTriangles(numTrisInGroup);
        groupArray.Append(newGroup);
        triIndex += numTrisInGroup;
    }
}

//------------------------------------------------------------------------------
/**
    Update the triangle group id's, material id's and usage flags
    from an existing group map.
*/
void
nMeshBuilder::UpdateTriangleIds(const nArray<Group>& groupMap)
{
    int groupIndex;
    int numGroups = groupMap.Size();
    for (groupIndex = 0; groupIndex < numGroups; groupIndex++)
    {
        const Group& group = groupMap[groupIndex];
        int triIndex = group.GetFirstTriangle();
        int maxTriIndex = triIndex + group.GetNumTriangles();
        while (triIndex < maxTriIndex)
        {
            Triangle& tri = this->GetTriangleAt(triIndex++);
            tri.SetGroupId(group.GetId());
            tri.SetMaterialId(group.GetMaterialId());
            tri.SetUsageFlags(group.GetUsageFlags());
        }
    }
}

//------------------------------------------------------------------------------
/**
    Clear all buffers.
*/
void
nMeshBuilder::Clear()
{
    this->vertexArray.Clear();
    this->triangleArray.Clear();
    this->edgeArray.Clear();
}

//------------------------------------------------------------------------------
/**
    Transform vertices in mesh, affects coordinates, normals and tangents.
*/
void
nMeshBuilder::Transform(const matrix44& m44)
{
    // build a normal transformation matrix (rotation only)
    vector3 xrot = m44.x_component();
    vector3 yrot = m44.y_component();
    vector3 zrot = m44.z_component();
    xrot.norm();
    yrot.norm();
    zrot.norm();
    matrix33 m33(xrot, yrot, zrot);

    vector3 v;
    int num = this->vertexArray.Size();
    int i;

    // transform vertices
    for (i = 0; i < num; i++)
    {
        this->vertexArray[i].Transform(m44, m33);
    }
}

//------------------------------------------------------------------------------
/**
    Build triangle neighbor data - edges:

    1 - create TempEdge data, 2 vertexIndex + faceIndex, where vertexIndex1 < vertexIndex2
        for every face and store in array for sorting.
        [tempMEM: numFaces * 3 * 3 * ushort]

    2 - sort TempEdge data by: groupID vertexIndex1 vertexIndex2 (TempEdgeSorter)

    3 - create GroupedEdges, divided by groupedID, merge the 2 tempEdge halfs to a complete edge, or
        marked border edges with a face index of -1;

    4 - sort GroupedEdges by: GroupID faceIndex1 faceIndex2 (GroupedEdgeSorter)

    5 - copy all edges to the edge array, skip identical edges (groupID/face).
*/
void
nMeshBuilder::CreateEdges()
{
    n_assert(0 == this->GetNumEdges());
    n_assert(this->GetNumTriangles() > 0);

    // sort the triangles, to work with the same data that is written later.
    this->SortTriangles();

    const int numEdges = this->triangleArray.Size() * 3;
    TempEdge* tempEdgeArray = n_new_array(TempEdge, numEdges);
    n_assert(tempEdgeArray);

    // generate edge data from all faces
    int i;
    int edgeIndex = 0;
    for (i = 0; i < this->triangleArray.Size(); i++)
    {
        Triangle& tri = this->GetTriangleAt(i);
        int i0, i1, i2;
        tri.GetVertexIndices(i0, i1, i2);
        int GroupID = tri.GetGroupId();

        //edge i0 - i1
        n_assert(edgeIndex < numEdges);
        tempEdgeArray[edgeIndex].fIndex = (short)i;
        tempEdgeArray[edgeIndex].vIndex[0] = i0;
        tempEdgeArray[edgeIndex].vIndex[1] = i1;
        tempEdgeArray[edgeIndex].GroupID = GroupID;
        edgeIndex++;

        //edge i1 - i2
        n_assert(edgeIndex < numEdges);
        tempEdgeArray[edgeIndex].fIndex = (short)i;
        tempEdgeArray[edgeIndex].vIndex[0] = i1;
        tempEdgeArray[edgeIndex].vIndex[1] = i2;
        tempEdgeArray[edgeIndex].GroupID = GroupID;
        edgeIndex++;

        //edge i2 - i0
        n_assert(edgeIndex < numEdges);
        tempEdgeArray[edgeIndex].fIndex = (short)i;
        tempEdgeArray[edgeIndex].vIndex[0] = i2;
        tempEdgeArray[edgeIndex].vIndex[1] = i0;
        tempEdgeArray[edgeIndex].GroupID = GroupID;
        edgeIndex++;
    }

    //sort the temp edges array
    qsort(tempEdgeArray, numEdges, sizeof(TempEdge), nMeshBuilder::TempEdgeSorter);

    // create array for the sorted edges
    GroupedEdge* groupedEdgeArray = n_new_array(GroupedEdge, numEdges);
    int groupedEdgeIndex = 0;

    // create the sorted edges array
    for (i = 0; i < numEdges - 1; i++)
    {
        const TempEdge& currEdge = tempEdgeArray[i];
        const TempEdge& nextEdge = tempEdgeArray[i+1];

        if (currEdge.vIndex[0] == nextEdge.vIndex[1] && currEdge.vIndex[1] == nextEdge.vIndex[0] && currEdge.GroupID == nextEdge.GroupID)
        {
            //current edge and next edge share the same vertex indices (cross compare) - copy only once
            GroupedEdge& edge =  groupedEdgeArray[groupedEdgeIndex++];
            edge.GroupID   = currEdge.GroupID;
            edge.vIndex[0] = currEdge.vIndex[0];
            edge.vIndex[1] = currEdge.vIndex[1];
            edge.fIndex[0] = currEdge.fIndex;
            edge.fIndex[1] = nextEdge.fIndex;

            //skip next element because we handled it already
            i++;
        }
        else
        {
            //must be a edge with only used by only one face
            GroupedEdge& edge =  groupedEdgeArray[groupedEdgeIndex++];
            edge.GroupID   = currEdge.GroupID;
            edge.vIndex[0] = currEdge.vIndex[0];
            edge.vIndex[1] = currEdge.vIndex[1];
            edge.fIndex[0] = currEdge.fIndex;
            edge.fIndex[1] = nMesh2::InvalidIndex;
        }
    }

    //fix last element
    const TempEdge& prevEdge = tempEdgeArray[numEdges-2];
    const TempEdge& currEdge = tempEdgeArray[numEdges-1];
    if (!(currEdge.vIndex[0] == prevEdge.vIndex[1] && currEdge.vIndex[1] == prevEdge.vIndex[0]))
    {
        // if the last and the previous are not the same than the last must be added, else this was handled before
        //must be a edge with only used by only one face
        GroupedEdge& edge =  groupedEdgeArray[groupedEdgeIndex++];
        edge.GroupID   = currEdge.GroupID;
        edge.vIndex[0] = currEdge.vIndex[0];
        edge.vIndex[1] = currEdge.vIndex[1];
        edge.fIndex[0] = currEdge.fIndex;
        edge.fIndex[1] = nMesh2::InvalidIndex;
    }

    // cleanup done data
    n_delete_array(tempEdgeArray);

    // sort grouped edges array
    qsort(groupedEdgeArray, groupedEdgeIndex, sizeof(GroupedEdge), nMeshBuilder::GroupedEdgeSorter);

    // all edges that are use from triangles with the same groupID are now in continuous chunks
    // remove duplicate edges when in the same group
    // don't skip duplicate edges when they are used from triangles with different groups

    //do the 1st element
    this->edgeArray.PushBack(groupedEdgeArray[0]);
    for (i = 1; i < groupedEdgeIndex; i++)
    {
        const GroupedEdge& e0 = groupedEdgeArray[i-1];
        const GroupedEdge& e1 = groupedEdgeArray[i];

        if (e1.GroupID == e0.GroupID)
        {
            if (e1.fIndex[0] == e0.fIndex[0])
            {
                if (e1.fIndex[1] == e0.fIndex[1])
                {
                    if (e1.vIndex[0] == e0.vIndex[0])
                    {
                        if (e1.vIndex[1] == e0.vIndex[1])
                        {
                            //skip this because it's identical to the previous
                            continue;
                        }
                    }
                }
            }
        }

        //fall through - e0 and e1 are different
        this->edgeArray.PushBack(e1);
    }
    n_delete_array(groupedEdgeArray);
}

//------------------------------------------------------------------------------
/**
    qsort() hook for CreateEdges() - sort tempEdges

    the compare is done cross, because if 2 faces share the same edge, than
    the index order for the edge from face1 is i0 - i1,
    but for face2 the order is always i1 - i0.
    sort by:
    - groupID
    - 1st vertex index (the 1st vertex index of the tempEdge is the smaller one)
    - 2nd vertex index (the 2nd vertex index of the tempEdge is the greater one)
    - force a definitive order: vIndex[0]
*/
int
__cdecl
nMeshBuilder::TempEdgeSorter(const void* elm0, const void* elm1)
{
    TempEdge e0 = *(TempEdge*)elm0;
    TempEdge e1 = *(TempEdge*)elm1;

    // group ID
    if (e0.GroupID < e1.GroupID)        return -1;
    else if (e0.GroupID > e1.GroupID)   return +1;
    else
    {
        // sort by lower index
        ushort e0i0, e0i1;
        ushort e1i0, e1i1;
        if (e0.vIndex[0] < e0.vIndex[1])
        {
            e0i0 = e0.vIndex[0]; e0i1 = e0.vIndex[1];
        }
        else
        {
            e0i0 = e0.vIndex[1]; e0i1 = e0.vIndex[0];
        }

        if (e1.vIndex[0] < e1.vIndex[1])
        {
            e1i0 = e1.vIndex[0]; e1i1 = e1.vIndex[1];
        }
        else
        {
            e1i0 = e1.vIndex[1]; e1i1 = e1.vIndex[0];
        }

        if (e0i0 < e1i0)            return -1;
        else if (e0i0 > e1i0)       return +1;
        else
        {
            if (e0i1 < e1i1)        return -1;
            else if (e0i1 > e1i1)   return +1;
            else
            {
                // force lower index be first
                if (e0.vIndex[0] < e1.vIndex[0])        return -1;
                else if (e0.vIndex[0] > e1.vIndex[0])   return +1;
            }
        }
    }
    return 0;
}

//------------------------------------------------------------------------------
/**
    qsort() hook for CreateEdges() - sort GroupedEdge
    - groupID
    - 1st vertex index (the 1st vertex index of the tempEdge is the smaller one)
    - 2nd vertex index (the 2nd vertex index of the tempEdge is the greater one)
    - 1st face index
    - 2nd face index
*/
int
__cdecl
nMeshBuilder::GroupedEdgeSorter(const void* elm0, const void* elm1)
{
    GroupedEdge e0 = *(GroupedEdge*)elm0;
    GroupedEdge e1 = *(GroupedEdge*)elm1;

    //first sort by groupID
    if (e0.GroupID < e1.GroupID)        return -1;
    else if (e0.GroupID > e1.GroupID)   return +1;
    else
    {
        ushort e0i0, e0i1;
        ushort e1i0, e1i1;
        if (e0.vIndex[0] < e0.vIndex[1])
        {
            e0i0 = e0.vIndex[0]; e0i1 = e0.vIndex[1];
        }
        else
        {
            e0i0 = e0.vIndex[1]; e0i1 = e0.vIndex[0];
        }

        if (e1.vIndex[0] < e1.vIndex[1])
        {
            e1i0 = e1.vIndex[0]; e1i1 = e1.vIndex[1];
        }
        else
        {
            e1i0 = e1.vIndex[1]; e1i1 = e1.vIndex[0];
        }

        if (e0i0 < e1i0)            return -1;
        else if (e0i0 > e1i0)       return +1;
        else
        {
            if (e0i1 < e1i1)        return -1;
            else if (e0i1 > e1i1)   return +1;
            else
            {
                // sort by 1st face index
                if (e0.fIndex[0] < e1.fIndex[0])      return -1;
                else if (e0.fIndex[0] > e1.fIndex[0]) return +1;
                else
                {
                    // sort by 2nd face index
                    if (e0.fIndex[1] < e1.fIndex[1])      return -1;
                    else if (e0.fIndex[1] > e1.fIndex[1]) return +1;
                }
            }
        }
    }
    return 0;
}

//------------------------------------------------------------------------------
/**
    qsort() hook for Cleanup() method.
*/
nMeshBuilder* nMeshBuilder::qsortData = 0;

int
__cdecl
nMeshBuilder::VertexSorter(const void* elm0, const void* elm1)
{
    nMeshBuilder* meshBuilder = qsortData;
    int i0 = *(int*)elm0;
    int i1 = *(int*)elm1;
    Vertex& v0 = meshBuilder->GetVertexAt(i0);
    Vertex& v1 = meshBuilder->GetVertexAt(i1);
    return v0.Compare(v1);
}

//------------------------------------------------------------------------------
/**
    This returns a value suitable for sorting, -1 if the rhs is 'smaller',
    0 if rhs is equal, and +1 if rhs is 'greater'.
*/
int
nMeshBuilder::Vertex::Compare(const Vertex& rhs) const
{
    if (this->HasComponent(COORD) && rhs.HasComponent(COORD))
    {
        int res = this->coord.compare(rhs.coord, 0.0001f);
        if (0 != res)
        {
            return res;
        }
    }
    if (this->HasComponent(NORMAL) && rhs.HasComponent(NORMAL))
    {
        int res = this->normal.compare(rhs.normal, 0.001f);
        if (0 != res)
        {
            return res;
        }
    }
    if (this->HasComponent(TANGENT) && rhs.HasComponent(TANGENT))
    {
        int res = this->tangent.compare(rhs.tangent, 0.001f);
        if (0 != res)
        {
            return res;
        }
    }
    if (this->HasComponent(BINORMAL) && rhs.HasComponent(BINORMAL))
    {
        int res = this->binormal.compare(rhs.binormal, 0.001f);
        if (0 != res)
        {
            return res;
        }
    }
    if (this->HasComponent(COLOR) && rhs.HasComponent(COLOR))
    {
        int res = this->color.compare(rhs.color, 0.001f);
        if (0 != res)
        {
            return res;
        }
    }
    if (this->HasComponent(UV0) && rhs.HasComponent(UV0))
    {
        int res = this->uv[0].compare(rhs.uv[0], 0.000001f);
        if (0 != res)
        {
            return res;
        }
    }
    if (this->HasComponent(UV1) && rhs.HasComponent(UV1))
    {
        int res = this->uv[1].compare(rhs.uv[1], 0.000001f);
        if (0 != res)
        {
            return res;
        }
    }
    if (this->HasComponent(UV2) && rhs.HasComponent(UV2))
    {
        int res = this->uv[2].compare(rhs.uv[2], 0.000001f);
        if (0 != res)
        {
            return res;
        }
    }
    if (this->HasComponent(UV3) && rhs.HasComponent(UV3))
    {
        int res = this->uv[3].compare(rhs.uv[3], 0.000001f);
        if (0 != res)
        {
            return res;
        }
    }
    if (this->HasComponent(WEIGHTS) && rhs.HasComponent(WEIGHTS))
    {
        int res = this->weights.compare(rhs.weights, 0.00001f);
        if (0 != res)
        {
            return res;
        }
    }
    if (this->HasComponent(JINDICES) && rhs.HasComponent(JINDICES))
    {
        int res = this->jointIndices.compare(rhs.jointIndices, 0.5f);
        if (0 != res)
        {
            return res;
        }
    }

    // fallthrough: all equal
    return 0;
}

//------------------------------------------------------------------------------
/**
    Cleanup the mesh. This removes redundant vertices and optionally record
    the collapse history into a client-provided collapseMap. The collapse map
    contains at each new vertex index the 'old' vertex indices which have
    been collapsed into the new vertex.

    30-Jan-03   floh    optimizations
*/
void
nMeshBuilder::Cleanup(nArray< nArray<int> >* collapseMap)
{
    int numVertices = this->vertexArray.Size();

    // generate a index remapping table and sorted vertex array
    int* indexMap = n_new_array(int, numVertices);
    int* sortMap  = n_new_array(int, numVertices);
    int* shiftMap = n_new_array(int, numVertices);
    int i;
    for (i = 0; i < numVertices; i++)
    {
        indexMap[i] = i;
        sortMap[i]  = i;
    }

    // generate a sorted index map (sort by X coordinate)
    qsortData = this;
    qsort(sortMap, numVertices, sizeof(int), nMeshBuilder::VertexSorter);

    // search sorted array for redundant vertices
    int baseIndex = 0;
    for (baseIndex = 0; baseIndex < (numVertices - 1);)
    {
        int nextIndex = baseIndex + 1;
        while ((nextIndex < numVertices) &&
               (this->vertexArray[sortMap[baseIndex]] == this->vertexArray[sortMap[nextIndex]]))
        {
            // mark the vertex as invalid
            this->vertexArray[sortMap[nextIndex]].SetFlag(Vertex::REDUNDANT);

            // put the new valid index into the index remapping table
            indexMap[sortMap[nextIndex]] = sortMap[baseIndex];
            nextIndex++;
        }
        baseIndex = nextIndex;
    }

    // fill the shiftMap, this contains for each vertex index the number
    // of invalid vertices in front of it
    int numInvalid = 0;
    int vertexIndex;
    for (vertexIndex = 0; vertexIndex < numVertices; vertexIndex++)
    {
        if (this->vertexArray[vertexIndex].CheckFlag(Vertex::REDUNDANT))
        {
            numInvalid++;
        }
        shiftMap[vertexIndex] = numInvalid;
    }

    // fix the triangle's vertex indices, first, remap the old index to a
    // valid index from the indexMap, then decrement by the shiftMap entry
    // at that index (which contains the number of invalid vertices in front
    // of that index)
    // fix vertex indices in triangles
    int numTriangles = this->triangleArray.Size();
    int curTriangle;
    for (curTriangle = 0; curTriangle < numTriangles; curTriangle++)
    {
        Triangle& t = this->triangleArray[curTriangle];
        for (i = 0; i < 3; i++)
        {
            int newIndex = indexMap[t.vertexIndex[i]];
            t.vertexIndex[i] = newIndex - shiftMap[newIndex];
        }
    }

    // initialize the collapse map so that for each new (collapsed)
    // index it contains a list of old vertex indices which have been
    // collapsed into the new vertex
    if (collapseMap)
    {
        for (i = 0; i < numVertices; i++)
        {
            int newIndex = indexMap[i];
            int collapsedIndex = newIndex - shiftMap[newIndex];
            collapseMap->At(collapsedIndex).Append(i);
        }
    }

    // finally, remove the redundant vertices
    numVertices = this->vertexArray.Size();
    nArray<Vertex> newArray(numVertices, numVertices);
    for (vertexIndex = 0; vertexIndex < numVertices; vertexIndex++)
    {
        if (!this->vertexArray[vertexIndex].CheckFlag(Vertex::REDUNDANT))
        {
            newArray.Append(vertexArray[vertexIndex]);
        }
    }
    this->vertexArray = newArray;

    // cleanup
    n_delete_array(indexMap);
    n_delete_array(sortMap);
    n_delete_array(shiftMap);
}

//------------------------------------------------------------------------------
/**
    qsort() hook for PackTrianglesByGroup().
*/
int
__cdecl
nMeshBuilder::TriangleGroupSorter(const void* elm0, const void* elm1)
{
    Triangle* t0 = (Triangle*) elm0;
    Triangle* t1 = (Triangle*) elm1;
    int groupDiff = t0->GetGroupId() - t1->GetGroupId();
    if (0 != groupDiff)
    {
        return groupDiff;
    }
    int materialDiff = t0->GetMaterialId() - t1->GetMaterialId();
    if (0 != materialDiff)
    {
        return materialDiff;
    }
    int usageDiff = t0->GetUsageFlags() - t1->GetUsageFlags();
    if (0 != usageDiff)
    {
        return usageDiff;
    }

    // make the sort order definitive
    int t0i0, t0i1, t0i2, t1i0, t1i1, t1i2;
    t0->GetVertexIndices(t0i0, t0i1, t0i2);
    t1->GetVertexIndices(t1i0, t1i1, t1i2);

    if (t0i0 < t1i0)        return -1;
    else if (t0i0 > t1i0)   return +1;
    else
    {
        if (t0i1 < t1i1)        return -1;
        else if (t0i1 > t1i1)   return +1;
        else
        {
            if (t0i2 < t1i2)        return -1;
            else if (t0i2 > t1i2)   return +1;
        }
    }

    // FIXME: For some reason it can happen that 2 triangles
    // with identical indices are exported. Make sure we
    // still get a definitive sorting order.
    // (however, duplicate triangles should not be generated
    // in the first place).
    return (t1 - t0);
}

//------------------------------------------------------------------------------
/**
    Sort triangles by group.
*/
void
nMeshBuilder::SortTriangles()
{
    // first, sort triangles by their group id
    qsort(&(this->triangleArray[0]), this->triangleArray.Size(), sizeof(Triangle), nMeshBuilder::TriangleGroupSorter);
}

//------------------------------------------------------------------------------
/**
    Optimize mesh for better HT&L cache locality.
*/
void
nMeshBuilder::Optimize()
{
    // TODO!
}

//------------------------------------------------------------------------------
/**
    All indices and group id's will be incremented accordingly.
*/
int
nMeshBuilder::Append(const nMeshBuilder& source)
{
    int baseVertexIndex = this->GetNumVertices();
    nArray<Group> groupMap;
    this->BuildGroupMap(groupMap);
    int baseGroupIndex = groupMap.Size();

    // add vertices
    int numVertices = source.GetNumVertices();
    int vertexIndex;
    for (vertexIndex = 0; vertexIndex < numVertices; vertexIndex++)
    {
        this->AddVertex(source.GetVertexAt(vertexIndex));
    }

    // add triangles
    Triangle triangle;
    int numTriangles = source.GetNumTriangles();
    int triangleIndex;
    for (triangleIndex = 0; triangleIndex < numTriangles; triangleIndex++)
    {
        triangle = source.GetTriangleAt(triangleIndex);

        // fix vertex indices
        triangle.vertexIndex[0] += baseVertexIndex;
        triangle.vertexIndex[1] += baseVertexIndex;
        triangle.vertexIndex[2] += baseVertexIndex;

        // fix group id
        triangle.groupId += baseGroupIndex;
        this->AddTriangle(triangle);
    }
    return baseGroupIndex;
}

//------------------------------------------------------------------------------
/**
    Replace content of this mesh with source mesh.
*/
void
nMeshBuilder::Copy(const nMeshBuilder& source)
{
    this->vertexArray   = source.vertexArray;
    this->triangleArray = source.triangleArray;
}

//------------------------------------------------------------------------------
/**
    Get the smallest vertex index referenced by the triangles in a group.

    @param  groupId             [in] group id of group to check
    @param  minVertexIndex      [out] filled with minimal vertex index
    @param  maxVertexIndex      [out] filled with maximal vertex index
*/
bool
nMeshBuilder::GetGroupVertexRange(int groupId, int& minVertexIndex, int& maxVertexIndex) const
{
    minVertexIndex = this->GetNumVertices();
    maxVertexIndex = 0;
    int numCheckedTris = 0;
    int triIndex;
    int numTriangles = this->GetNumTriangles();
    for (triIndex = 0; triIndex < numTriangles; triIndex++)
    {
        const Triangle& tri = this->GetTriangleAt(triIndex);
        if (tri.GetGroupId() == groupId)
        {
            numCheckedTris++;

            int vertexIndex[3];
            tri.GetVertexIndices(vertexIndex[0], vertexIndex[1], vertexIndex[2]);
            int i;
            for (i = 0; i < 3; i++)
            {
                if (vertexIndex[i] < minVertexIndex)  minVertexIndex = vertexIndex[i];
                if (vertexIndex[i] > maxVertexIndex)  maxVertexIndex = vertexIndex[i];
            }
        }
    }
    if (0 == numCheckedTris)
    {
        minVertexIndex = 0;
        maxVertexIndex = 0;
        return false;
    }
    return true;
}

//------------------------------------------------------------------------------
/**
    Get the smallest edge index with a groupID.

    @param  groupId           [in] group id of group to check
    @param  minEdgeIndex      [out] filled with minimal edge index
    @param  maxEdgeIndex      [out] filled with maximal edge index
*/
bool
nMeshBuilder::GetGroupEdgeRange(int groupId, int& minEdgeIndex, int& maxEdgeIndex) const
{
    minEdgeIndex = this->GetNumEdges();
    maxEdgeIndex = 0;
    bool foundGroupStart = false;
    bool foundGroupEnd = false;
    int edgeIndex;
    int numEdges = this->GetNumEdges();

    //find first edge with this groupID
    for (edgeIndex = 0; edgeIndex < numEdges; edgeIndex++)
    {
        const GroupedEdge& edge = this->GetEdgeAt(edgeIndex);
        if (edge.GroupID == groupId)
        {
            foundGroupStart = true;
            minEdgeIndex = edgeIndex;
            break;
        }
    }
    //find the begin of the next group, or the end of the edgeArray
    for (;edgeIndex < numEdges; edgeIndex++)
    {
        const GroupedEdge& edge = this->GetEdgeAt(edgeIndex);
        if (edge.GroupID != groupId)
        {
            foundGroupEnd = true;
            maxEdgeIndex = edgeIndex - 1;
            break;
        }
    }

    if (foundGroupStart)
    {
        if (!foundGroupEnd)
        {
            //is the group end the array end?
            const GroupedEdge& edge = this->GetEdgeAt(numEdges-1);
            if (edge.GroupID == groupId)
            {
                maxEdgeIndex = numEdges-1;
            }
        }
    }
    else
    {
        minEdgeIndex = 0;
        maxEdgeIndex = 0;
        return false;
    }
    return true;
}

//------------------------------------------------------------------------------
/**
    This method will erase or create empty vertex component arrays.
    New vertex component arrays will always be set to zeros.
    The method can be used to make sure that a mesh file has the same
    vertex size as expected by a vertex shader program.
*/
void
nMeshBuilder::ForceVertexComponents(int wantedMask)
{
    // for each vertex...
    int numVertices = this->GetNumVertices();
    int vertexIndex = 0;
    for (vertexIndex = 0; vertexIndex < numVertices; vertexIndex++)
    {
        Vertex& vertex = this->GetVertexAt(vertexIndex);
        int hasMask = vertex.GetComponentMask();
        if (wantedMask != hasMask)
        {
            int compIndex;
            for (compIndex = 0; compIndex < Vertex::NUM_VERTEX_COMPONENTS; compIndex++)
            {
                int curMask = 1 << compIndex;
                if ((hasMask & curMask) && !(wantedMask & curMask))
                {
                    // delete the vertex component
                    vertex.DelComponent((Vertex::Component) curMask);
                }
                else if (!(hasMask & curMask) && (wantedMask & curMask))
                {
                    // add the vertex component
                    vertex.ZeroComponent((Vertex::Component) curMask);
                }
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
    Copy one vertex component to another if source vertex component exists.
    If source vertex component does not exist, do nothing.
*/
void
nMeshBuilder::CopyVertexComponents(Vertex::Component from, Vertex::Component to)
{
    int numVertices = this->GetNumVertices();
    int vertexIndex = 0;
    for (vertexIndex = 0; vertexIndex < numVertices; vertexIndex++)
    {
        this->GetVertexAt(vertexIndex).CopyComponentFromComponent(from, to);
    }
}

//------------------------------------------------------------------------------
/**
    Does an inflated component copy from a cleaned up source mesh to
    this mesh.
*/
void
nMeshBuilder::InflateCopyComponents(const nMeshBuilder& src, const nArray< nArray<int> >& collapseMap, int compMask)
{
    int srcIndex;
    int srcNum = src.GetNumVertices();
    for (srcIndex = 0; srcIndex < srcNum; srcIndex++)
    {
        const Vertex& srcVertex = src.GetVertexAt(srcIndex);
        int dstIndex;
        int dstNum = collapseMap[srcIndex].Size();
        for (dstIndex = 0; dstIndex < dstNum; dstIndex++)
        {
            Vertex& dstVertex = this->GetVertexAt(collapseMap[srcIndex][dstIndex]);
            dstVertex.CopyComponentFromVertex(srcVertex, compMask);

        }
    }
}

//------------------------------------------------------------------------------
/**
    Compute the bounding box of the mesh, filtered by a triangle group id.
*/
bbox3
nMeshBuilder::GetGroupBBox(int groupId) const
{
    bbox3 box;
    box.begin_extend();
    int numTriangles = this->GetNumTriangles();
    int triangleIndex;
    for (triangleIndex = 0; triangleIndex < numTriangles; triangleIndex++)
    {
        const Triangle& triangle = this->GetTriangleAt(triangleIndex);
        if (triangle.GetGroupId() == groupId)
        {
            int index[3];
            triangle.GetVertexIndices(index[0], index[1], index[2]);
            int i;
            for (i = 0; i < 3; i++)
            {
                box.extend(this->GetVertexAt(index[i]).GetCoord());
            }
        }
    }
    box.end_extend();
    return box;
}

//------------------------------------------------------------------------------
/**
    Compute the bounding box of the complete mesh.
*/
bbox3
nMeshBuilder::GetBBox() const
{
    bbox3 box;
    box.begin_extend();
    int numVertices = this->GetNumVertices();
    int vertexIndex;
    for (vertexIndex = 0; vertexIndex < numVertices; vertexIndex++)
    {
        box.extend(this->GetVertexAt(vertexIndex).GetCoord());
    }
    box.end_extend();
    return box;
}

//------------------------------------------------------------------------------
/**
    Count number of vertices inside bounding box.
*/
int
nMeshBuilder::CountVerticesInBBox(const bbox3& box) const
{
    int numVertices = this->GetNumVertices();
    int vertexIndex = 0;
    int numInside = 0;
    for (vertexIndex = 0; vertexIndex < numVertices; vertexIndex++)
    {
        if (box.contains(this->GetVertexAt(vertexIndex).GetCoord()))
        {
            numInside++;
        }
    }
    return numInside;
}

//------------------------------------------------------------------------------
/**
    Clip a triangle group by a plane. All positive side triangles will
    remain in the group, a new group will be created which contains
    all negative side triangles.
    Please make sure that all triangles have a correct group index set
    before calling this method!

    NOTE: new triangles will be created at the end of the triangle array.
    Although this method does not depend on correct triangle ordering,
    other will. It is recommended to do a PackTrianglesByGroup() and
    to update all existing group triangle ranges with GetNumTrianglesInGroup()
    and GetFirstTriangleInGroup().

    Please note also that groups can become empty! At the end of the split
    you generally want to clean up the group array and remove empty groups!

    @param  clipPlane           [in] a clip plane
    @param  groupId             [in] defines triangle group to split
    @param  posGroupId          [in] group id to use for the positive group
    @param  negGroupId          [in] group id to use for the negative group
    @param  numPosTriangles     [out] resulting num of triangles in positive group
    @param  numNegTriangles     [out] resulting num of triangles in negative group
*/
void
nMeshBuilder::Split(const plane& clipPlane,
                    int groupId,
                    int posGroupId,
                    int negGroupId,
                    int& numPosTriangles,
                    int& numNegTriangles)
{
    numPosTriangles = 0;
    numNegTriangles = 0;
    int numTriangles = this->GetNumTriangles();
    int triangleIndex;
    for (triangleIndex = 0; triangleIndex < numTriangles; triangleIndex++)
    {
        Triangle& tri = this->GetTriangleAt(triangleIndex);
        if (tri.GetGroupId() != groupId)
        {
            continue;
        }

        int i0, i1, i2;
        tri.GetVertexIndices(i0, i1, i2);
        const Vertex& v0 = this->GetVertexAt(i0);
        const Vertex& v1 = this->GetVertexAt(i1);
        const Vertex& v2 = this->GetVertexAt(i2);

        // classify vertex positions
        float dist[3];
        dist[0] = clipPlane.distance(v0.GetCoord());
        dist[1] = clipPlane.distance(v1.GetCoord());
        dist[2] = clipPlane.distance(v2.GetCoord());
        const int posCode = 1;
        const int negCode = 2;
        int v0Code = (dist[0] >= 0.0f) ? posCode : negCode;
        int v1Code = (dist[1] >= 0.0f) ? posCode : negCode;
        int v2Code = (dist[2] >= 0.0f) ? posCode : negCode;
        if ((posCode == v0Code) && (posCode == v1Code) && (posCode == v2Code))
        {
            // triangle entirely on positive side of clipPlane,
            tri.SetGroupId(posGroupId);
            numPosTriangles++;
        }
        else if ((negCode == v0Code) && (negCode == v1Code) && (negCode == v2Code))
        {
            // triangle entirely on negative side of clipPlane
            tri.SetGroupId(negGroupId);
            numNegTriangles++;
        }
        else
        {
            // triangle is clipped by clipPlane, this is a bit tricky...
            // the clip operation will introduce 2 new vertices, which
            // will be appended to the end of the vertex array,
            // it will also add 2 new triangles which will be appended
            // to the end of the triangle array
            int posVertexIndices[4];
            int negVertexIndices[4];
            int numPosVertexIndices = 0;
            int numNegVertexIndices = 0;

            if (posCode == v0Code) posVertexIndices[numPosVertexIndices++] = i0;
            else                   negVertexIndices[numNegVertexIndices++] = i0;
            if ((v0Code & v1Code) == 0)
            {
                // v0 and v1 are on different sides, add a new
                // inbetween vertex to the vertex array and record
                // its index
                Vertex v01;
                float t01;
                line3 edge01(this->GetVertexAt(i0).GetCoord(), this->GetVertexAt(i1).GetCoord());
                clipPlane.intersect(edge01, t01);
                v01.Interpolate(this->GetVertexAt(i0), this->GetVertexAt(i1), t01);
                this->vertexArray.Append(v01);
                posVertexIndices[numPosVertexIndices++] = this->vertexArray.Size() - 1;
                negVertexIndices[numNegVertexIndices++] = this->vertexArray.Size() - 1;
            }

            if (posCode == v1Code) posVertexIndices[numPosVertexIndices++] = i1;
            else                   negVertexIndices[numNegVertexIndices++] = i1;
            if ((v1Code & v2Code) == 0)
            {
                // v1 and v2 are on different sides
                Vertex v12;
                float t12;
                line3 edge12(this->GetVertexAt(i1).GetCoord(), this->GetVertexAt(i2).GetCoord());
                clipPlane.intersect(edge12, t12);
                v12.Interpolate(this->GetVertexAt(i1), this->GetVertexAt(i2), t12);
                this->vertexArray.Append(v12);
                posVertexIndices[numPosVertexIndices++] = this->vertexArray.Size() - 1;
                negVertexIndices[numNegVertexIndices++] = this->vertexArray.Size() - 1;
            }
            if (posCode == v2Code) posVertexIndices[numPosVertexIndices++] = i2;
            else                   negVertexIndices[numNegVertexIndices++] = i2;
            if ((v2Code & v0Code) == 0)
            {
                // v2 and v0 are on different sides
                Vertex v20;
                float t20;
                line3 edge20(this->GetVertexAt(i2).GetCoord(), this->GetVertexAt(i0).GetCoord());
                clipPlane.intersect(edge20, t20);
                v20.Interpolate(this->GetVertexAt(i2), this->GetVertexAt(i0), t20);
                this->vertexArray.Append(v20);
                posVertexIndices[numPosVertexIndices++] = this->vertexArray.Size() - 1;
                negVertexIndices[numNegVertexIndices++] = this->vertexArray.Size() - 1;
            }

            // update the triangle array, reuse the original triangle
            int i;
            this->GetTriangleAt(triangleIndex).SetGroupId(posGroupId);
            Triangle newTri = this->GetTriangleAt(triangleIndex);
            for (i = 0; i < (numPosVertexIndices - 2); i++)
            {
                if (0 == i)
                {
                    // reuse the existing triangle (but only the
                    // first positive triangle may do this!)
                    this->GetTriangleAt(triangleIndex).SetVertexIndices(posVertexIndices[0],
                                                                        posVertexIndices[i+1],
                                                                        posVertexIndices[i+2]);
                }
                else
                {
                    newTri.SetVertexIndices(posVertexIndices[0],
                                            posVertexIndices[i+1],
                                            posVertexIndices[i+2]);
                    this->AddTriangle(newTri);
                }
                numPosTriangles++;
            }
            newTri.SetGroupId(negGroupId);
            for (i = 0; i < (numNegVertexIndices - 2); i++)
            {
                newTri.SetVertexIndices(negVertexIndices[0],
                                        negVertexIndices[i+1],
                                        negVertexIndices[i+2]);
                this->AddTriangle(newTri);
                numNegTriangles++;
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
    Build a vertex/triangle map. Lookup the map with the vertex index,
    and find an array of indices of all triangles using that vertex.
    You want to make sure to clean up the mesh before to ensure
    correct vertex sharing behavior.

    @param  vertexTriangleMap   2D-array to be filled with resulting map
*/
void
nMeshBuilder::BuildVertexTriangleMap(nArray< nArray<int> >& vertexTriangleMap) const
{
    // pre-initialize map size
    vertexTriangleMap.SetFixedSize(this->GetNumVertices());

    // iterate over triangle and record vertex/triangle mapping
    int triangleIndex;
    int numTriangles = this->GetNumTriangles();
    for (triangleIndex = 0; triangleIndex < numTriangles; triangleIndex++)
    {
        const Triangle& tri = this->GetTriangleAt(triangleIndex);
        int i[3];
        tri.GetVertexIndices(i[0], i[1], i[2]);
        vertexTriangleMap[i[0]].Append(triangleIndex);
        vertexTriangleMap[i[1]].Append(triangleIndex);
        vertexTriangleMap[i[2]].Append(triangleIndex);
    }
}

//------------------------------------------------------------------------------
/**
    Flip vertical texture coordinates.
*/
void
nMeshBuilder::FlipUvs()
{
    int numVertices = this->GetNumVertices();
    int vertexIndex;
    vector2 uv;
    for (vertexIndex = 0; vertexIndex < numVertices; vertexIndex++)
    {
        Vertex& v = this->GetVertexAt(vertexIndex);
        int layer;
        for (layer = 0; layer < 4; layer++)
        {
            if (v.HasComponent(Vertex::Component(Vertex::UV0 << layer)))
            {
                uv = v.GetUv(layer);
                uv.y = 1.0f - uv.y;
                v.SetUv(layer, uv);
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
    Or's the components of all vertices, and forces the whole
    vertex pool to that mask. This ensures that all vertices
    in the mesh builder have the same format.
*/
void
nMeshBuilder::ExtendVertexComponents()
{
    // get or'ed mask of all vertex components
    int numVertices = this->GetNumVertices();
    int mask = 0;
    int vertexIndex;
    for (vertexIndex = 0; vertexIndex < numVertices; vertexIndex++)
    {
        Vertex& v = this->GetVertexAt(vertexIndex);
        mask |= v.GetComponentMask();
    }

    // extend all vertices to the or'ed vertex component mask
    this->ForceVertexComponents(mask);
}

//------------------------------------------------------------------------------
/**
    Checks the mesh for geometry errors.

    FIXME: check for duplicate triangles!
*/
nArray<nString>
nMeshBuilder::CheckForGeometryError()
{
    nArray<nString> errors;
    char msg[1024];

    int i;
    int numTriangles = this->triangleArray.Size();
    for (i = 0; i < numTriangles; i++)
    {
        const Triangle& tri = this->triangleArray[i];

        // check triangle indices
        if ((tri.vertexIndex[0] == tri.vertexIndex[1]) ||
            (tri.vertexIndex[0] == tri.vertexIndex[2]) ||
            (tri.vertexIndex[1] == tri.vertexIndex[2]))
        {
            snprintf(msg, sizeof(msg), "Triangle with index %d has identical indices!\n", i);
            errors.Append(msg);
        }

        // check if vertices are identical
        const Vertex& v0 = this->vertexArray[tri.vertexIndex[0]];
        const Vertex& v1 = this->vertexArray[tri.vertexIndex[1]];
        const Vertex& v2 = this->vertexArray[tri.vertexIndex[2]];
        if ((v0 == v1) || (v0 == v2) || (v1 == v2))
        {
            snprintf(msg, sizeof(msg), "Triangle with index %d has identical vertices!\n", i);
            errors.Append(msg);
        }

        // check if uv coords are sane for tangent generation
        int j;
        for (j = 0; j < Vertex::MAX_TEXTURE_LAYERS; j++)
        {
            if (v0.HasComponent((Vertex::Component) (Vertex::UV0 << j)))
            {
                float dv0 = n_abs(v1.uv[j].y - v0.uv[j].y);
                float dv1 = n_abs(v2.uv[j].y - v0.uv[j].y);
                float dv2 = n_abs(v2.uv[j].y - v1.uv[j].y);
                const float thresh = 0.00001f;
                if ((dv0 < thresh) || (dv1 < thresh) || (dv2 < thresh))
                {
                    snprintf(msg, sizeof(msg), "Triangle with index %d has identical v texcoords on layer %d (may cause per-pixel lighting artifacts)!\n", i, j);
                    errors.Append(msg);
                }
            }
        }
    }

    // check edges
    int numEdges = this->edgeArray.Size();
    for (i = 0; i < numEdges; i++)
    {
        const GroupedEdge& edge = this->edgeArray[i];
        if (edge.vIndex[0] == edge.vIndex[1])
        {
            snprintf(msg, sizeof(msg), "Edge with index %d has identical vertex indices!\n", i);
            errors.Append(msg);
        }
        if (edge.fIndex[0] == edge.fIndex[1])
        {
            snprintf(msg, sizeof(msg), "Edge with index %d has identical face indices!\n", i);
            errors.Append(msg);
        }

        // check for open edges
        if ((nMesh2::InvalidIndex == edge.fIndex[0]) || (nMesh2::InvalidIndex == edge.fIndex[1]))
        {
            snprintf(msg, sizeof(msg), "Edge %d is open (holes in model?)!\n", i);
            errors.Append(msg);
        }
        if ((nMesh2::InvalidIndex == edge.vIndex[0]) || (nMesh2::InvalidIndex == edge.vIndex[1]))
        {
            snprintf(msg, sizeof(msg), "Edge %d has invalid vertex index!\n", i);
            errors.Append(msg);
        }
    }
    return errors;
}

//------------------------------------------------------------------------------
/**
    checks the mesh for duplicated faces
*/
nArray<nString>
nMeshBuilder::CheckForDuplicatedFaces()
{
    nArray<nString> emptyArray;
    return emptyArray;

// FIXME: temporarily commented out because of error with quad geometry
/*
    // create temporary meshbuilder and erase any vertex component but the COORD, then search for duplicated faces
    nMeshBuilder tempBuilder = *this;
    tempBuilder.ForceVertexComponents(Vertex::COORD);
    tempBuilder.Cleanup(0);
    nArray<int> dupFaces = tempBuilder.SearchDuplicatedFaces();

    nArray<nString> errors;
    char msg[1024];
    int i;
    int offset = 0;

    // erase duplicated
    for (i = 0; i < dupFaces.Size(); i++)
    {
        this->triangleArray.Erase(dupFaces[i]-offset);
        offset++;

        sprintf(msg,"Erasing triangle %i",dupFaces[i]);
        errors.Append(msg);
    };

    return errors;
*/
}

//------------------------------------------------------------------------------
/**
    searches the mesh for duplicated faces
*/
nArray<int>
nMeshBuilder::SearchDuplicatedFaces()
{
    nArray<int> result;
    int i;

    // an array which holds 1 list for each vertex. 1 lists contains references to all faces that the vertice
    // belongs to
    nArray<nArray<Triangle*> > triangleReferences;

    // initialize lists
    nArray<Triangle*> emptyDummy;
    triangleReferences.SetFixedSize(this->vertexArray.Size());

    // now insert face references
    for (i = 0; i < this->triangleArray.Size(); i++)
    {
        int index0 = this->triangleArray[i].vertexIndex[0];
        int index1 = this->triangleArray[i].vertexIndex[1];
        int index2 = this->triangleArray[i].vertexIndex[2];

        // now we check if there is a triangle in the list at index0 that equals the one we are currently working on
        // the size of the list at index0 should be low for the average case
        int k;
        bool found = false;
        for (k = 0; k < triangleReferences[index0].Size(); k++)
        {
            if (triangleReferences[index0].At(k)->Equals(this->triangleArray[i]))
            {
                found = true;
            }
        }

        if (found)
        {
            // ok, we found a triangle that is equal to the current
            result.Append(i);
        }
        else
        {
            // otherwise we store a reference to this triangle in the lists at the indices
            triangleReferences[index0].PushBack(&this->triangleArray[i]);
            triangleReferences[index1].PushBack(&this->triangleArray[i]);
            triangleReferences[index2].PushBack(&this->triangleArray[i]);
        }
    }

    return result;
}

//------------------------------------------------------------------------------
/**
    This creates 3 unique vertices for each triangle in the mesh, generating
    redundant vertices. This is the opposite operation to Cleanup(). This will
    throw away any generated edges!
*/
void
nMeshBuilder::Inflate()
{
    // for each triangle...
    nArray<Vertex> newVertexArray(this->GetNumTriangles() * 3, 0);
    int numTriangles = this->GetNumTriangles();
    int triangleIndex;
    for (triangleIndex = 0; triangleIndex < numTriangles; triangleIndex++)
    {
        // build new vertex array and fix triangle vertex indices
        Triangle& tri = this->GetTriangleAt(triangleIndex);
        int i;
        for (i = 0; i < 3; i++)
        {
            newVertexArray.Append(this->GetVertexAt(tri.vertexIndex[i]));
            tri.vertexIndex[i] = triangleIndex * 3 + i;
        }
    }

    // replace vertex array
    this->vertexArray = newVertexArray;

    // dump edge array
    this->edgeArray.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void nMeshBuilder::CopyGroup(nMeshBuilder& srcMesh, int groupId) {
    nArray<int> indexMap(0, 0);
    indexMap.SetFixedSize(srcMesh.GetNumVertices());
    indexMap.Fill(0, indexMap.Size(), -1);
    for (int i = 0; i < srcMesh.triangleArray.Size(); i++) {
        if (srcMesh.triangleArray[i].GetGroupId() == groupId) {
            this->CopyTriangle(srcMesh, i, indexMap);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void nMeshBuilder::ChangeGroupId(int srcGroupId, int dstGroupId) {
    for (int i = 0; i < this->triangleArray.Size(); i++) {
        if (this->triangleArray[i].GetGroupId() == srcGroupId) {
            this->triangleArray[i].SetGroupId(dstGroupId);
        }
    }
}
