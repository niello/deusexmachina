#include <ContentForgeTool.h>
#include <Utils.h>
#include <ParamsUtils.h>
#include <pugixml.hpp>

namespace fs = std::filesystem;

// Set working directory to $(TargetDir)
// Example args:
// -s src/terrain

static inline bool IsPow2(uint32_t Value) { return Value > 0 && (Value & (Value - 1)) == 0; }

template <class T> inline T NextPow2(T x)
{
	// For unsigned only, else uncomment the next line
	//if (x < 0) return 0;
	--x;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	return x + 1;
}

class CL3DTTool : public CContentForgeTool
{
public:

	CL3DTTool(const std::string& Name, const std::string& Desc, CVersion Version) :
		CContentForgeTool(Name, Desc, Version)
	{
	}

	virtual bool SupportsMultithreading() const override
	{
		// FIXME: must handle duplicate targets (for example 2 metafiles writing the same resulting file, like depth_atest_ps)
		// FIXME: CStrID
		return false;
	}

	virtual bool ProcessTask(CContentForgeTask& Task) override
	{
		// TODO: check whether the metafile can be processed by this tool

		const std::string Output = ParamsUtils::GetParam<std::string>(Task.Params, "Output", std::string{});
		const std::string TaskID(Task.TaskID.CStr());
		auto DestPath = fs::path(Output) / (TaskID + ".cdlod");
		if (!_RootDir.empty() && DestPath.is_relative())
			DestPath = fs::path(_RootDir) / DestPath;

		// Validate task params

		uint32_t PatchSize = static_cast<uint32_t>(ParamsUtils::GetParam<int>(Task.Params, "PatchSize", 32));
		if (!IsPow2(PatchSize) || PatchSize < 4 || PatchSize > 1024)
		{
			Task.Log.LogWarning("PatchSize must be power of 2 in range [4 .. 1024]");
			PatchSize = NextPow2(std::clamp<uint32_t>(PatchSize, 4, 1024));
		}

		uint32_t LODCount = static_cast<uint32_t>(ParamsUtils::GetParam<int>(Task.Params, "LODCount", 5));
		if (LODCount < 2 || LODCount > 64)
		{
			Task.Log.LogWarning("PatchSize must be in range [2 .. 64]");
			LODCount = std::clamp<uint32_t>(PatchSize, 2, 64);
		}

		// Read project XML

		pugi::xml_document XMLDoc;
		auto XMLResult = XMLDoc.load_buffer(Task.SrcFileData->data(), Task.SrcFileData->size());
		if (!XMLResult)
		{
			Task.Log.LogError(std::string("Error reading L3DT XML: ") + XMLResult.description());
			return false;
		}

		auto XMLRoot = XMLDoc.first_child();
		auto XMLVersion = XMLRoot.find_child_by_attribute("int", "name", "FileVersion");
		if (!XMLVersion || XMLVersion.text().as_int() != 1)
		{
			Task.Log.LogError(std::string("Unsupported version of L3DT XML: ") + XMLVersion.text().as_string());
			return false;
		}

		// Check all necessary maps exist in appropriate format

		// Write CDLOD file

		/*
		Data::CBuffer Buffer;
		IOSrv->LoadFileToBuffer(InFileName, Buffer);
		IO::CBTFile BTFile(Buffer.GetPtr());
		n_assert(BTFile.GetFileSize() == Buffer.GetSize());
		UPTR Width = BTFile.GetWidth();
		UPTR Height = BTFile.GetHeight();

		CString OutPath = PathUtils::ExtractDirName(OutFileName);
		if (!IOSrv->DirectoryExists(OutPath)) IOSrv->CreateDirectory(OutPath);

		IO::PStream OutFile = IOSrv->CreateStream(OutFileName);
		if (!OutFile->Open(IO::SAM_WRITE, IO::SAP_SEQUENTIAL))
		{
			n_msg(VL_ERROR, "Can't open output file" );
			return ExitApp(ERR_IO_WRITE, WaitKey);
		}

		UPTR PatchesW = (Width - 1 + PatchSize - 1) / PatchSize;
		UPTR PatchesH = (Height - 1 + PatchSize - 1) / PatchSize;
		UPTR TotalMinMaxDataSize = PatchesW * PatchesH;
		for (UPTR LOD = 1; LOD < LODCount; ++LOD)
		{
			PatchesW = (PatchesW + 1) / 2;
			PatchesH = (PatchesH + 1) / 2;
			TotalMinMaxDataSize += PatchesW * PatchesH;
		}
		TotalMinMaxDataSize *= 2 * sizeof(short);

		IO::CBinaryWriter Writer(*OutFile);
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
		UPTR HeightDataSize = (UPTR)BTFile.GetHFDataSize();
		short* pHeights = (short*)n_malloc(HeightDataSize);
		for (UPTR Row = 0; Row < Height; ++Row)
			for (UPTR Col = 0; Col < Width; ++Col)
				pHeights[Row * Width + Col] = BTFile.GetHeightsS()[Col * Height + Height - 1 - Row];

		bool FormatL16 = true;
		if (FormatL16)
		{
			unsigned short* pHFL16 = (unsigned short*)n_malloc(HeightDataSize);
			for (UPTR i = 0; i < BTFile.GetHeightCount(); ++i)
				pHFL16[i] = pHeights[i] + 32768;
			OutFile->Write(pHFL16, HeightDataSize);
			n_free(pHFL16);
		}
		else OutFile->Write(pHeights, HeightDataSize);

		PatchesW = (Width - 1 + PatchSize - 1) / PatchSize;
		PatchesH = (Height - 1 + PatchSize - 1) / PatchSize;

		UPTR MinMaxDataSize = PatchesW * PatchesH * 2 * sizeof(short);
		short* pMinMaxBuffer = (short*)n_malloc(MinMaxDataSize);

		// Generate top-level minmax map

		for (UPTR Row = 0; Row < PatchesH; ++Row)
		{
			UPTR StopAtZ = n_min((Row + 1) * PatchSize + 1, Height);

			for (UPTR Col = 0; Col < PatchesW; ++Col)
			{
				UPTR StopAtX = n_min((Col + 1) * PatchSize + 1, Width);

				short MinHeight = pHeights[Row * PatchSize * Width + Col * PatchSize];
				short MaxHeight = MinHeight;
				for (UPTR Z = Row * PatchSize; Z < StopAtZ; ++Z)
					for (UPTR X = Col * PatchSize; X < StopAtX; ++X)
					{
						short CurrHeight = pHeights[Z * Width + X];
						if (CurrHeight != IO::CBTFile::NoDataS)
						{
							if (CurrHeight < MinHeight) MinHeight = CurrHeight;
							else if (CurrHeight > MaxHeight) MaxHeight = CurrHeight;
						}
					}
				UPTR Idx = Row * PatchesW + Col;
				pMinMaxBuffer[Idx * 2] = MinHeight;
				pMinMaxBuffer[Idx * 2 + 1] = MaxHeight;
			}
		}

		n_free(pHeights);

		OutFile->Write(pMinMaxBuffer, MinMaxDataSize);

		// Generate minmax map hierarchy

		short* pPrevData = pMinMaxBuffer;
		UPTR PrevPatchesW = PatchesW;
		UPTR PrevPatchesH = PatchesH;
		for (UPTR LOD = 1; LOD < LODCount; ++LOD)
		{
			PatchesW = (PrevPatchesW + 1) / 2;
			PatchesH = (PrevPatchesH + 1) / 2;

			UPTR MinMaxDataSize = PatchesW * PatchesH * 2 * sizeof(short);
			short* pMinMaxBuffer = (short*)n_malloc(MinMaxDataSize);

			short* pSrc = pPrevData;
			short* pDst = pMinMaxBuffer;

			// Algorithm from the original CDLOD code

			for (UPTR i = 0; i < PatchesW * PatchesH * 2; i += 2)
			{
				pMinMaxBuffer[i] = 32767;
				pMinMaxBuffer[i + 1] = -32767;
			}

			for (UPTR Z = 0; Z < PrevPatchesH; ++Z)
			{
				for (UPTR X = 0; X < PrevPatchesW; ++X)
				{
					const int Idx = (X / 2) * 2;
					pDst[Idx] = n_min(pDst[Idx], pSrc[X * 2]);
					pDst[Idx + 1] = n_max(pDst[Idx + 1], pSrc[X * 2 + 1]);
				}
				pSrc += PrevPatchesW * 2;
				if (Z % 2 == 1) pDst += PatchesW * 2;
			}

			OutFile->Write(pMinMaxBuffer, MinMaxDataSize);

			n_free(pPrevData);
			pPrevData = pMinMaxBuffer;
			PrevPatchesW = PatchesW;
			PrevPatchesH = PatchesH;
		}

		n_free(pPrevData);
		*/

		// Merge alpha maps (optionally, if required)

		// Write material

		// Write scene file

		return true;
	}
};

int main(int argc, const char** argv)
{
	CL3DTTool Tool("cf-l3dt", "L3DT (Large 3D Terrain) to DeusExMachina scene asset converter", { 0, 1, 0 });
	return Tool.Execute(argc, argv);
}
