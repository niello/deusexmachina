#include <IO/IOServer.h>
#include <IO/BTFile.h>
#include <IO/Streams/FileStream.h>
#include <IO/BinaryWriter.h>
#include <Data/Buffer.h>
#include <ConsoleApp.h>

#define TOOL_NAME		"CFTerrain"
#define VERSION			"1.0"
#define CDLOD_VERSION	((DWORD)1)

int		Verbose = VL_ERROR;

int		ExitApp(int Code, bool WaitKey);

int main(int argc, const char** argv)
{
	nCmdLineArgs Args(argc, argv);

	bool WaitKey = Args.GetBoolArg("-waitkey");
	Verbose = Args.GetIntArg("-v");

	bool Help = Args.GetBoolArg("-help") || Args.GetBoolArg("/?");

	if (Help)
	{
		printf(	TOOL_NAME" v"VERSION" - DeusExMachina heightfield to CDLOD conversion tool\n"
				"Command line args:\n"
				"------------------\n"
				"-help                 show this help\n"
				"-in [filename]        input file (.bt) or resource desc\n" //LATER may be also .png, .bmp, .raw, .r32
				"-out [filename]       resulting CDLOD file\n"
				"-patch [uint]         quads per patch edge, where quad is formed by a triangle pair\n"
				"-lod [uint]           LOD count, including finest LOD 0\n");

		return ExitApp(SUCCESS_HELP, WaitKey);
	}

	nString InFileName = Args.GetStringArg("-in");
	nString OutFileName = Args.GetStringArg("-out");
	DWORD PatchSize = Args.GetIntArg("-patch");
	DWORD LODCount = Args.GetIntArg("-lod");

	if (InFileName.IsEmpty() || OutFileName.IsEmpty())
	{
	    n_msg(VL_ERROR, "Specify -in and -out files\n");
	    return ExitApp(ERR_INVALID_CMD_LINE, WaitKey);
	}

	if (!IsPow2(PatchSize) || PatchSize < 4 || PatchSize > 1024)
	{
		n_msg(VL_ERROR, "PatchSize must be pow of 2 in 4 .. 1024 range" );
	    return ExitApp(ERR_INVALID_CMD_LINE, WaitKey);
	}

	if (LODCount < 2) // || LODCount > MAX_LOD_COUNT)
	{
		n_msg(VL_ERROR, "LODCount must be > 1" );
	    return ExitApp(ERR_INVALID_CMD_LINE, WaitKey);
	}

	Ptr<IO::CIOServer> IOServer = n_new(IO::CIOServer);

	if (InFileName.CheckExtension("bt"))
	{
		Data::CBuffer Buffer;
		IOSrv->LoadFileToBuffer(InFileName, Buffer);
		IO::CBTFile BTFile(Buffer.GetPtr());
		n_assert(BTFile.GetFileSize() == Buffer.GetSize());
		DWORD Width = BTFile.GetWidth();
		DWORD Height = BTFile.GetHeight();

		nString OutPath = OutFileName.ExtractDirName();
		if (!IOSrv->DirectoryExists(OutPath)) IOSrv->CreateDirectory(OutPath);

		IO::CFileStream OutFile;
		if (!OutFile.Open(OutFileName, IO::SAM_WRITE, IO::SAP_SEQUENTIAL))
		{
			n_msg(VL_ERROR, "Can't open output file" );
			return ExitApp(ERR_IO_WRITE, WaitKey);
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

		IO::CBinaryWriter Writer(OutFile);
		Writer.Write('CDLD');							// Magic
		Writer.Write(CDLOD_VERSION);
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

				short MinHeight = pHeights[Row * PatchSize * Width + Col * PatchSize];
				short MaxHeight = MinHeight;
				for (DWORD Z = Row * PatchSize; Z < StopAtZ; ++Z)
					for (DWORD X = Col * PatchSize; X < StopAtX; ++X)
					{
						short CurrHeight = pHeights[Z * Width + X];
						if (CurrHeight != IO::CBTFile::NoDataS)
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

			for (DWORD Z = 0; Z < PrevPatchesH; ++Z)
			{
				for (DWORD X = 0; X < PrevPatchesW; ++X)
				{
					const int Idx = (X / 2) * 2;
					pDst[Idx] = n_min(pDst[Idx], pSrc[X * 2]);
					pDst[Idx + 1] = n_max(pDst[Idx + 1], pSrc[X * 2 + 1]);
				}
				pSrc += PrevPatchesW * 2;
				if (Z % 2 == 1) pDst += PatchesW * 2;
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
		n_msg(VL_ERROR, "Can't load input HF");
		return ExitApp(ERR_IO_READ, WaitKey);
	}

	return ExitApp(SUCCESS, WaitKey);
}
//---------------------------------------------------------------------

int ExitApp(int Code, bool WaitKey)
{
	if (Code != SUCCESS) n_msg(VL_ERROR, TOOL_NAME" v"VERSION": Error occured with code %d\n", Code);

	if (WaitKey)
	{
		n_printf("\nPress any key to exit...\n");
		getch();
	}

	return Code;
}
//---------------------------------------------------------------------

/*
	else if (InFile.CheckExtension("raw") || InFile.CheckExtension("r32"))
	{
	}
	else if (InFile.CheckExtension("png"))
	{
		// IT IS NOT A CODE, IT IS AN EXAMPLE OF IL USAGE
//#include <IL/il.h>
//#include <IL/ilu.h>

		ilInit();
		iluInit();

		ILuint SrcImg = iluGenImage();
		ilBindImage(SrcImg);
		ilEnable(IL_CONV_PAL);
		ilEnable(IL_ORIGIN_SET);
		ilOriginFunc(IL_ORIGIN_UPPER_LEFT); // L3DT origin is lower-left, but DX is upper-left
		if (!ilLoadImage(InFile.Get()))
		{
			printf("Can't load input HF image!\n");
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
