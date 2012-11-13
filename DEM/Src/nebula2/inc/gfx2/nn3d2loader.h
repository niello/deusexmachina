#ifndef N_N3D2LOADER_H
#define N_N3D2LOADER_H
//------------------------------------------------------------------------------
/**
    @class nN3d2Loader
    @ingroup Gfx2

    Load a N3D2 mesh file into user provided vertex and index buffers.

    (C) 2003 RadonLabs GmbH
*/
#include "gfx2/nmeshloader.h"

//------------------------------------------------------------------------------
class nN3d2Loader : public nMeshLoader
{
protected:
	bool GetS(char* buf, int bufSize);
public:

	virtual ~nN3d2Loader() {}
	/// open file and read header data
    virtual bool Open();
    /// close the file
    virtual void Close();
    /// read vertex data
    virtual bool ReadVertices(void* buffer, int bufferSize);
    /// read index data
    virtual bool ReadIndices(void* buffer, int bufferSize);
    /// read edge data
    virtual bool ReadEdges(void* buffer, int bufferSize);
};

//------------------------------------------------------------------------------
/**
*/
inline bool nN3d2Loader::GetS(char* buf, int bufSize)
{
    n_assert(buf);
    n_assert(bufSize > 1);

    if (file->IsEOF()) return false;

    bufSize--; // make room for final terminating 0    

	int startPos = file->GetPosition();

    // read file contents in chunks of 64 bytes, not char by char
    int chunkSize = n_min(bufSize, 256);
    char* readPos = buf;
    int curIndex;
    for (curIndex = 0; curIndex < bufSize; curIndex++)
    {
        // read next chunk of data?
        if (0 == (curIndex % chunkSize))
        {
            // if we reached end-of-file before, break out
            if (file->IsEOF()) break;

            // now, read the next chunk of data
            int readSize = chunkSize;
            if ((curIndex + readSize) >= bufSize)
				readSize = bufSize - curIndex;
            int bytesRead = file->Read(readPos, readSize);
            if (bytesRead != readSize) readPos[bytesRead] = 0;
            readPos += bytesRead;
        }

        // check for newline
        if (buf[curIndex] == '\n')
        {
            // reset file pointer to position after new-line
			file->Seek(startPos + curIndex + 1, Data::SSO_BEGIN);
            break;
        }
    }
    buf[curIndex] = 0;
    return true;
}


//------------------------------------------------------------------------------
/**
    - 28-Sep-04     floh    bugfix: vertexComponents was overwritten with 0,
                            instead of fileVertexComponents
*/
inline
bool
nN3d2Loader::Open()
{
    n_assert(!this->file);

	file = n_new(Data::CFileStream);
    n_assert(this->file);

    // open the file
	if (!this->file->Open(this->filename.Get(), Data::SAM_READ))
    {
        n_printf("nN3d2Loader: could not open file '%s'!\n", this->filename.Get());
        this->Close();
        return false;
    }

    // read file header
    bool headerDone = false;
    char line[1024];
    while ((!headerDone) && (this->GetS(line, sizeof(line))))
    {
        // get keyword
        char* keyWord = strtok(line, N_WHITESPACE);
        if (0 == keyWord)
        {
            continue;
        }
        if (0 == strcmp(keyWord, "type"))
        {
            // type must be 'n3d2'
            const char* typeString = strtok(0, N_WHITESPACE);
            n_assert(typeString);
            if (0 != strcmp(typeString, "n3d2"))
            {
                n_printf("nN3d2Loader: In file '%s' invalid type '%s', must be 'n3d2'\n", filename.Get(), typeString);
                this->Close();
                return false;
            }
        }
        else if (0 == strcmp(keyWord, "numgroups"))
        {
            // number of groups
            const char* numGroupsString = strtok(0, N_WHITESPACE);
            n_assert(numGroupsString);
            this->numGroups = atoi(numGroupsString);
        }
        else if (0 == strcmp(keyWord, "numvertices"))
        {
            // number of vertices
            const char* numVerticesString = strtok(0, N_WHITESPACE);
            n_assert(numVerticesString);
            this->numVertices = atoi(numVerticesString);
        }
        else if (0 == strcmp(keyWord, "numedges"))
        {
            // number of edges
            const char* numEdgesString = strtok(0, N_WHITESPACE);
            n_assert(numEdgesString);
            this->numEdges = atoi(numEdgesString);
        }
        else if (0 == strcmp(keyWord, "vertexcomps"))
        {
            // vertex components
            char* str;
            this->fileVertexComponents = 0;
            while (str = strtok(0, N_WHITESPACE))
            {
                if (0 == strcmp(str, "coord"))          this->fileVertexComponents |= nMesh2::Coord;
                else if (0 == strcmp(str, "normal"))    this->fileVertexComponents |= nMesh2::Normal;
                else if (0 == strcmp(str, "uv0"))       this->fileVertexComponents |= nMesh2::Uv0;
                else if (0 == strcmp(str, "uv1"))       this->fileVertexComponents |= nMesh2::Uv1;
                else if (0 == strcmp(str, "uv2"))       this->fileVertexComponents |= nMesh2::Uv2;
                else if (0 == strcmp(str, "uv3"))       this->fileVertexComponents |= nMesh2::Uv3;
                else if (0 == strcmp(str, "color"))     this->fileVertexComponents |= nMesh2::Color;
                else if (0 == strcmp(str, "tangent"))   this->fileVertexComponents |= nMesh2::Tangent;
                else if (0 == strcmp(str, "binormal"))  this->fileVertexComponents |= nMesh2::Binormal;
                else if (0 == strcmp(str, "weights"))   this->fileVertexComponents |= nMesh2::Weights;
                else if (0 == strcmp(str, "jindices"))  this->fileVertexComponents |= nMesh2::JIndices;
            }
        }
        else if (0 == strcmp(keyWord, "numtris"))
        {
            // number of triangles
            const char* numTrisString = strtok(0, N_WHITESPACE);
            n_assert(numTrisString);
            this->numTriangles = atoi(numTrisString);
            this->numIndices = this->numTriangles * 3;
        }
        else if (0 == strcmp(keyWord, "vertexwidth"))
        {
            // vertex width
            const char* vertexWidthString = strtok(0, N_WHITESPACE);
            n_assert(vertexWidthString);
            this->fileVertexWidth = atoi(vertexWidthString);
        }
        else if (0 == strcmp(keyWord, "g"))
        {
            // a group definition
            const char* firstVertString = strtok(0, N_WHITESPACE);
            const char* numVertsString  = strtok(0, N_WHITESPACE);
            const char* firstTriString  = strtok(0, N_WHITESPACE);
            const char* numTrisString   = strtok(0, N_WHITESPACE);
            const char* firstEdgeString = strtok(0, N_WHITESPACE);
            const char* numEdgeString   = strtok(0, N_WHITESPACE);

            n_assert(firstVertString && numVertsString);
            n_assert(firstTriString && numTrisString);
            n_assert(firstEdgeString && numEdgeString);

            nMeshGroup meshGroup;
            meshGroup.FirstVertex = atoi(firstVertString);
            meshGroup.NumVertices = atoi(numVertsString);
            meshGroup.FirstIndex = atoi(firstTriString) * 3;
            meshGroup.NumIndices = atoi(numTrisString) * 3;
            this->groupArray.Append(meshGroup);

            // if all groups read, set the headerDone flag to true
            if (this->groupArray.Size() == this->numGroups)
            {
                headerDone = true;
            }
        }
    }

    return nMeshLoader::Open();
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nN3d2Loader::Close()
{
    if (this->file)
    {
        if (this->file->IsOpen())
        {
            this->file->Close();
        }
		n_delete(file);
        this->file = 0;
    }
}

//------------------------------------------------------------------------------
/**
    - 28-Sep-04     floh    fixed bug in sparse vertex reading
*/
inline
bool
nN3d2Loader::ReadVertices(void* buffer, int bufferSize)
{
    n_assert(buffer);
    n_assert(this->file);

    // check required buffer size
    n_assert((this->numVertices * this->vertexWidth * sizeof(float)) == bufferSize);
    float* endOfBuffer = ((float*)buffer) + (this->numVertices * this->vertexWidth);

    // load vertices
    if (this->vertexComponents == this->fileVertexComponents)
    {
        n_assert(this->vertexWidth == this->fileVertexWidth);
        int vertexIndex;
        for (vertexIndex = 0; vertexIndex < this->numVertices; vertexIndex++)
        {
            char line[1024];
            bool res = this->GetS(line, sizeof(line));
            n_assert(res);

            char* keyWord = strtok(line, N_WHITESPACE);
            n_assert(0 == strcmp(keyWord, "v"));

            float* vPtr = ((float*)buffer) + (vertexIndex * this->vertexWidth);
            int i;
            for (i = 0; i < this->vertexWidth; i++)
            {
                const char* curFloatStr = strtok(0, N_WHITESPACE);
                n_assert(curFloatStr);
                float curFloat = (float)atof(curFloatStr);
                vPtr[i] = curFloat;
                n_assert(&(vPtr[i]) < endOfBuffer);
            }
        }
    }
    else
    {
        int vertexIndex;
        for (vertexIndex = 0; vertexIndex < this->numVertices; vertexIndex++)
        {
            char line[1024];
            bool res = this->GetS(line, sizeof(line));
            n_assert(res);

            char* keyWord = strtok(line, N_WHITESPACE);
            n_assert(0 == strcmp(keyWord, "v"));

            float* vPtr = ((float*)buffer) + (vertexIndex * this->vertexWidth);

            int bitIndex;
            for (bitIndex = 0; bitIndex < nMesh2::NumVertexComponents; bitIndex++)
            {
                int mask = (1<<bitIndex);

                // skip completely if current vertex component is not in file
                if (0 == (this->fileVertexComponents & mask))
                {
                    continue;
                }

                // get width of current vertex component
                int width = nMesh2::GetVertexWidthFromMask(mask);
                n_assert(width > 0);
                if (this->vertexComponents & mask)
                {
                    // read the vertex component
                    for (int f = 0; f < width; f++)
                    {
                        const char* curFloatStr = strtok(0, N_WHITESPACE);
                        n_assert(curFloatStr);
                        float curFloat = (float)atof(curFloatStr);
                        *(vPtr++) = curFloat;
                        n_assert(vPtr <= endOfBuffer)
                    }
                }
                else
                {
                    // skip the vertex component
                    for (int f = 0; f < width; f++)
                    {
                        const char* curFloatStr = strtok(0, N_WHITESPACE);
                        n_assert(curFloatStr);
                    }
                }
            }
        }
    }

    return true;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nN3d2Loader::ReadIndices(void* buffer, int bufferSize)
{
    n_assert(buffer);
    n_assert(this->file);

    void* endOfBuffer = 0;
    if (Index16 == this->indexType)
    {
        n_assert(bufferSize == (this->numIndices * int(sizeof(ushort))));
        endOfBuffer = ((ushort*)buffer) + this->numIndices;
    }
    else
    {
        n_assert(bufferSize == (this->numIndices * int(sizeof(uint))));
        endOfBuffer = ((uint*)buffer) + this->numIndices;
    }

    ushort* indexBuffer16 = (ushort*)buffer;
    uint* indexBuffer32 = (uint*)buffer;

    int triangleIndex;
    int index = 0;
    for (triangleIndex = 0; triangleIndex < this->numTriangles; triangleIndex++)
    {
        char line[1024];
        bool res = this->GetS(line, sizeof(line));
        n_assert(res);

        char* keyWord = strtok(line, N_WHITESPACE);
        n_assert(0 == strcmp(keyWord, "t"));

        const char* i0String = strtok(0, N_WHITESPACE);
        const char* i1String = strtok(0, N_WHITESPACE);
        const char* i2String = strtok(0, N_WHITESPACE);
        n_assert(i0String && i1String && i2String);
        uint i0 = atoi(i0String);
        uint i1 = atoi(i1String);
        uint i2 = atoi(i2String);

        if (Index16 == this->indexType)
        {
            indexBuffer16[index++] = (ushort)i0;
            indexBuffer16[index++] = (ushort)i1;
            indexBuffer16[index++] = (ushort)i2;
            n_assert(&(indexBuffer16[index]) <= endOfBuffer);
        }
        else
        {
            indexBuffer32[index++] = i0;
            indexBuffer32[index++] = i1;
            indexBuffer32[index++] = i2;
            n_assert(&(indexBuffer32[index]) <= endOfBuffer);
        }
    }
    return true;
}

//------------------------------------------------------------------------------
/**
    A edge has the size of 4 * ushort, so you have to provide a buffer with the
    size numEdges * 4 * sizeof(ushort).
    The edge data is: ushort vertexIndex1, vertexIndex2, faceIndex1, faceIndex2;
    If a face Indicie is invalid (a border edge with only on face connected)
    the value is nMeshBuilder::InvalidIndex (== -1).
*/
inline
bool
nN3d2Loader::ReadEdges(void* buffer, int bufferSize)
{
    n_assert(buffer);
    n_assert(this->file);

    void* endOfBuffer = 0;
    n_assert(bufferSize == (this->numEdges * 4 * int(sizeof(ushort))));
    endOfBuffer = ((ushort*)buffer) + this->numEdges * 4;

    //nMesh2::Edge* edgeBuffer = (nMesh2::Edge*)buffer;

    int edgeIndex;
    int index = 0;
    for (edgeIndex = 0; edgeIndex < this->numEdges; edgeIndex++)
    {
        char line[1024];
        bool res = this->GetS(line, sizeof(line));
        n_assert(res);

        char* keyWord = strtok(line, N_WHITESPACE);
        n_assert(0 == strcmp(keyWord, "e"));

        const char* f0String = strtok(0, N_WHITESPACE);
        const char* f1String = strtok(0, N_WHITESPACE);
        const char* v0String = strtok(0, N_WHITESPACE);
        const char* v1String = strtok(0, N_WHITESPACE);
        n_assert(v0String && v1String && f0String && f1String);
        ushort f0 = atoi(f0String);
        ushort f1 = atoi(f1String);
        ushort v0 = atoi(v0String);
        ushort v1 = atoi(v1String);

        //edgeBuffer[index].fIndex[0] = f0;
        //edgeBuffer[index].fIndex[1] = f1;
        //edgeBuffer[index].vIndex[0] = v0;
        //edgeBuffer[index].vIndex[1] = v1;
        index++;

        //n_assert(&(edgeBuffer[index]) <= endOfBuffer);
    }
    return true;
}

//------------------------------------------------------------------------------
#endif
