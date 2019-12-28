#include <ContentForgeTool.h>
#include <BTFile.h>
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

		const fs::path SrcFolder = Task.SrcFilePath.parent_path();

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
			Task.Log.LogWarning(std::string("Possibly unsupported version of L3DT XML: ") + XMLVersion.text().as_string());

		uint32_t Width = 0, Height = 0;
		float MinY = 0.f, MaxY = 0.f, HorizScale = 1.f;
		if (auto XMLTerrain = XMLRoot.find_child_by_attribute("name", "MapInfo").find_child_by_attribute("name", "Terrain"))
		{
			Width = XMLTerrain.find_child_by_attribute("int", "name", "nx").text().as_int();
			Height = XMLTerrain.find_child_by_attribute("int", "name", "ny").text().as_int();
			MinY = XMLTerrain.find_child_by_attribute("float", "name", "MinAlt").text().as_float();
			MaxY = XMLTerrain.find_child_by_attribute("float", "name", "MaxAlt").text().as_float();
			HorizScale = XMLTerrain.find_child_by_attribute("float", "name", "HorizScale").text().as_float();
		}

		// Check all necessary maps exist in appropriate formats

		auto XMLMaps = XMLRoot.find_child_by_attribute("name", "Maps");
		if (!XMLMaps)
		{
			Task.Log.LogError(std::string("No maps declared in L3DT XML"));
			return false;
		}

		fs::path HFPath;
		if (auto XMLHFMap = XMLMaps.find_child_by_attribute("name", "HF"))
		{
			HFPath = SrcFolder / XMLHFMap.find_child_by_attribute("name", "Filename").text().as_string();
			if (HFPath.extension() != ".bt")
			{
				Task.Log.LogError(std::string("HF map format must be BT"));
				return false;
			}
			if (!fs::exists(HFPath))
			{
				Task.Log.LogError(HFPath.generic_string() + " doesn't exist");
				return false;
			}
		}
		else
		{
			Task.Log.LogError(std::string("No HF map declared in L3DT XML"));
			return false;
		}

		fs::path TNPath;
		if (auto XMLTNMap = XMLMaps.find_child_by_attribute("name", "TN"))
		{
			TNPath = SrcFolder / XMLTNMap.find_child_by_attribute("name", "Filename").text().as_string();
			if (TNPath.extension() != ".dds")
			{
				Task.Log.LogError(std::string("TN map format must be DDS"));
				return false;
			}
			if (!fs::exists(TNPath))
			{
				Task.Log.LogError(TNPath.generic_string() + " doesn't exist");
				return false;
			}
		}
		else
		{
			Task.Log.LogError(std::string("No TN map declared in L3DT XML"));
			return false;
		}

		fs::path SplatMapPath;
		if (auto XMLAlpha1Map = XMLMaps.find_child_by_attribute("name", "Alpha_1"))
		{
			SplatMapPath = SrcFolder / XMLAlpha1Map.find_child_by_attribute("name", "Filename").text().as_string();

			const auto Ext = SplatMapPath.extension();
			if (Ext != ".dds" && Ext != ".tga")
			{
				Task.Log.LogError(std::string("Alpha (splatting) map format must be DDS or TGA"));
				return false;
			}
			if (!fs::exists(SplatMapPath))
			{
				Task.Log.LogError(SplatMapPath.generic_string() + " doesn't exist");
				return false;
			}
			if (!strcmp(XMLAlpha1Map.find_child_by_attribute("name", "MapType").text().as_string(), "BYTE"))
				Task.Log.LogWarning("Alpha (splatting) map is one channel. Use L3DT Professional to generate one multichannel map. It is freeware now.");
			if (XMLMaps.find_child_by_attribute("name", "Alpha_2"))
				Task.Log.LogWarning("Multiple alpha (splatting) maps are declared. Use L3DT Professional to generate one multichannel map. It is freeware now.");
		}
		else
		{
			Task.Log.LogError(std::string("No Alpha_1 map declared in L3DT XML, use Operations->Alpha maps->Generate maps..."));
			return false;
		}

		//!!!get climate, detect absolute path and warn/fail if not found locally!
		//may declare climate data (textures only?) in .meta
		//???offer to copy climate data to src folder and patch pathes in a project?
		//!!!it is hard to use climate without parsing AM, also textures aren't production!
		//probably explicit material declaration is better. Material can be hand-authored and set
		//into .meta instead of an effect.
		// Validate splat texture channel count is equal to splatting texture (ground type) count

		// Write CDLOD file

		//???TODO: unified normals+heightmap? 4-channel, single vertex texture fetch operation.
		//Physics will require separate data, but physics uses CPU RAM, and rendering uses VRAM.
		//Could create unified texture on loading, combining 3-channel normal map and HF.

		std::vector<char> HeightfieldFileData;
		if (!ReadAllFile(HFPath.string().c_str(), HeightfieldFileData))
		{
			Task.Log.LogError("HF map reading failed");
			return false;
		}

		if (HFPath.extension() == ".bt")
		{
			CBTFile BTFile(HeightfieldFileData.data());

			//if (HFSampleSize >= 4 && !BTFile.IsFloatData())
			//{
			//	Task.Log.LogError("Source BT file must store uncompressed float data!");
			//	return false;
			//}

			if (!Width) Width = BTFile.GetWidth();
			if (!Height) Height = BTFile.GetHeight();
			assert(Width == BTFile.GetWidth() && Height == BTFile.GetHeight());

			const uint32_t PatchesW = (Width - 1 + PatchSize - 1) / PatchSize;
			const uint32_t PatchesH = (Height - 1 + PatchSize - 1) / PatchSize;

			// TODO: support float BT!

			// S->N col-major to N->S row-major, convert to ushort for L16 texture format
			// TODO: there are different possible formats: D3DFMT_R16F, D3DFMT_R32F, D3DFMT_L16
			std::vector<uint16_t> HeightMap(Width * Height);
			for (uint32_t Row = 0; Row < Height; ++Row)
				for (uint32_t Col = 0; Col < Width; ++Col)
					HeightMap[Row * Width + Col] = static_cast<uint16_t>(static_cast<int>(BTFile.GetHeightsS()[Col * Height + Height - 1 - Row]) + 32768);

			// Calculate minmax data

			uint32_t CurrPatchesW = PatchesW;
			uint32_t CurrPatchesH = PatchesH;
			uint32_t TotalMinMaxDataSize = CurrPatchesW * CurrPatchesH;
			for (uint32_t LOD = 1; LOD < LODCount; ++LOD)
			{
				CurrPatchesW = (CurrPatchesW + 1) / 2;
				CurrPatchesH = (CurrPatchesH + 1) / 2;
				TotalMinMaxDataSize += CurrPatchesW * CurrPatchesH;
			}
			TotalMinMaxDataSize *= 2;

			std::vector<uint16_t> MinMaxData(TotalMinMaxDataSize);

			// Generate top-level minmax map
			for (uint32_t Row = 0; Row < PatchesH; ++Row)
			{
				uint32_t StopAtZ = std::min((Row + 1) * PatchSize + 1, Height);

				for (uint32_t Col = 0; Col < PatchesW; ++Col)
				{
					uint32_t StopAtX = std::min((Col + 1) * PatchSize + 1, Width);

					auto MinHeight = HeightMap[Row * PatchSize * Width + Col * PatchSize];
					auto MaxHeight = MinHeight;
					for (uint32_t Z = Row * PatchSize; Z < StopAtZ; ++Z)
					{
						for (uint32_t X = Col * PatchSize; X < StopAtX; ++X)
						{
							const auto CurrHeight = HeightMap[Z * Width + X];
							if (CurrHeight != CBTFile::NoDataUS)
							{
								if (CurrHeight < MinHeight) MinHeight = CurrHeight;
								else if (CurrHeight > MaxHeight) MaxHeight = CurrHeight;
							}
						}
					}

					const uint32_t Idx = Row * PatchesW + Col;
					MinMaxData[Idx * 2] = MinHeight;
					MinMaxData[Idx * 2 + 1] = MaxHeight;
				}
			}

			// Generate minmax map hierarchy
			size_t PrevDataOffset = 0;
			size_t CurrDataOffset = PatchesW * PatchesH * 2;
			auto PrevPatchesW = PatchesW;
			auto PrevPatchesH = PatchesH;
			for (uint32_t LOD = 1; LOD < LODCount; ++LOD)
			{
				auto CurrPatchesW = (PrevPatchesW + 1) / 2;
				auto CurrPatchesH = (PrevPatchesH + 1) / 2;
				auto CurrDataSize = CurrPatchesW * CurrPatchesH * 2;

				auto* pSrc = MinMaxData.data() + PrevDataOffset;
				auto* pDst = MinMaxData.data() + CurrDataOffset;

				// Algorithm from the original CDLOD code modified for unsigned values

				for (uint32_t i = 0; i < CurrDataSize; i += 2)
				{
					pDst[i] = 65535;
					pDst[i + 1] = 0;
				}

				for (uint32_t Z = 0; Z < PrevPatchesH; ++Z)
				{
					for (uint32_t X = 0; X < PrevPatchesW; ++X)
					{
						const int Idx = (X / 2) * 2;
						pDst[Idx] = std::min(pDst[Idx], pSrc[X * 2]);
						pDst[Idx + 1] = std::max(pDst[Idx + 1], pSrc[X * 2 + 1]);
					}
					pSrc += PrevPatchesW * 2;
					if (Z % 2 == 1) pDst += CurrPatchesW * 2;
				}

				PrevDataOffset = CurrDataOffset;
				CurrDataOffset += CurrDataSize;
				PrevPatchesW = CurrPatchesW;
				PrevPatchesH = CurrPatchesH;
			}

			// Write resulting CDLOD file

			{
				fs::create_directories(DestPath.parent_path());

				std::ofstream File(DestPath, std::ios_base::binary | std::ios_base::trunc);
				if (!File)
				{
					Task.Log.LogError("Error opening an output file " + DestPath.generic_string());
					return false;
				}

				WriteStream<uint32_t>(File, 'CDLD');        // Format magic value
				WriteStream<uint32_t>(File, 0x00010000);    // Version 0.1.0.0
				WriteStream(File, Width);
				WriteStream(File, Height);
				WriteStream(File, PatchSize);
				WriteStream(File, LODCount);
				WriteStream(File, TotalMinMaxDataSize * sizeof(short));
				WriteStream(File, BTFile.GetVerticalScale());
				WriteStream(File, static_cast<float>(BTFile.GetLeftExtent())); // Min X
				WriteStream(File, static_cast<float>(BTFile.GetRightExtent())); // Max X
				WriteStream(File, static_cast<float>(BTFile.GetBottomExtent())); // Min Z
				WriteStream(File, static_cast<float>(BTFile.GetTopExtent())); // Max Z
				WriteStream(File, BTFile.GetMinHeight()); // Min Y
				WriteStream(File, BTFile.GetMaxHeight()); // Max Y

				File.write(reinterpret_cast<const char*>(HeightMap.data()), HeightMap.size() * sizeof(uint16_t));
				File.write(reinterpret_cast<const char*>(MinMaxData.data()), MinMaxData.size() * sizeof(uint16_t));
			}
		}
		else
		{
			// TODO: support different formats
			assert(false);
		}

		// Write material file

		// Write scene file

		return true;
	}
};

int main(int argc, const char** argv)
{
	CL3DTTool Tool("cf-l3dt", "L3DT (Large 3D Terrain) to DeusExMachina scene asset converter", { 0, 1, 0 });
	return Tool.Execute(argc, argv);
}
