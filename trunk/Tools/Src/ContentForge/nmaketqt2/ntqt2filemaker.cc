//------------------------------------------------------------------------------
//  ntqt2filemaker.cc
//  (C) 2003 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "ntqt2filemaker.h"
#include <Data/DataServer.h>

//------------------------------------------------------------------------------
/**
*/
nTqt2FileMaker::nTqt2FileMaker(nKernelServer* ks) :
    kernelServer(ks),
    tileSize(0),
    treeDepth(0),
    tarFile(0)
{
    // initialize IL and ILU
    ilInit();
    iluInit();

    // set ILU's scale filter
    //iluImageParameter(ILU_FILTER, ILU_BILINEAR);
}

//------------------------------------------------------------------------------
/**
*/
nTqt2FileMaker::~nTqt2FileMaker()
{
    CloseFiles();
}

//------------------------------------------------------------------------------
/**
    Close all files.
*/
void
nTqt2FileMaker::CloseFiles()
{
    if (tarFile)
    {
        n_delete(tarFile);
        tarFile = 0;
    }
}

//------------------------------------------------------------------------------
/**
    Open all files.
*/
bool
nTqt2FileMaker::OpenFiles()
{
    n_assert(kernelServer);
    n_assert(!srcFileName.IsEmpty());
    n_assert(!tarFileName.IsEmpty());
    n_assert(0 == tarFile);

	tarFile = n_new(Data::CFileStream);
	if (!tarFile->Open(tarFileName.Get(), Data::SAM_READWRITE))
    {
        SetError("Could not open target file '%s'\n", tarFileName.Get());
        CloseFiles();
        return false;
    }
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool nTqt2FileMaker::Run()
{
	SetError("NoError");

	// check parameters
	if ((treeDepth < 1) || (treeDepth > 12))
	{
		SetError("Invalid tree depth (must be between 1 and 12)\n");
		return false;
	}

	// check that filesize is power of 2
	if (tileSize < 0)
	{
		SetError("Tile size must be 2^n or 0 to use no scaling (is: %d)", tileSize);
		return false;
	}
	if (tileSize > 0)
	{
		int loggedTileSize = 1 << n_frnd(n_log2(float(tileSize)));
		if ((tileSize != loggedTileSize))
		{
			SetError("Tile size must be 2^n or 0 to use no scaling (is: %d)", tileSize);
			return false;
		}
	}

	// open the files...
	if (!OpenFiles()) return false;

	// write tqt2 header
	tarFile->Put<int>('TQT2');
	tarFile->Put<int>(treeDepth);
	tarFile->Put<int>(tileSize);
	tarFile->Put<int>(0);   // format, 0 is "RAW"

	// write tqt2 table of contents, record start of table of contents
	toc.SetFixedSize(CountNodes(treeDepth));
	struct TocEntry emptyTocEntry = { 0, 0 };
	toc.Fill(0, toc.Size(), emptyTocEntry);
	int tocStart = tarFile->GetPosition();
	for (int i = 0; i < toc.Size(); i++)
	{
		tarFile->Put<int>(toc[i].pos);
		tarFile->Put<int>(toc[i].size);
	}

	ILuint SrcImg = iluGenImage();
	ilBindImage(SrcImg);
	ilEnable(IL_CONV_PAL);
	ilEnable(IL_ORIGIN_SET);
	ilOriginFunc(IL_ORIGIN_UPPER_LEFT); // Niello: BT files and corresponding TGAs index from lower-left, but DX (and hfchunk) index from upper-left
	ilLoadImage(srcFileName.Get());
	//ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE); // Niello: BGRA tga file wasn't converted here =\

	int Format = ilGetInteger(IL_IMAGE_FORMAT);
	int Width = ilGetInteger(IL_IMAGE_WIDTH);
	int Height = ilGetInteger(IL_IMAGE_HEIGHT);
	int BPP = ilGetInteger(IL_IMAGE_BPP);
	int SrcTileSizePixels = Height >> (treeDepth - 1);
	if (tileSize == 0) tileSize = SrcTileSizePixels;
	int DstTileSizeBytes = tileSize * tileSize * BPP;

	ILuint DstImg = iluGenImage();
	if (tileSize == SrcTileSizePixels)
	{
		ilBindImage(DstImg);
		ilTexImage(SrcTileSizePixels, SrcTileSizePixels, 1, BPP, IL_RGBA, IL_UNSIGNED_BYTE, NULL);
	}
	else
		iluImageParameter(ILU_FILTER, (tileSize > SrcTileSizePixels) ? ILU_SCALE_BELL : ILU_BILINEAR);
	
	void* pTileData = n_malloc(SrcTileSizePixels * SrcTileSizePixels * BPP);

	n_printf("\n\n-> Generating base level chunks...\n");
	for (int y = 0; y < Height; y += SrcTileSizePixels)
	{
		for (int x = 0; x < Width; x += SrcTileSizePixels)
		{
			ilBindImage(SrcImg);
			ilCopyPixels(x, y, 0, SrcTileSizePixels, SrcTileSizePixels, 1, Format, IL_UNSIGNED_BYTE, pTileData);

			ilBindImage(DstImg);
			if (tileSize != SrcTileSizePixels)
				ilTexImage(SrcTileSizePixels, SrcTileSizePixels, 1, BPP, IL_RGBA, IL_UNSIGNED_BYTE, pTileData);
			else
				ilSetPixels(0, 0, 0, SrcTileSizePixels, SrcTileSizePixels, 1, IL_RGBA, IL_UNSIGNED_BYTE, pTileData);
	
			//char s[1024];
			//sprintf(s, "_Dst_%d_%d_side_%d.bmp", x, y, SrcTileSizePixels);
			//ilSaveImage(s);

			//??? Niello: it didn't any data copying without errors.
			//if (!ilBlit(SrcImg, 0, 0, 0, x, y, 0, SrcTileSizePixels, SrcTileSizePixels, 1))
			//	n_error(iluErrorString(ilGetError()));
			
			if (tileSize != SrcTileSizePixels)
				iluScale(tileSize, tileSize, 1);

			// update table of contents in tqt2 file
			int quadTreeIndex = GetNodeIndex(treeDepth - 1, x / SrcTileSizePixels, y / SrcTileSizePixels);
			toc[quadTreeIndex].pos = tarFile->GetPosition();
			toc[quadTreeIndex].size = DstTileSizeBytes;

			tarFile->Write(ilGetData(), DstTileSizeBytes);

			n_printf("-> tile (level=%d, x=%d, y=%d) written\n", treeDepth - 1, x, y);
			fflush(stdout);
		}
    }

	n_free(pTileData);
	iluDeleteImage(SrcImg);
	iluDeleteImage(DstImg);

	// recursively generate sublevel tiles...
	n_printf("-> Generate sublevel chunks...\n");
	ILuint rootTileImage = RecurseGenerateTiles(0, 0, 0);
	iluDeleteImage(rootTileImage);

	// write TOC back to file
	tarFile->Seek(tocStart, Data::SSO_BEGIN);
	for (int i = 0; i < toc.Size(); i++)
	{
		tarFile->Put<int>(toc[i].pos);
		tarFile->Put<int>(toc[i].size);
	}

	// close and return
	CloseFiles();
	n_printf("-> Done.\n");
	return true;
}

//------------------------------------------------------------------------------
/**
    Copy one image into anther.
*/
void
nTqt2FileMaker::CopyImage(ILuint srcImage, ILuint dstImage, int dstX, int dstY)
{
    ilBindImage(srcImage);
    int srcW = ilGetInteger(IL_IMAGE_WIDTH);
    int srcH = ilGetInteger(IL_IMAGE_HEIGHT);
    uint* srcData = (uint*) ilGetData();
    ilBindImage(dstImage);
    int dstW = ilGetInteger(IL_IMAGE_WIDTH);
    // int dstH = ilGetInteger(IL_IMAGE_HEIGHT);
    uint* dstData = (uint*) ilGetData();
    int y;
    for (y = 0; y < srcH; y++)
    {
        int x;
        for (x = 0; x < srcW; x++)
        {
            uint* fromPtr = srcData + y * srcW + x;
            uint* toPtr = dstData + (y + dstY) * dstW + (x + dstX);
            *toPtr = *fromPtr;
        }
    }
}

//------------------------------------------------------------------------------
/**
    Recursively generates the sublevel files by reading 4 chunks, combining them
    into one image, scaling them to the tile size and writing the chunk 
    back to the target file.
*/
ILuint
nTqt2FileMaker::RecurseGenerateTiles(int level, int col, int row)
{
    const int bytesPerPixel = 4;

    int quadIndex = GetNodeIndex(level, col, row);
    int filePos = toc[quadIndex].pos;
    int dataSize = toc[quadIndex].size;
    if (filePos > 0)
    {
        // tile alread built, read it from the file into a new IL image
        ILuint newImage = iluGenImage();
        ilBindImage(newImage);
        ilTexImage(tileSize, tileSize, 1, bytesPerPixel, IL_RGBA, IL_UNSIGNED_BYTE, 0); 
        ILubyte* data = ilGetData();
        tarFile->Seek(filePos, Data::SSO_BEGIN);
        tarFile->Read(data, dataSize);
        return newImage;
    }

    // should never reach the bottom of the tree
    n_assert(level < (treeDepth - 1));

    // create an image which can hold 2x2 images
    ILuint tileImage = iluGenImage();
    ilBindImage(tileImage);
    ilTexImage(tileSize * 2, tileSize * 2, 1, bytesPerPixel, IL_RGBA, IL_UNSIGNED_BYTE, 0);
    ilClearColor(255, 0, 255, 255);
    ilClearImage();

    // resample the 4 children to make this tile
    int j;
    for (j = 0; j < 2; j++)
    {
        int i;
        for (i = 0; i < 2; i++)
        {
            int childCol = col * 2 + i;
            int childRow = row * 2 + j;
            ILuint childImage = RecurseGenerateTiles(level + 1, childCol, childRow);

            // copy the child image to its place in the tile image (topleft, topright, botleft or botright)
            int xOffset = i ? tileSize : 0;
            int yOffset = j ? tileSize : 0;
            //int yOffset = j ? 0 : tileSize;
            CopyImage(childImage, tileImage, xOffset, yOffset);

            // dispose of child image
            ilBindImage(childImage);
            iluDeleteImage(childImage);
        }
    }
    ilBindImage(tileImage);

    // scale tile image down to tile size
    iluScale(tileSize, tileSize, 1);

    // write the generated image to the tqt2 file
    tarFile->Seek(0, Data::SSO_END);
    filePos = tarFile->GetPosition();
    ILubyte* dstData = ilGetData();
    const int byteSize = tileSize * tileSize * bytesPerPixel;
    toc[quadIndex].pos = filePos;
    toc[quadIndex].size = byteSize;
    tarFile->Write(dstData, byteSize);
    n_printf("  -> tile (level=%d, col=%d, row=%d) written\n", level, col, row);
    fflush(stdout);

    // return the tile image to the caller
    return tileImage;
}
