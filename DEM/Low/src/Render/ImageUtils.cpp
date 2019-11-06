#include "ImageUtils.h"

#include <Render/RenderFwd.h>
#include <Data/Regions.h>

namespace Render
{

void __fastcall CopyImage(const CImageData& Src, const CImageData& Dest, UPTR Flags, const CCopyImageParams& Params)
{
	const IPTR X = 0, Y = 1, Z = 2;

	UPTR BitsPerBlock, OffsetBytesX, OffsetBlocksY, CopyBlocksW, CopyBlocksH, TotalBlocksW, TotalBlocksH;
	if (Flags & CopyImage_BlockCompressed)
	{
		// Here was universal BlockSize-based code, look at revisions near r420 if you need it
		// Block-compressed 3D textures would compress each slice separately, don't recalculate Depth
		BitsPerBlock = Params.BitsPerPixel << 4;
		OffsetBytesX = (Params.Offset[X] >> 2) * Params.BitsPerPixel << 1;
		OffsetBlocksY = Params.Offset[Y] >> 2;
		CopyBlocksW = (Params.CopySize[X] + 3) >> 2;
		CopyBlocksH = (Params.CopySize[Y] + 3) >> 2;
		TotalBlocksW = (Params.TotalSize[X] + 3) >> 2;
		TotalBlocksH = (Params.TotalSize[Y] + 3) >> 2;
	}
	else
	{
		BitsPerBlock = Params.BitsPerPixel;
		OffsetBytesX = (Params.Offset[X] * Params.BitsPerPixel) >> 3;
		OffsetBlocksY = Params.Offset[Y];
		CopyBlocksW = Params.CopySize[X];
		CopyBlocksH = Params.CopySize[Y];
		TotalBlocksW = Params.TotalSize[X];
		TotalBlocksH = Params.TotalSize[Y];
	}

	const char* pSrc = Src.pData;
	if (Flags & CopyImage_AdjustSrc)
		pSrc += Params.Offset[Z] * Src.SlicePitch + OffsetBlocksY * Src.RowPitch + OffsetBytesX;

	char* pDest = Dest.pData;
	if (Flags & CopyImage_AdjustDest)
		pDest += Params.Offset[Z] * Dest.SlicePitch + OffsetBlocksY * Dest.RowPitch + OffsetBytesX;

#ifdef _DEBUG
	// Don't check adjusted pointers, user can't align offsets
	if ((!IsAligned16(pSrc) && !(Flags & CopyImage_AdjustSrc))  || (!IsAligned16(pDest) && !(Flags & CopyImage_AdjustDest)))
		Sys::DbgOut("Render::CopyImage() > Either pSrc or pDest is not aligned at 16 bytes boundary. Align your pointers to achieve MUCH faster copying.\n");
#endif

	UPTR Depth = Params.CopySize[Z];
	const bool WholeRow = (CopyBlocksW == TotalBlocksW && Src.RowPitch == Dest.RowPitch);
	const bool WholeSlice = (WholeRow && (Flags & CopyImage_3DImage) && CopyBlocksH == TotalBlocksH && Src.SlicePitch == Dest.SlicePitch);
	if (WholeSlice)
	{
		UPTR DataSize = Src.SlicePitch * Depth;
		memcpy(pDest, pSrc, DataSize);
	}
	else if (WholeRow)
	{
		UPTR SlicePartSize = Src.RowPitch * CopyBlocksH;
		for (UPTR i = 0; i < Depth; ++i)
		{
			memcpy(pDest, pSrc, SlicePartSize);
			pSrc += Src.SlicePitch;
			pDest += Dest.SlicePitch;
		}
	}
	else
	{
		UPTR RowPartSize = (BitsPerBlock * CopyBlocksW) >> 3;
		for (UPTR i = 0; i < Depth; ++i)
		{
			const char* pSrcRow = pSrc;
			char* pDestRow = pDest;
			for (UPTR j = 0; j < CopyBlocksH; ++j)
			{
				memcpy(pDestRow, pSrcRow, RowPartSize);
				pSrcRow += Src.RowPitch;
				pDestRow += Dest.RowPitch;
			}

			pSrc += Src.SlicePitch;
			pDest += Dest.SlicePitch;
		}
	}
}
//---------------------------------------------------------------------

bool __fastcall CalcValidImageRegion(const Data::CBox* pInRegion, UPTR Dimensions,
									 UPTR ImageWidth, UPTR ImageHeight, UPTR ImageDepth,
									 UPTR& OutOffsetX, UPTR& OutOffsetY, UPTR& OutOffsetZ,
									 UPTR& OutSizeX, UPTR& OutSizeY, UPTR& OutSizeZ)
{
	if (pInRegion)
	{
		int OffsetX = n_max(pInRegion->X, 0);
		int SizeX = n_min(pInRegion->W, ImageWidth - OffsetX);
		if (SizeX <= 0) FAIL;

		if (Dimensions > 1)
		{
			int OffsetY = n_max(pInRegion->Y, 0);
			int SizeY = n_min(pInRegion->H, ImageHeight - OffsetY);
			if (SizeY <= 0) FAIL;

			if (Dimensions > 2)
			{
				int OffsetZ = n_max(pInRegion->Z, 0);
				int SizeZ = n_min(pInRegion->D, ImageDepth - OffsetZ);
				if (SizeZ <= 0) FAIL;

				OutOffsetZ = OffsetZ;
				OutSizeZ = SizeZ;
			}
			else
			{
				OutOffsetZ = 0;
				OutSizeZ = 1;
			}

			OutOffsetY = OffsetY;
			OutSizeY = SizeY;
		}
		else
		{
			OutOffsetY = 0;
			OutSizeY = 1;
		}

		OutOffsetX = OffsetX;
		OutSizeX = SizeX;
	}
	else
	{
		OutOffsetX = 0;
		OutOffsetY = 0;
		OutOffsetZ = 0;
		OutSizeX = ImageWidth;
		OutSizeY = ImageHeight;
		OutSizeZ = ImageDepth;
	}

	OK;
}
//---------------------------------------------------------------------

}