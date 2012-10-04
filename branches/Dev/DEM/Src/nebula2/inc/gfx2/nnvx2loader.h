#ifndef N_NVX2LOADER_H
#define N_NVX2LOADER_H
//------------------------------------------------------------------------------
/**
    @class nNvx2Loader
    @ingroup Gfx2

    Load a NVX2 mesh file into user provided vertex and index buffers.

    (C) 2003 RadonLabs GmbH
*/
#include "gfx2/nmeshloader.h"

//------------------------------------------------------------------------------
class nNvx2Loader : public nMeshLoader
{
public:

	virtual ~nNvx2Loader() {}

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
inline
bool
nNvx2Loader::Open()
{
    n_assert(!this->file);

	file = n_new(Data::CFileStream);
    n_assert(this->file);

    // open the file
	if (!this->file->Open(this->filename, Data::SAM_READ))
    {
        n_printf("nNvx2Loader: could not open file '%s'!\n", this->filename.Get());
        this->Close();
        return false;
    }

    // read file header, including groups
    int magic;
	file->Read(&magic, sizeof(int));
    if (magic != 'NVX2')
    {
        n_printf("nNvx2Loader: '%s' is not a NVX2 file!\n", this->filename.Get());
        this->Close();
        return false;
    }
	file->Read(&numGroups, sizeof(int));
	file->Read(&numVertices, sizeof(int));
	file->Read(&fileVertexWidth, sizeof(int));
	file->Read(&numTriangles, sizeof(int));
	file->Read(&numEdges, sizeof(int));
	file->Read(&fileVertexComponents, sizeof(int));
    this->numIndices       = this->numTriangles * 3;

    int groupIndex;
    for (groupIndex = 0; groupIndex < this->numGroups; groupIndex++)
    {
        int firstVertex;
        int numVertices;
        int firstTriangle;
        int numTriangles;
        int firstEdge;
        int numEdges;
		file->Read(&firstVertex, sizeof(int));
		file->Read(&numVertices, sizeof(int));
		file->Read(&firstTriangle, sizeof(int));
		file->Read(&numTriangles, sizeof(int));
		file->Read(&firstEdge, sizeof(int));
		file->Read(&numEdges, sizeof(int));

        nMeshGroup group;
        group.FirstVertex = firstVertex;
        group.NumVertices = numVertices;
        group.FirstIndex = firstTriangle * 3;
        group.NumIndices = numTriangles * 3;
        group.FirstEdge = firstEdge;
        group.NumEdges = numEdges;
        this->groupArray.Append(group);
    }

    return nMeshLoader::Open();
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nNvx2Loader::Close()
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
*/
inline
bool
nNvx2Loader::ReadVertices(void* buffer, int bufferSize)
{
    n_assert(buffer);
    n_assert(this->file);
    n_assert((this->numVertices * this->vertexWidth * int(sizeof(float))) == bufferSize);
    if (this->vertexComponents == this->fileVertexComponents)
    {
        n_assert(this->vertexWidth == this->fileVertexWidth);
        file->Read(buffer, bufferSize);
    }
    else
    {
        float* destBuf = (float*)buffer;
        float* readBuffer = n_new_array(float, this->fileVertexWidth);
        const int readSize = int(sizeof(float)) * this->fileVertexWidth;
        int v = 0;
        for (v = 0; v < this->numVertices; v++)
        {
            float* vBuf = readBuffer;
            int numRead = file->Read(vBuf, readSize);
            n_assert(numRead == readSize);

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
                    int f;
                    for (f = 0; f < width; f++)
                    {
                        *destBuf++ = *vBuf++;
                    }
                }
                else
                {
                    // skip the vertex component
                    vBuf += width;
                }
            }
        }
        n_delete_array(readBuffer);
    }
    return true;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nNvx2Loader::ReadIndices(void* buffer, int bufferSize)
{
    n_assert(buffer);
    n_assert(this->file);
    if (Index16 == this->indexType)
    {
        // 16 bit indices: read index array directly
        n_assert((this->numIndices * int(sizeof(ushort))) == bufferSize);
        file->Read(buffer, bufferSize);
    }
    else
    {
        // 32 bit indices, read into 16 bit buffer, and expand
        n_assert((this->numIndices * int(sizeof(uint))) == bufferSize);

        // read 16 bit indices into tmp buffer
        int size16 = this->numIndices * sizeof(ushort);
        ushort* ptr16 = (ushort*)n_malloc(size16);
        n_assert(ptr16);
        file->Read(ptr16, size16);

        // expand to 32 bit indices
        uint* ptr32 = (uint*)buffer;
        int i;
        for (i = 0; i < this->numIndices; i++)
        {
            ptr32[i] = (uint)ptr16[i];
        }

        // release tmp buffer
        n_free(ptr16);
    }
    return true;
}

//------------------------------------------------------------------------------
/**
    A edge has the size of 4 * ushort, so you have to provide a buffer with the
    size numEdges * 4 * sizeof(ushort).
    The edge data is: ushort faceIndex1, faceIndex2, vertexIndex1, vertexIndex2;
    If a face Indicie is invalid (a border edge with only on face connected)
    the value is (ushort)nMeshBuilder::InvalidIndex (== -1).
*/
inline
bool
nNvx2Loader::ReadEdges(void* buffer, int bufferSize)
{
    n_assert(buffer);
    n_assert(this->file);
    if (this->numEdges > 0)
    {
        n_assert((this->numEdges * 4 * int(sizeof(ushort))) == bufferSize);
        file->Read(buffer, bufferSize);
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
#endif
