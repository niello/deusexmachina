#include <Data/DataServer.h>
#include <Data/BTFile.h>
#include <Data/Buffer.h>
#include <Data/Streams/FileStream.h>
#include <Data/BinaryWriter.h>
#include "ncmdlineargs.h"
#include <IL/il.h>
#include <IL/ilu.h>

#undef CreateDirectory
#undef CopyFile

using namespace Data;

int main(int argc, const char** argv)
{
	// Debug cmd line
	// -patch 16 -lod 4 -in "..\..\..\..\InsanePoet\Content\Src\Terrain\Eger Cathedral Courtyard\ECCY_HF.bt" -out ECCY -proj "..\..\..\..\InsanePoet\Content\"

	nCmdLineArgs Args(argc, argv);
	bool Help = Args.GetBoolArg("-help");

	if (Help)
	{
		printf(	"ConvHF - DeusExMachina heightfield CDLOD conversion tool\n"
				"Command line args:\n"
				"------------------\n"
				"-help                 show this help\n"
				"-in [filename]        input file (.bt)\n" //LATER may be also .png, .bmp, .raw, .r32
				"-out [rsrc id]        resulting resource ID\n"
				"-proj [dirname]       project directory\n"
				"-patch [uint]         quads per patch edge, where quad is formed by a triangle pair\n"
				"-lod [uint]           LOD count, including finest LOD 0\n");
		return 5;
	}

	nString ProjDir = Args.GetStringArg("-proj");
	nString HFFileName = Args.GetStringArg("-in");
	nString RsrcID = Args.GetStringArg("-out");
	DWORD PatchSize = Args.GetIntArg("-patch");
	DWORD LODCount = Args.GetIntArg("-lod");

	if (ProjDir.IsEmpty())
	{
	    printf("ConvHF error: No input file, output resource or project directory!\n");
	    return 5;
	}

	if (!IsPow2(PatchSize) || PatchSize < 4 || PatchSize > 1024)
	{
		printf("ConvHF error: PatchSize must be pow of 2 in 4 .. 1024 range" );
		return 5;
	}

	if (LODCount < 2) // || LODCount > MAX_LOD_COUNT)
	{
		printf("ConvHF error: LODCount must be > 1" );
		return 5;
	}

	Ptr<Data::CDataServer> DataServer;
	DataServer.Create();
	DataSrv->SetAssign("proj", ProjDir);
	DataSrv->SetAssign("src", "proj:Src");
	DataSrv->SetAssign("export", "proj:Export");
	DataSrv->SetAssign("terrain", "proj:Export/Terrain");

	if (HFFileName.CheckExtension("bt"))
	{
		CBuffer Buffer;
		DataSrv->LoadFileToBuffer(HFFileName, Buffer);
		CBTFile BTFile(Buffer.GetPtr());
		n_assert(BTFile.GetFileSize() == Buffer.GetSize());
		DWORD Width = BTFile.GetWidth();
		DWORD Height = BTFile.GetHeight();

		nString OutFileName = "terrain:" + RsrcID + ".cdlod";
		nString OutPath = OutFileName.ExtractDirName();
		if (!DataSrv->DirectoryExists(OutPath)) DataSrv->CreateDirectory(OutPath);

		Data::CFileStream OutFile;
		if (!OutFile.Open(OutFileName, Data::SAM_WRITE, Data::SAP_SEQUENTIAL))
		{
			printf("ConvHF error: Can't open output file" );
			return 5;
		}

		DWORD PatchesW = (Width - 1 + PatchSize - 1) / PatchSize;
		DWORD PatchesH = (Height - 1 + PatchSize - 1) / PatchSize;
		DWORD TotalMinMaxDataSize = PatchesW * PatchesH;
		for (DWORD LOD = 1; LOD < LODCount; ++LOD)
		{
			PatchesW = (PatchesW + 1) / 2;
			PatchesH = (PatchesH + 1) / 2;
			TotalMinMaxDataSize += PatchesW * PatchesH;
		}
		TotalMinMaxDataSize *= 2 * sizeof(short);

		Data::CBinaryWriter Writer(OutFile);
		Writer.Write('CDLD');							// Magic
		Writer.Write((DWORD)1);							// Version
		Writer.Write(Width);
		Writer.Write(Height);
		Writer.Write(PatchSize);
		Writer.Write(LODCount);
		Writer.Write(TotalMinMaxDataSize);
		Writer.Write(BTFile.GetVerticalScale());
		Writer.Write((float)BTFile.GetLeftExtent());	// Min X
		Writer.Write(BTFile.GetMinHeight());			// Min Y
		Writer.Write((float)BTFile.GetBottomExtent());	// Min Z
		Writer.Write((float)BTFile.GetRightExtent());	// Max X
		Writer.Write(BTFile.GetMaxHeight());			// Max Y
		Writer.Write((float)BTFile.GetTopExtent());		// Max Z

		// S->N col-major to N->S row-major
		DWORD HeightDataSize = BTFile.GetHFDataSize();
		short* pHeights = (short*)n_malloc(HeightDataSize);
		for (DWORD Row = 0; Row < Height; ++Row)
			for (DWORD Col = 0; Col < Width; ++Col)
				pHeights[Row * Width + Col] = BTFile.GetHeightsS()[Col * Height + Height - 1 - Row];

		bool FormatL16 = true;
		if (FormatL16)
		{
			unsigned short* pHFL16 = (unsigned short*)n_malloc(HeightDataSize);
			for (DWORD i = 0; i < BTFile.GetHeightCount(); ++i)
				pHFL16[i] = pHeights[i] + 32768;
			OutFile.Write(pHFL16, HeightDataSize);
			n_free(pHFL16);
		}
		else OutFile.Write(pHeights, HeightDataSize);

		PatchesW = (Width - 1 + PatchSize - 1) / PatchSize;
		PatchesH = (Height - 1 + PatchSize - 1) / PatchSize;

		DWORD MinMaxDataSize = PatchesW * PatchesH * 2 * sizeof(short);
		short* pMinMaxBuffer = (short*)n_malloc(MinMaxDataSize);

		// Generate top-level minmax map

		for (DWORD Row = 0; Row < PatchesH; ++Row)
		{
			DWORD StopAtZ = n_min((Row + 1) * PatchSize + 1, Height);

			for (DWORD Col = 0; Col < PatchesW; ++Col)
			{
				DWORD StopAtX = n_min((Col + 1) * PatchSize + 1, Width);

				short MinHeight = 32767;
				short MaxHeight = -32767;
				for (DWORD Z = Row * PatchSize; Z < StopAtZ; ++Z)
					for (DWORD X = Col * PatchSize; X < StopAtX; ++X)
					{
						short CurrHeight = pHeights[Z * Width + X];
						if (CurrHeight != CBTFile::NoDataS)
						{
							if (CurrHeight < MinHeight) MinHeight = CurrHeight;
							else if (CurrHeight > MaxHeight) MaxHeight = CurrHeight;
						}
					}
				DWORD Idx = Row * PatchesW + Col;
				pMinMaxBuffer[Idx * 2] = MinHeight;
				pMinMaxBuffer[Idx * 2 + 1] = MaxHeight;
			}
		}

		n_free(pHeights);

		OutFile.Write(pMinMaxBuffer, MinMaxDataSize);

		// Generate minmax map hierarchy

		short* pPrevData = pMinMaxBuffer;
		DWORD PrevPatchesW = PatchesW;
		DWORD PrevPatchesH = PatchesH;
		for (DWORD LOD = 1; LOD < LODCount; ++LOD)
		{
			PatchesW = (PrevPatchesW + 1) / 2;
			PatchesH = (PrevPatchesH + 1) / 2;

			DWORD MinMaxDataSize = PatchesW * PatchesH * 2 * sizeof(short);
			short* pMinMaxBuffer = (short*)n_malloc(MinMaxDataSize);

			short* pSrc = pPrevData;
			short* pDst = pMinMaxBuffer;

			// Algorithm from the original CDLOD code

			for (DWORD i = 0; i < PatchesW * PatchesH * 2; i += 2)
			{
				pMinMaxBuffer[i] = 32767;
				pMinMaxBuffer[i + 1] = -32767;
			}

			for (DWORD y = 0; y < PrevPatchesH; ++y)
			{
				for (DWORD x = 0; x < PrevPatchesW; ++x)
				{
					const int Idx = x / 2;
					pDst[Idx * 2] = n_min(pDst[Idx * 2], pSrc[x * 2]);
					pDst[Idx * 2 + 1] = n_max(pDst[Idx * 2 + 1], pSrc[x * 2]);
				}
				pSrc += PrevPatchesW * 2;
				if (y % 2 == 1) pDst += PatchesW * 2;
			}

			OutFile.Write(pMinMaxBuffer, MinMaxDataSize);

			n_free(pPrevData);
			pPrevData = pMinMaxBuffer;
			PrevPatchesW = PatchesW;
			PrevPatchesH = PatchesH;
		}

		n_free(pPrevData);
	}
	else
	{
		printf("ConvHF error: Can't load input HF!\n");
		return 5;
	}

	return 0;
}
//---------------------------------------------------------------------

/*
	else if (HFFileName.CheckExtension("raw") || HFFileName.CheckExtension("r32"))
	{
	}
	else if (HFFileName.CheckExtension("png"))
	{
		// IT IS NOT A CODE, IT IS AN EXAMPLE OF IL USAGE

		ilInit();
		iluInit();

		ILuint SrcImg = iluGenImage();
		ilBindImage(SrcImg);
		ilEnable(IL_CONV_PAL);
		ilEnable(IL_ORIGIN_SET);
		ilOriginFunc(IL_ORIGIN_UPPER_LEFT); // L3DT origin is lower-left, but DX is upper-left
		if (!ilLoadImage(HFFileName.Get()))
		{
			printf("ConvHF error: Can't load input HF image!\n");
			return 5;
		}

		int Format = ilGetInteger(IL_IMAGE_FORMAT);
		int Width = ilGetInteger(IL_IMAGE_WIDTH);
		int Height = ilGetInteger(IL_IMAGE_HEIGHT);
		int BPP = ilGetInteger(IL_IMAGE_BPP);

		ILuint DstImg = iluGenImage();
		if (tileSize == SrcTileSizePixels)
		{
			ilBindImage(DstImg);
			ilTexImage(SrcTileSizePixels, SrcTileSizePixels, 1, BPP, IL_RGBA, IL_UNSIGNED_BYTE, NULL);
		}
		else
			iluImageParameter(ILU_FILTER, (tileSize > SrcTileSizePixels) ? ILU_SCALE_BELL : ILU_BILINEAR);

		ilBindImage(SrcImg);
		ilCopyPixels(x, y, 0, SrcTileSizePixels, SrcTileSizePixels, 1, Format, IL_UNSIGNED_BYTE, pTileData);

		ilBindImage(DstImg);
		if (tileSize != SrcTileSizePixels)
			ilTexImage(SrcTileSizePixels, SrcTileSizePixels, 1, BPP, IL_RGBA, IL_UNSIGNED_BYTE, pTileData);
		else
			ilSetPixels(0, 0, 0, SrcTileSizePixels, SrcTileSizePixels, 1, IL_RGBA, IL_UNSIGNED_BYTE, pTileData);

		iluDeleteImage(SrcImg);
		iluDeleteImage(DstImg);
	}
*/
