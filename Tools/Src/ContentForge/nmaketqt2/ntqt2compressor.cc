//------------------------------------------------------------------------------
//  ntqt2compressor.cc
//  (C) 2003 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "ntqt2compressor.h"

//------------------------------------------------------------------------------
/**
*/
nTqt2Compressor::nTqt2Compressor(nKernelServer* ks) :
    kernelServer(ks),
    sourceFile(0),
    targetFile(0),
    treeDepth(0),
    tileSize(0),
    mode(DXT5)
{
    // initialize Direct3D and reference device
    this->d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    n_assert(this->d3d9);

    // create a reference device
    D3DDISPLAYMODE mode;
    this->d3d9->GetAdapterDisplayMode(0, &mode);

    D3DPRESENT_PARAMETERS pp = { 0 };
    pp.BackBufferWidth = 1;
    pp.BackBufferHeight = 1;
    pp.BackBufferFormat = mode.Format;
    pp.BackBufferCount = 1;
    pp.SwapEffect = D3DSWAPEFFECT_COPY;
    pp.Windowed = TRUE;
    HRESULT hr = this->d3d9->CreateDevice(0, 
                                          D3DDEVTYPE_REF, 
                                          GetDesktopWindow(), 
                                          D3DCREATE_HARDWARE_VERTEXPROCESSING,
                                          &pp, &(this->d3d9Dev));
    n_assert(SUCCEEDED(hr));
}

//------------------------------------------------------------------------------
/**
*/
nTqt2Compressor::~nTqt2Compressor()
{
    if (this->sourceFile)
    {
        n_delete(sourceFile);
        this->sourceFile = 0;
    }
    if (this->targetFile)
    {
        n_delete(targetFile);
        this->targetFile = 0;
    }

    this->d3d9Dev->Release();
    this->d3d9Dev = 0;

    this->d3d9->Release();
    this->d3d9 = 0;
}

//------------------------------------------------------------------------------
/**
    Compute number of nodes from tree depth.
*/
int
nTqt2Compressor::GetNumNodes(int level) const
{
    return 0x55555555 & ((1 << level * 2) - 1);
}

//------------------------------------------------------------------------------
/**
    Open the source file and read header.
*/
bool
nTqt2Compressor::OpenSourceFile()
{
    n_assert(0 == this->sourceFile);
    
	this->sourceFile = n_new(Data::CFileStream);
	if (!this->sourceFile->Open(this->GetSourceFile(), Data::SAM_READ))
    {
        this->SetError("Could not open source file '%s'", this->GetSourceFile());
        this->CloseSourceFile();
        return false;
    }
    
    // read tqt header, make sure it's in RAW format
    int magic = this->sourceFile->Get<int>();
    if (magic != 'TQT2')
    {
        this->SetError("Source file '%s' not a TQT2 file!", this->GetSourceFile());
        this->CloseSourceFile();
        return false;
    }

    this->treeDepth = this->sourceFile->Get<int>();
    this->tileSize  = this->sourceFile->Get<int>();
    int format = this->sourceFile->Get<int>();
    if (format != 0)
    {
        this->SetError("Source file '%s' not in RAW format!", this->GetSourceFile());
        this->CloseSourceFile();
        return false;
    }

    // read table of contents
    int numNodes = this->GetNumNodes(this->treeDepth);
    this->sourceToc.SetFixedSize(numNodes);
    int i;
    for (i = 0; i < numNodes; i++)
    {
        this->sourceToc[i].pos = this->sourceFile->Get<int>();
        this->sourceToc[i].size = this->sourceFile->Get<int>();
    }
    return true;
}

//------------------------------------------------------------------------------
/**
    Open the target file and write header, depends on data initialized
    by OpenSourceFile().
*/
bool
nTqt2Compressor::OpenTargetFile()
{
    n_assert(0 == this->targetFile);
    n_assert(0 != this->sourceFile);

    this->targetFile = n_new(Data::CFileStream);
	if (!this->targetFile->Open(this->GetTargetFile(), Data::SAM_WRITE))
    {
        this->SetError("Could not open target file '%s'", this->GetTargetFile());
        this->CloseTargetFile();
        return false;
    }

    // write header
    this->targetFile->Put<int>('TQT2');
    this->targetFile->Put<int>(this->treeDepth);
    this->targetFile->Put<int>(this->tileSize);
    this->targetFile->Put<int>(1); // DDS format
    
    // write empty toc
    int numNodes = this->GetNumNodes(this->treeDepth);
    this->targetToc.SetFixedSize(numNodes);
    struct TocEntry emptyTocEntry = { 0, 0 };
    this->targetToc.Fill(0, numNodes, emptyTocEntry);
    int i;
    for (i = 0; i < numNodes; i++)
    {
        this->targetFile->Put<int>(this->targetToc[i].pos);
        this->targetFile->Put<int>(this->targetToc[i].size);
    }
    return true;
}

//------------------------------------------------------------------------------
/**
    Close the source file.
*/
void
nTqt2Compressor::CloseSourceFile()
{
    if (this->sourceFile)
    {
        n_delete(sourceFile);
        this->sourceFile = 0;
    }
}

//------------------------------------------------------------------------------
/**
    Close the target file after writing TOC.
*/
void
nTqt2Compressor::CloseTargetFile()
{
    if (this->targetFile)
    {
        if (this->targetFile->IsOpen())
        {
            // write toc
            this->targetFile->Seek(4 * sizeof(int), Data::SSO_BEGIN);
            int numNodes = this->GetNumNodes(this->treeDepth);
            int i;
            for (i = 0; i < numNodes; i++)
            {
                this->targetFile->Put<int>(this->targetToc[i].pos);
                this->targetFile->Put<int>(this->targetToc[i].size);
            }
            this->targetFile->Close();
        }
        n_delete(targetFile);
        this->targetFile = 0;
    }
}

//------------------------------------------------------------------------------
/**
    Compress tile with given index and write to target file, updating
    the target TOC.
*/
bool
nTqt2Compressor::CompressTile(int index)
{
    n_assert(this->d3d9Dev);
    n_assert(this->sourceFile);
    n_assert(this->targetFile);

    n_printf("-> converting texture %d of %d\n", index + 1, this->sourceToc.Size());
    fflush(stdout);

    // read texture data into a temporary buffer
    int srcDataSize = this->sourceToc[index].size;
    void* srcData = n_malloc(srcDataSize);
    n_assert(srcData);
    this->sourceFile->Seek(this->sourceToc[index].pos, Data::SSO_BEGIN);
    int bytesRead = this->sourceFile->Read(srcData, srcDataSize);
    n_assert(bytesRead == srcDataSize);

    // create a d3d texture in compressed format
    D3DFORMAT pixelFormat;
    if (this->mode == DXT1)
    {
        pixelFormat = D3DFMT_DXT1;
    }
    else
    {
        pixelFormat = D3DFMT_DXT5;
    }
    IDirect3DTexture9* d3dTexture = 0;
    HRESULT hr = D3DXCreateTexture(this->d3d9Dev,       // pDevice
                                   this->tileSize,      // Width
                                   this->tileSize,      // Height
                                   D3DX_DEFAULT,        // MipLevels
                                   0,                   // Usage
                                   pixelFormat,         // Format
                                   D3DPOOL_SCRATCH,     // Pool
                                   &d3dTexture);        // ppTexture
    n_assert(SUCCEEDED(hr));
    n_assert(d3dTexture);

    // get toplevel surface
    IDirect3DSurface9* surf = 0;
    hr = d3dTexture->GetSurfaceLevel(0, &surf);
    n_assert(SUCCEEDED(hr));
    n_assert(surf);

    // load src image data into surface (DXT compression happens here)
    RECT srcRect = { 0, 0, this->tileSize, this->tileSize };
    hr = D3DXLoadSurfaceFromMemory(surf,                // pDestSurface
                                   NULL,                // pDestPalette
                                   NULL,                // pDestRect (entire surface)
                                   srcData,             // pSrcMemory
                                   D3DFMT_A8R8G8B8,     // srcFormat
                                   this->tileSize * 4,  // srcPitch
                                   NULL,                // pSrcPalette
                                   &srcRect,            // pSrcRect
                                   D3DX_FILTER_NONE,    // Filter
                                   0);                  // ColorKey (disabled)
    n_assert(SUCCEEDED(hr));

    // we no longer need to access the toplevel surface
    int ref = surf->Release();
    surf = 0;

    // free source data
    n_free(srcData);
    srcData = 0;

    // generate mipmaps
    hr = D3DXFilterTexture(d3dTexture, 0, D3DX_DEFAULT, D3DX_DEFAULT);
    n_assert(SUCCEEDED(hr));

    // save texture to a memory buffer
    ID3DXBuffer* d3dBuffer = 0;
    hr = D3DXSaveTextureToFileInMemory(&d3dBuffer, D3DXIFF_DDS, d3dTexture, 0);
    n_assert(SUCCEEDED(hr));
    n_assert(d3dBuffer);
    void* bufferPtr = d3dBuffer->GetBufferPointer();
    int bufferSize  = d3dBuffer->GetBufferSize();

    // write buffer to target file, record file position
    this->targetToc[index].pos = this->targetFile->GetPosition();
    this->targetToc[index].size = bufferSize;
    int bytesWritten = this->targetFile->Write(bufferPtr, bufferSize);
    n_assert(bytesWritten == bufferSize);

    // release stuff
    ref = d3dBuffer->Release();
    d3dBuffer = 0;
    ref = d3dTexture->Release();
    d3dTexture = 0;

    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
nTqt2Compressor::Run()
{
    this->SetError("NoError");

    // check parameters
    if (this->sourceFileName.IsEmpty())
    {
        this->SetError("No source filename given!");
        return false;
    }
    if (this->targetFileName.IsEmpty())
    {
        this->SetError("No target filename given!");
        return false;
    }

    // open file
    if (!this->OpenSourceFile())
    {
        return false;
    }
    if (!this->OpenTargetFile())
    {
        this->SetError("Could not open target file '%s'", this->GetTargetFile());
        this->CloseSourceFile();
        return false;
    }

    // for each tile...
    int numTiles = this->GetNumNodes(this->treeDepth);
    int i;
    for (i = 0; i < numTiles; i++)
    {
        this->CompressTile(i);
    }

    // close files
    this->CloseTargetFile();
    this->CloseSourceFile();

    return true;
}
