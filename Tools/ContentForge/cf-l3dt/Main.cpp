#include <ContentForgeTool.h>
#include <Render/ShaderMetaCommon.h>
#include <BTFile.h>
#include <Utils.h>
#include <ParamsUtils.h>
#include <CLI11.hpp>
#include <pugixml.hpp>

namespace fs = std::filesystem;

// Set working directory to $(TargetDir)
// Example args:
// -s src/terrain --path Data ../../../content

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
protected:

	Data::CSchemeSet          _SceneSchemes;

	std::map<std::string, std::string> _EffectsByType;
	std::map<std::string, std::string> _EffectParamAliases;

	std::string               _ResourceRoot;
	std::string               _SchemeFile;
	std::string               _SettingsFile;
	bool                      _OutputBin = false;
	bool                      _OutputHRD = false; // For debug purposes, saves scene hierarchies in a human-readable format

public:

	CL3DTTool(const std::string& Name, const std::string& Desc, CVersion Version)
		: CContentForgeTool(Name, Desc, Version)
		, _ResourceRoot("Data:")
	{
		// Set default before parsing command line
		_SchemeFile = "../schemes/scene.dss";
		_SettingsFile = "../schemes/settings.hrd";
	}

	virtual bool SupportsMultithreading() const override
	{
		// FIXME: must handle duplicate targets (for example 2 metafiles writing the same resulting file, like depth_atest_ps)
		// FIXME: CStrID
		return false;
	}

	virtual int Init() override
	{
		if (_ResourceRoot.empty())
			if (_LogVerbosity >= EVerbosity::Warnings)
				std::cout << "Resource root is empty, external references may not be resolved from the game!";

		if (!_OutputHRD) _OutputBin = true;

		if (_OutputBin)
		{
			if (!ParamsUtils::LoadSchemes(_SchemeFile.c_str(), _SceneSchemes))
			{
				std::cout << "Couldn't load scene binary serialization scheme from " << _SchemeFile;
				return 2;
			}
		}

		{
			Data::CParams EffectSettings;
			if (!ParamsUtils::LoadParamsFromHRD(_SettingsFile.c_str(), EffectSettings))
			{
				std::cout << "Couldn't load effect settings from " << _SettingsFile;
				return 3;
			}

			const Data::CParams* pMap;
			if (ParamsUtils::TryGetParam(pMap, EffectSettings, "Effects"))
			{
				for (const auto& Pair : *pMap)
					_EffectsByType.emplace(Pair.first.ToString(), Pair.second.GetValue<std::string>());
			}
			if (ParamsUtils::TryGetParam(pMap, EffectSettings, "Params"))
			{
				for (const auto& Pair : *pMap)
					_EffectParamAliases.emplace(Pair.first.ToString(), Pair.second.GetValue<std::string>());
			}
		}

		return 0;
	}

	virtual void ProcessCommandLine(CLI::App& CLIApp) override
	{
		//???use --project-file instead of --res-root + --settings?
		CContentForgeTool::ProcessCommandLine(CLIApp);
		CLIApp.add_option("--res-root", _ResourceRoot, "Resource root prefix for referencing external subresources by path");
		CLIApp.add_option("--scheme,--schema", _SchemeFile, "Scene binary serialization scheme file path");
		CLIApp.add_option("--settings", _SettingsFile, "Settings file path");
		CLIApp.add_flag("-t,--txt", _OutputHRD, "Output scenes in a human-readable format, suitable for debugging only");
		CLIApp.add_flag("-b,--bin", _OutputBin, "Output scenes in a binary format, suitable for loading into the engine");
	}

	fs::path GetPath(const Data::CParams& TaskParams, const char* pPathID)
	{
		fs::path Result;

		std::string PathValue;
		if (ParamsUtils::TryGetParam(PathValue, TaskParams, pPathID))
			Result = PathValue;
		else if (ParamsUtils::TryGetParam(PathValue, TaskParams, "Output"))
			Result = PathValue;
		else return Result;

		if (!_RootDir.empty() && Result.is_relative())
			Result = fs::path(_RootDir) / Result;

		return Result;
	}

	const std::string& GetEffectParamID(const std::string& Alias)
	{
		auto It = _EffectParamAliases.find(Alias);
		return (It == _EffectParamAliases.cend()) ? Alias : It->second;
	}

	virtual bool ProcessTask(CContentForgeTask& Task) override
	{
		// TODO: check whether the metafile can be processed by this tool

		const std::string TaskName = Task.TaskID.ToString();

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

		// Write CDLOD file

		std::vector<char> HeightfieldFileData;
		if (!ReadAllFile(HFPath.string().c_str(), HeightfieldFileData))
		{
			Task.Log.LogError("HF map reading failed");
			return false;
		}

		std::string CDLODID;
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
			// TODO: there are different possible formats: D3DFMT_R16F, D3DFMT_R32F, D3DFMT_L16
			//???TODO: unified normals+heightmap? 4-channel, single vertex texture fetch operation.
			//Physics will require separate data, but physics uses CPU RAM, and rendering uses VRAM.
			//Could create unified texture on loading, combining 3-channel normal map and HF.

			// S->N col-major to N->S row-major, convert to ushort for D3DFMT_L16 texture format
			std::vector<uint16_t> HeightMap(Width * Height);
			for (uint32_t Row = 0; Row < Height; ++Row)
				for (uint32_t Col = 0; Col < Width; ++Col)
					HeightMap[Row * Width + Col] = static_cast<uint16_t>(static_cast<int>(BTFile.GetHeightsS()[Col * Height + Height - 1 - Row]) + 32768);

			// Calculate minmax data

			uint32_t CurrPatchesW = PatchesW;
			uint32_t CurrPatchesH = PatchesH;
			uint32_t TotalMinMaxDataCount = CurrPatchesW * CurrPatchesH;
			for (uint32_t LOD = 1; LOD < LODCount; ++LOD)
			{
				CurrPatchesW = (CurrPatchesW + 1) / 2;
				CurrPatchesH = (CurrPatchesH + 1) / 2;
				TotalMinMaxDataCount += CurrPatchesW * CurrPatchesH;
			}
			TotalMinMaxDataCount *= 2;

			std::vector<uint16_t> MinMaxData(TotalMinMaxDataCount);

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
				auto DestPath = GetPath(Task.Params, "CDLODOutput") / (TaskName + ".cdlod");
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
				WriteStream(File, TotalMinMaxDataCount);
				WriteStream(File, BTFile.GetVerticalScale());
				WriteStream(File, static_cast<float>(BTFile.GetLeftExtent())); // Min X
				WriteStream(File, static_cast<float>(BTFile.GetRightExtent())); // Max X
				WriteStream(File, static_cast<float>(BTFile.GetBottomExtent())); // Min Z
				WriteStream(File, static_cast<float>(BTFile.GetTopExtent())); // Max Z
				WriteStream(File, BTFile.GetMinHeight()); // Min Y
				WriteStream(File, BTFile.GetMaxHeight()); // Max Y

				File.write(reinterpret_cast<const char*>(HeightMap.data()), HeightMap.size() * sizeof(uint16_t));

				// Convert minmax back to signed
				// FIXME: improve?
				//File.write(reinterpret_cast<const char*>(MinMaxData.data()), MinMaxData.size() * sizeof(uint16_t));
				for (uint16_t Value : MinMaxData)
					WriteStream(File, static_cast<int16_t>(static_cast<int>(Value) - 32768));

				CDLODID = _ResourceRoot + fs::relative(DestPath, _RootDir).generic_string();
			}
		}
		else
		{
			// TODO: support different formats
			assert(false && "Heightfield format not supported");
			return false;
		}

		// Generate material
		
		std::string MaterialID;

		const auto TexturePath = GetPath(Task.Params, "TextureOutput");

		auto EffectIt = _EffectsByType.find("MetallicRoughnessTerrain");
		if (EffectIt == _EffectsByType.cend() || EffectIt->second.empty())
		{
			Task.Log.LogError("Material type MetallicRoughnessTerrain has no mapped DEM effect file in effect settings");
			return false;
		}

		CMaterialParams MtlParamTable;
		auto Path = ResolvePathAliases(EffectIt->second).generic_string();
		Task.Log.LogDebug("Opening effect " + Path);
		if (!GetEffectMaterialParams(MtlParamTable, Path, Task.Log))
		{
			Task.Log.LogError("Error reading material param table for effect " + Path);
			return false;
		}

		Data::CParams MtlParams;

		const auto& GeomNormalTextureID = GetEffectParamID("TerrainGeometryNormalTexture");
		if (MtlParamTable.HasResource(GeomNormalTextureID))
		{
			//std::string FileName = TNPath.filename().generic_string();
			//ToLower(FileName);
			//auto DestPath = TexturePath / FileName;
			auto DestPath = TexturePath / (TaskName + "_tn" + TNPath.extension().string());
			fs::create_directories(DestPath.parent_path());
			std::error_code ec;
			if (!fs::copy_file(TNPath, DestPath, fs::copy_options::overwrite_existing, ec))
			{
				Task.Log.LogError("Error copying texture from " + TNPath.generic_string() + " to " + DestPath.generic_string() + ": " + ec.message());
				return false;
			}

			MtlParams.emplace_back(CStrID(GeomNormalTextureID), _ResourceRoot + fs::relative(DestPath, _RootDir).generic_string());
		}

		const auto& SplatMapTextureID = GetEffectParamID("SplatMapTexture");
		if (MtlParamTable.HasResource(SplatMapTextureID))
		{
			//std::string FileName = SplatMapPath.filename().generic_string();
			//ToLower(FileName);
			//auto DestPath = TexturePath / FileName;
			auto DestPath = TexturePath / (TaskName + "_sm" + SplatMapPath.extension().string());
			fs::create_directories(DestPath.parent_path());
			std::error_code ec;
			if (!fs::copy_file(SplatMapPath, DestPath, fs::copy_options::overwrite_existing, ec))
			{
				Task.Log.LogError("Error copying texture from " + SplatMapPath.generic_string() + " to " + DestPath.generic_string() + ": " + ec.message());
				return false;
			}

			MtlParams.emplace_back(CStrID(SplatMapTextureID), _ResourceRoot + fs::relative(DestPath, _RootDir).generic_string());
		}

		const auto& AlbedoTextureID = GetEffectParamID("AlbedoTexture");
		Data::CDataArray* pAlbedoTextures;
		if (ParamsUtils::TryGetParam(pAlbedoTextures, Task.Params, "AlbedoTexture"))
		{
			const auto Size = pAlbedoTextures->size();
			for (size_t i = 0; i < Size; ++i)
			{
				const std::string TexParamID = AlbedoTextureID + std::to_string(i);
				if (MtlParamTable.HasResource(TexParamID))
					MtlParams.emplace_back(CStrID(TexParamID), _ResourceRoot + pAlbedoTextures->at(i).GetValue<std::string>());
			}
		}

		// TODO: NormalTexture, MRTexture

		{
			auto DestPath = GetPath(Task.Params, "MaterialOutput") / (TaskName + ".mtl");

			fs::create_directories(DestPath.parent_path());

			std::ofstream File(DestPath, std::ios_base::binary | std::ios_base::trunc);

			if (!SaveMaterial(File, EffectIt->second, MtlParamTable, MtlParams, Task.Log)) return false;

			MaterialID = _ResourceRoot + fs::relative(DestPath, _RootDir).generic_string();
		}

		// Write scene file

		const auto SplatSizeX = ParamsUtils::GetParam<float>(Task.Params, "SplatSizeX", 1.f);
		const auto SplatSizeZ = ParamsUtils::GetParam<float>(Task.Params, "SplatSizeZ", 1.f);

		Data::CParams Result;

		Data::CDataArray Attributes;
		Data::CParams Attribute;
		Attribute.emplace_back(CStrID("Class"), 'TRNA'); //std::string("Frame::CTerrainAttribute"));
		if (!CDLODID.empty())
			Attribute.emplace_back(CStrID("CDLODFile"), CDLODID);
		if (!MaterialID.empty())
			Attribute.emplace_back(CStrID("Material"), MaterialID);
		if (SplatSizeX > 0.f && SplatSizeX != 1.f)
			Attribute.emplace_back(CStrID("SplatSizeX"), SplatSizeX);
		if (SplatSizeZ > 0.f && SplatSizeZ != 1.f)
			Attribute.emplace_back(CStrID("SplatSizeZ"), SplatSizeZ);
		Attributes.push_back(std::move(Attribute));
		Result.emplace_back(CStrID("Attrs"), std::move(Attributes));

		const fs::path OutPath = GetPath(Task.Params, "Output");

		if (_OutputHRD)
		{
			const auto DestPath = OutPath / (TaskName + ".hrd");
			if (!ParamsUtils::SaveParamsToHRD(DestPath.string().c_str(), Result))
			{
				Task.Log.LogError("Error serializing " + TaskName + " to text");
				return false;
			}
		}

		if (_OutputBin)
		{
			const auto DestPath = OutPath / (Task.TaskID.ToString() + ".scn");
			if (!ParamsUtils::SaveParamsByScheme(DestPath.string().c_str(), Result, CStrID("SceneNode"), _SceneSchemes))
			{
				Task.Log.LogError("Error serializing " + TaskName + " to binary");
				return false;
			}
		}

		return true;
	}
};

int main(int argc, const char** argv)
{
	CL3DTTool Tool("cf-l3dt", "L3DT (Large 3D Terrain) to DeusExMachina scene asset converter", { 1, 0, 0 });
	return Tool.Execute(argc, argv);
}
