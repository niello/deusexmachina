#include <ContentForgeTool.h>
#include <SceneTools.h>
#include <Render/ShaderMetaCommon.h>
#include <BTFile.h>
#include <Utils.h>
#include <ParamsUtils.h>
#include <CLI11.hpp>
#include <pugixml.hpp>

namespace fs = std::filesystem;

// Set working directory to $(TargetDir)
// Example args:
// -s src/terrain -c --path Data ../../../content

class CL3DTTool : public CContentForgeTool
{
protected:

	Data::CSchemeSet _SceneSchemes;
	CSceneSettings   _Settings;

	std::string      _ResourceRoot;
	std::string      _SchemeFile;
	std::string      _SettingsFile;
	bool             _OutputBin = false;
	bool             _OutputHRD = false; // For debug purposes, saves scene hierarchies in a human-readable format
	bool             _NeedCollision = false;

public:

	CL3DTTool(const std::string& Name, const std::string& Desc, CVersion Version)
		: CContentForgeTool(Name, Desc, Version)
		, _ResourceRoot("Data:")
	{
		// Set default before parsing command line
		_SchemeFile = "../schemes/scene.dss";
		_SettingsFile = "../schemes/settings.hrd";

		InitImageProcessing();
	}

	~CL3DTTool()
	{
		TermImageProcessing();
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

		if (!LoadSceneSettings(_SettingsFile, _Settings)) return 3;

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
		CLIApp.add_flag("-c,--collision", _NeedCollision, "Add a collision attribute to the asset");
	}

	virtual ETaskResult ProcessTask(CContentForgeTask& Task) override
	{
		const std::string TaskName = GetValidResourceName(Task.TaskID.ToString());

		// Read project XML

		const fs::path SrcFolder = Task.SrcFilePath.parent_path();

		std::vector<char> SrcFileData;
		if (!ReadAllFile(Task.SrcFilePath.string().c_str(), SrcFileData))
		{
			Task.Log.LogError("Error reading shader source " + Task.SrcFilePath.generic_string());
			return ETaskResult::Failure;
		}

		pugi::xml_document XMLDoc;
		auto XMLResult = XMLDoc.load_buffer(SrcFileData.data(), SrcFileData.size());
		if (!XMLResult)
		{
			Task.Log.LogError(std::string("Error reading L3DT XML: ") + XMLResult.description());
			return ETaskResult::Failure;
		}

		auto XMLRoot = XMLDoc.first_child();
		auto XMLVersion = XMLRoot.find_child_by_attribute("int", "name", "FileVersion");
		if (!XMLVersion || XMLVersion.text().as_int() != 1)
			Task.Log.LogWarning(std::string("Possibly unsupported version of L3DT XML: ") + XMLVersion.text().as_string());

		uint32_t HeightmapWidth = 0, HeightmapHeight = 0;
		float MinY = 0.f, MaxY = 0.f, HorizScale = 1.f;
		if (auto XMLTerrain = XMLRoot.find_child_by_attribute("name", "MapInfo").find_child_by_attribute("name", "Terrain"))
		{
			HeightmapWidth = XMLTerrain.find_child_by_attribute("int", "name", "nx").text().as_int();
			HeightmapHeight = XMLTerrain.find_child_by_attribute("int", "name", "ny").text().as_int();
			MinY = XMLTerrain.find_child_by_attribute("float", "name", "MinAlt").text().as_float();
			MaxY = XMLTerrain.find_child_by_attribute("float", "name", "MaxAlt").text().as_float();
			HorizScale = XMLTerrain.find_child_by_attribute("float", "name", "HorizScale").text().as_float();
		}

		// Check all necessary maps exist in appropriate formats

		auto XMLMaps = XMLRoot.find_child_by_attribute("name", "Maps");
		if (!XMLMaps)
		{
			Task.Log.LogError(std::string("No maps declared in L3DT XML"));
			return ETaskResult::Failure;
		}

		fs::path HFPath;
		if (auto XMLHFMap = XMLMaps.find_child_by_attribute("name", "HF"))
		{
			HFPath = SrcFolder / XMLHFMap.find_child_by_attribute("name", "Filename").text().as_string();
			if (HFPath.extension() != ".bt")
			{
				Task.Log.LogError(std::string("HF map format must be BT"));
				return ETaskResult::Failure;
			}
			if (!fs::exists(HFPath))
			{
				Task.Log.LogError(HFPath.generic_string() + " doesn't exist");
				return ETaskResult::Failure;
			}
		}
		else
		{
			Task.Log.LogError(std::string("No HF map declared in L3DT XML"));
			return ETaskResult::Failure;
		}

		fs::path TNPath;
		auto TNImageID = 0;
		if (auto XMLTNMap = XMLMaps.find_child_by_attribute("name", "TN"))
		{
			TNPath = SrcFolder / XMLTNMap.find_child_by_attribute("name", "Filename").text().as_string();
			if (TNPath.extension() != ".dds")
			{
				Task.Log.LogError(std::string("TN map format must be DDS"));
				return ETaskResult::Failure;
			}
			if (!fs::exists(TNPath))
			{
				Task.Log.LogError(TNPath.generic_string() + " doesn't exist");
				return ETaskResult::Failure;
			}

			TNImageID = LoadILImage(TNPath, Task.Log);
			if (!TNImageID) return ETaskResult::Failure;
		}
		else
		{
			Task.Log.LogError(std::string("No TN map declared in L3DT XML"));
			return ETaskResult::Failure;
		}

		fs::path SplatMapPath;
		auto SMImageID = 0;
		if (auto XMLAlpha1Map = XMLMaps.find_child_by_attribute("name", "Alpha_1"))
		{
			SplatMapPath = SrcFolder / XMLAlpha1Map.find_child_by_attribute("name", "Filename").text().as_string();

			const auto Ext = SplatMapPath.extension();
			if (Ext != ".dds" && Ext != ".tga")
			{
				Task.Log.LogError(std::string("Alpha (splatting) map format must be DDS or TGA"));
				return ETaskResult::Failure;
			}
			if (!fs::exists(SplatMapPath))
			{
				Task.Log.LogError(SplatMapPath.generic_string() + " doesn't exist");
				return ETaskResult::Failure;
			}
			if (!strcmp(XMLAlpha1Map.find_child_by_attribute("name", "MapType").text().as_string(), "BYTE"))
				Task.Log.LogWarning("Alpha (splatting) map is one channel. Use L3DT Professional to generate one multichannel map. It is freeware now.");
			if (XMLMaps.find_child_by_attribute("name", "Alpha_2"))
				Task.Log.LogWarning("Multiple alpha (splatting) maps are declared. Use L3DT Professional to generate one multichannel map. It is freeware now.");

			SMImageID = LoadILImage(SplatMapPath, Task.Log);
			if (!SMImageID) return ETaskResult::Failure;
		}
		else
		{
			Task.Log.LogError(std::string("No Alpha_1 map declared in L3DT XML, use Operations->Alpha maps->Generate maps..."));
			return ETaskResult::Failure;
		}

		const auto TNDestFormat = GetTextureDestFormat(TNPath, Task.Params);
		const auto SMDestFormat = GetTextureDestFormat(SplatMapPath, Task.Params);

		// Read terrain subdivision params

		// Terrain is divided into quad clusters, each of them containing a quadtree for LOD control.
		// At the coarsest LOD the whole cluster is rendered with one grid mesh.
		// The more LODs, the better CDLOD performs but the more memory and quadtree traversal CPU is required.
		// Each cluster occupies a separate scene node and can be loaded on demand. View & light culling also use
		// an AABB of the cluster and not of the whole terrain asset.
		uint32_t ClusterSize = static_cast<uint32_t>(ParamsUtils::GetParam<int>(Task.Params, "ClusterSize", 0));

		// Depth of subdivision for each cluster. 0 and 1 mean no subdivision, i.e. regular grid of clusters. This is not recommeneded.
		uint32_t LODCount = static_cast<uint32_t>(ParamsUtils::GetParam<int>(Task.Params, "LODCount", 6));
		if (!LODCount) LODCount = 1;

		// Read source heightfield

		std::vector<char> HeightfieldFileData;
		if (!ReadAllFile(HFPath.string().c_str(), HeightfieldFileData))
		{
			Task.Log.LogError("HF map reading failed");
			return ETaskResult::Failure;
		}

		CBTFile BTFile(HeightfieldFileData.data());

		if (!HeightmapWidth) HeightmapWidth = BTFile.GetWidth();
		if (!HeightmapHeight) HeightmapHeight = BTFile.GetHeight();
		assert(HeightmapWidth == BTFile.GetWidth() && HeightmapHeight == BTFile.GetHeight());

		if (!HeightmapWidth || !HeightmapHeight)
		{
			Task.Log.LogError("Heightmap with any dimension of 1 will have zero area");
			return ETaskResult::Failure;
		}

		const auto RasterQuadsX = HeightmapWidth - 1;
		const auto RasterQuadsY = HeightmapHeight - 1;

		// Cluster size must be pow2 because it is subdivided into a quadtree with integral number of texels per node at each level
		if (!ClusterSize)
		{
			// Default or 0 means that clustering is disabled. Create a single cluster covering the whole heightmap.
			ClusterSize = NextPow2(std::max(RasterQuadsX, RasterQuadsY));
		}
		else
		{
			// Clamp unnecessarily big cluster sizes to match source data better. Otherwise empty
			// quadrants would exist in a quadtree and consume processing power for nothing.
			ClusterSize = NextPow2(std::min(ClusterSize, std::max(RasterQuadsX, RasterQuadsY)));
		}

		// Clamp LOD count so that smallest patch covers at least 1 raster quad
		{
			const uint32_t SmallestPatchesPerCluster = 1 << (LODCount - 1);
			if (SmallestPatchesPerCluster > ClusterSize) LODCount = Log2(ClusterSize);
		}

		// Raster quad count per smallest and finest quadtree node
		const auto PatchSize = ClusterSize >> (LODCount - 1);

		// Prepare common part of the material

		const auto TexturePath = GetPath(Task.Params, "TextureOutput");

		auto EffectIt = _Settings.EffectsByType.find("MetallicRoughnessTerrain");
		if (EffectIt == _Settings.EffectsByType.cend() || EffectIt->second.empty())
		{
			Task.Log.LogError("Material type MetallicRoughnessTerrain has no mapped DEM effect file in effect settings");
			return ETaskResult::Failure;
		}

		CMaterialParams MtlParamTable;
		auto Path = ResolvePathAliases(EffectIt->second, _PathAliases).generic_string();
		Task.Log.LogDebug("Opening effect " + Path);
		if (!GetEffectMaterialParams(MtlParamTable, Path, Task.Log))
		{
			Task.Log.LogError("Error reading material param table for effect " + Path);
			return ETaskResult::Failure;
		}

		// Process clusters

		const auto ClustersX = DivCeil(HeightmapWidth, ClusterSize);
		const auto ClustersY = DivCeil(HeightmapHeight, ClusterSize);

		Task.Log.LogInfo("Cluster count: " + std::to_string(ClustersX) + "x" + std::to_string(ClustersY));
		Task.Log.LogInfo("Cluster size:  " + std::to_string(ClusterSize) + " unit quads");
		Task.Log.LogInfo("LOD count:     " + std::to_string(LODCount));
		Task.Log.LogInfo("Patch size:    " + std::to_string(PatchSize) + " unit quads");

		for (uint32_t ClusterY = 0; ClusterY < ClustersY; ++ClusterY)
		{
			for (uint32_t ClusterX = 0; ClusterX < ClustersX; ++ClusterX)
			{
				Task.Log.LogInfo("Processing cluster " + std::to_string(ClusterX) + "," + std::to_string(ClusterY) + "...");

				// ClusterSize is in raster quads, but heightmap stores vertices between these quads
				const auto HeightmapClusterSize = ClusterSize + 1;

				const auto HeightFromX = ClusterX * ClusterSize;
				const auto HeightFromY = ClusterY * ClusterSize;
				const auto HeightToX = std::min(HeightmapWidth, HeightFromX + HeightmapClusterSize);
				const auto HeightToY = std::min(HeightmapHeight, HeightFromY + HeightmapClusterSize);

				// Cluster heightmap data size in texels (vertices) and in raster quads
				const auto ClusterDataWidth = HeightToX - HeightFromX;
				const auto ClusterDataHeight = HeightToY - HeightFromY;
				const auto ClusterDataRasterWidth = ClusterDataWidth - 1;
				const auto ClusterDataRasterHeight = ClusterDataHeight - 1;

				const uint32_t PatchesX = DivCeil(ClusterDataRasterWidth, PatchSize);
				const uint32_t PatchesY = DivCeil(ClusterDataRasterHeight, PatchSize);

				Task.Log.LogInfo(" Heights: [" + std::to_string(HeightFromX) + "," + std::to_string(HeightFromY) + " - " + std::to_string(HeightToX - 1) + "," + std::to_string(HeightToY - 1) + "]");
				Task.Log.LogInfo(" Size:    " + std::to_string(ClusterDataRasterWidth) + "x" + std::to_string(ClusterDataRasterHeight) + " unit quads");
				Task.Log.LogInfo(" Patches: " + std::to_string(PatchesX) + "x" + std::to_string(PatchesY));

				// Prepare heightmap data for this cluster.
				// S->N col-major to N->S row-major, convert to ushort for D3DFMT_L16 texture format.
				std::vector<uint16_t> ClusterHeightMap(ClusterDataWidth * ClusterDataHeight);
				{
					// TODO: support float BT!
					// TODO: there are different possible formats: D3DFMT_R16F, D3DFMT_R32F, D3DFMT_L16
					//???TODO: unified normals+heightmap? 4-channel, single vertex texture fetch operation.
					//Physics will require separate data, but physics uses CPU RAM, and rendering uses VRAM.
					//Could create unified texture on loading, combining 3-channel normal map and HF.

					uint16_t* pDest = ClusterHeightMap.data();
					for (uint32_t y = HeightFromY; y < HeightToY; ++y)
						for (uint32_t x = HeightFromX; x < HeightToX; ++x)
							*pDest++ = static_cast<uint16_t>(static_cast<int>(BTFile.GetHeightsS()[x * HeightmapHeight + HeightmapHeight - 1 - y]) + 32768);
				}

				// Calculate min and max height for each patch in the cluster and build a hierarchy similar to mipmap chain
				std::vector<std::pair<uint16_t, uint16_t>> MinMaxData(CalcSizeWithMips(PatchesX, PatchesY, LODCount, false));

				// Generate finest level minmax map, one record per LOD 0 patch
				for (uint32_t PatchY = 0; PatchY < PatchesY; ++PatchY)
				{
					const auto PatchHeightStartY = PatchY * PatchSize;
					const auto PatchHeightEndY = std::min((PatchY + 1) * PatchSize + 1, ClusterDataHeight);

					for (uint32_t PatchX = 0; PatchX < PatchesX; ++PatchX)
					{
						const auto PatchHeightStartX = PatchX * PatchSize;
						const auto PatchHeightEndX = std::min((PatchX + 1) * PatchSize + 1, ClusterDataWidth);

						// Find min & max heights in the current patch
						auto MinHeight = CBTFile::NoDataUS;
						auto MaxHeight = CBTFile::NoDataUS;
						for (auto y = PatchHeightStartY; y < PatchHeightEndY; ++y)
						{
							for (auto x = PatchHeightStartX; x < PatchHeightEndX; ++x)
							{
								// Skip vertices without data
								const auto CurrHeight = ClusterHeightMap[y * ClusterDataWidth + x];
								if (CurrHeight == CBTFile::NoDataUS) continue;

								if (MinHeight == CBTFile::NoDataUS)
								{
									// First vertex with data found
									MinHeight = CurrHeight;
									MaxHeight = CurrHeight;
								}
								else
								{
									if (CurrHeight < MinHeight) MinHeight = CurrHeight;
									else if (CurrHeight > MaxHeight) MaxHeight = CurrHeight;
								}
							}
						}

						MinMaxData[PatchY * PatchesX + PatchX] = { MinHeight, MaxHeight };
					}
				}

				// Generate minmax map hierarchy similar to how mipmaps are generated.
				// Algorithm from the original CDLOD code modified for unsigned values.
				size_t PrevDataOffset = 0;
				size_t CurrDataOffset = PatchesX * PatchesY;
				auto PrevPatchesW = PatchesX;
				auto PrevPatchesH = PatchesY;

				std::fill(MinMaxData.begin() + CurrDataOffset, MinMaxData.end(), std::make_pair<uint16_t, uint16_t>(65535, 0));

				for (uint32_t LOD = 1; LOD < LODCount; ++LOD)
				{
					auto CurrPatchesW = DivCeil(PrevPatchesW, 2);
					auto CurrPatchesH = DivCeil(PrevPatchesH, 2);
					auto CurrDataSize = CurrPatchesW * CurrPatchesH;

					auto* pSrc = MinMaxData.data() + PrevDataOffset;
					auto* pDst = MinMaxData.data() + CurrDataOffset;

					// Consolidate quad of 4 patches from the previous level into 1 patch of the current level.
					// It also handles cases on edges when only 1 or 2 patches exist due to odd dimension size.
					for (uint32_t y = 0; y < PrevPatchesH; ++y)
					{
						for (uint32_t x = 0; x < PrevPatchesW; ++x)
						{
							auto& Dest = pDst[x / 2];
							Dest.first = std::min(Dest.first, pSrc[x].first);
							Dest.second = std::max(Dest.second, pSrc[x].second);
						}
						pSrc += PrevPatchesW;
						if (y % 2 == 1) pDst += CurrPatchesW;
					}

					PrevDataOffset = CurrDataOffset;
					CurrDataOffset += CurrDataSize;
					PrevPatchesW = CurrPatchesW;
					PrevPatchesH = CurrPatchesH;
				}

				// Ensure that the minmax map is folded up to 1 sample with global min & max values of the whole cluster
				assert(PrevPatchesW == 1 && PrevPatchesH == 1);

				// Use postfix in all file names when split into multiple clusters
				std::string Postfix;
				if (ClustersX > 1 || ClustersY > 1)
					Postfix = "_" + std::to_string(ClusterX) + "_" + std::to_string(ClusterY);

				// Write resulting CDLOD file
				std::string CDLODID;
				{
					auto DestPath = GetPath(Task.Params, "CDLODOutput") / (TaskName + Postfix + ".cdlod");
					fs::create_directories(DestPath.parent_path());

					std::ofstream File(DestPath, std::ios_base::binary | std::ios_base::trunc);
					if (!File)
					{
						Task.Log.LogError("Error opening an output file " + DestPath.generic_string());
						return ETaskResult::Failure;
					}

					//!!!FIXME: REVISIT WHAT PARAMS ARE NEEDED TO BE SAVED!
					//???aabb? patch size?

					WriteStream<uint32_t>(File, 'CDLD');        // Format magic value
					WriteStream<uint32_t>(File, 0x00010000);    // Version 0.1.0.0
					WriteStream(File, ClusterDataWidth);
					WriteStream(File, ClusterDataHeight);
					WriteStream(File, PatchSize);
					WriteStream(File, LODCount);
					WriteStream(File, 2 * static_cast<uint32_t>(MinMaxData.size()));
					WriteStream(File, BTFile.GetVerticalScale());
					WriteStream(File, static_cast<float>(BTFile.GetLeftExtent())); // Min X
					WriteStream(File, static_cast<float>(BTFile.GetRightExtent())); // Max X
					WriteStream(File, static_cast<float>(BTFile.GetBottomExtent())); // Min Z
					WriteStream(File, static_cast<float>(BTFile.GetTopExtent())); // Max Z
					WriteStream(File, BTFile.GetMinHeight()); // Min Y
					WriteStream(File, BTFile.GetMaxHeight()); // Max Y

					// Write heightmap
					File.write(reinterpret_cast<const char*>(ClusterHeightMap.data()), ClusterHeightMap.size() * sizeof(uint16_t));

					// Write minmax maps converted back to signed int16_t
					for (const auto& MinMax : MinMaxData)
					{
						WriteStream(File, static_cast<int16_t>(static_cast<int>(MinMax.first) - 32768));
						WriteStream(File, static_cast<int16_t>(static_cast<int>(MinMax.second) - 32768));
					}

					CDLODID = _ResourceRoot + fs::relative(DestPath, _RootDir).generic_string();

					Task.Log.LogInfo(" Written CDLOD: " + DestPath.generic_string() + " (" + std::to_string(File.tellp()) + " bytes)");
				}

				// Generate material

				Data::CParams MtlParams;

				const auto GeomNormalTextureID = _Settings.GetEffectParamID("TerrainGeometryNormalTexture");
				if (TNImageID && MtlParamTable.HasResource(GeomNormalTextureID))
				{
					// Terrain geometry normals are per-vertex, like heights. Must have exactly one normal per height point.
					const auto ImageRect = GetILImageRect(TNImageID);
					if (ImageRect.Width() != HeightmapWidth || ImageRect.Height() != HeightmapHeight)
					{
						Task.Log.LogError("Geometry normal texture size doesn't match heightmap dimensions");
						return ETaskResult::Failure;
					}

					CRect ClusterRect(HeightFromX, HeightFromY, HeightToX - 1, HeightToY - 1);
					auto DestPath = TexturePath / (TaskName + Postfix + "_tn" + TNPath.extension().string());
					if (!SaveILImageRegion(TNImageID, TNDestFormat, DestPath, ClusterRect, Task.Log))
					{
						Task.Log.LogError("Could not save geometry normals texture region for the cluster");
						return ETaskResult::Failure;
					}

					MtlParams.emplace_back(CStrID(GeomNormalTextureID), _ResourceRoot + fs::relative(DestPath, _RootDir).generic_string());
				}

				float SMUVScaleX = 1.f;
				float SMUVScaleY = 1.f;
				float SMUVOffsetX = 0.f;
				float SMUVOffsetY = 0.f;
				const auto SplatMapTextureID = _Settings.GetEffectParamID("SplatMapTexture");
				if (SMImageID && MtlParamTable.HasResource(SplatMapTextureID))
				{
					const auto ImageRect = GetILImageRect(SMImageID);

					// Splat map texels per raster quad. This can be used to remap UV from HM to SM.
					const float RatioX = ImageRect.Width() / static_cast<float>(RasterQuadsX);
					const float RatioY = ImageRect.Height() / static_cast<float>(RasterQuadsY);

					// Project heightmap region onto the splat map. This is an effective
					// rect on the source texture that covers the current cluster.
					// NB: values can be fractional if the texture is not perfectly mapped.
					float SMLeft = HeightFromX * RatioX;
					float SMTop = HeightFromY * RatioY;
					const float SMWidth = ClusterDataRasterWidth * RatioX;
					const float SMHeight = ClusterDataRasterHeight * RatioY;

					// For seamless cluster rendering with linear texture filtration
					constexpr int32_t MipLevel = 0;
					const int32_t BorderSize = 1 << MipLevel;

					// Round outside to whole pixels
					CRect ClusterRect(
						static_cast<int32_t>(SMLeft) - BorderSize,
						static_cast<int32_t>(SMTop) - BorderSize,
						static_cast<int32_t>(SMLeft + SMWidth + 0.5f) - 1 + BorderSize,
						static_cast<int32_t>(SMTop + SMHeight + 0.5f) - 1 + BorderSize);

					// Save the region
					auto DestPath = TexturePath / (TaskName + Postfix + "_sm" + SplatMapPath.extension().string());
					if (!SaveILImageRegion(SMImageID, SMDestFormat, DestPath, ClusterRect, Task.Log))
					{
						Task.Log.LogError("Could not save splat map texture region for the cluster");
						return ETaskResult::Failure;
					}

					// Now ClusterRect contains an actually saved region. Let's offset coverage rect
					// from the source texture to the saved texture. It involves only shifting the rect.
					SMLeft -= static_cast<float>(ClusterRect.Left);
					SMTop -= static_cast<float>(ClusterRect.Top);

					// Finally calculate UV coefficients for actual cluater data in an inflated texture
					SMUVScaleX = SMWidth / static_cast<float>(ClusterRect.Width());
					SMUVScaleY = SMHeight / static_cast<float>(ClusterRect.Height());
					SMUVOffsetX = SMLeft / static_cast<float>(ClusterRect.Width());
					SMUVOffsetY = SMTop / static_cast<float>(ClusterRect.Height());

					MtlParams.emplace_back(CStrID(SplatMapTextureID), _ResourceRoot + fs::relative(DestPath, _RootDir).generic_string());
				}

				//???can be different for different clusters?
				const auto AlbedoTextureID = _Settings.GetEffectParamID("AlbedoTexture");
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

				// TODO: other per-splat textures - normal, metallic-roughness

				std::string MaterialID;
				{
					auto DestPath = GetPath(Task.Params, "MaterialOutput") / (TaskName + ".mtl");

					fs::create_directories(DestPath.parent_path());

					std::ofstream File(DestPath, std::ios_base::binary | std::ios_base::trunc);

					if (!SaveMaterial(File, EffectIt->second, MtlParamTable, MtlParams, Task.Log)) return ETaskResult::Failure;

					MaterialID = _ResourceRoot + fs::relative(DestPath, _RootDir).generic_string();
				}
			}
		}

		UnloadILImage(TNImageID);

		// Write scene file

		const auto SplatSizeX = ParamsUtils::GetParam<float>(Task.Params, "SplatSizeX", 1.f);
		const auto SplatSizeZ = ParamsUtils::GetParam<float>(Task.Params, "SplatSizeZ", 1.f);

		Data::CParams Result;

		Data::CDataArray Attributes;

		//!!!DBG TMP!
		std::string CDLODID;
		std::string MaterialID;

		{
			Data::CParams Attribute;
			Attribute.emplace_back(CStrID("Class"), 'TRNA'); // Frame::CTerrainAttribute
			if (!CDLODID.empty())
				Attribute.emplace_back(CStrID("CDLODFile"), CDLODID);
			if (!MaterialID.empty())
				Attribute.emplace_back(CStrID("Material"), MaterialID);
			if (SplatSizeX > 0.f && SplatSizeX != 1.f)
				Attribute.emplace_back(CStrID("SplatSizeX"), SplatSizeX);
			if (SplatSizeZ > 0.f && SplatSizeZ != 1.f)
				Attribute.emplace_back(CStrID("SplatSizeZ"), SplatSizeZ);
			Attributes.push_back(std::move(Attribute));
		}

		if (_NeedCollision && !CDLODID.empty())
		{
			Data::CParams Attribute;
			Attribute.emplace_back(CStrID("Class"), 'COLA'); // Physics::CCollisionAttribute
			Attribute.emplace_back(CStrID("Shape"), CDLODID + "#Collision");
			Attribute.emplace_back(CStrID("Static"), true);
			Attributes.push_back(std::move(Attribute));
		}

		Result.emplace_back(CStrID("Attrs"), std::move(Attributes));

		const fs::path OutPath = GetPath(Task.Params, "Output");

		if (_OutputHRD)
		{
			const auto DestPath = OutPath / (TaskName + ".hrd");
			if (!ParamsUtils::SaveParamsToHRD(DestPath.string().c_str(), Result))
			{
				Task.Log.LogError("Error serializing " + TaskName + " to text");
				return ETaskResult::Failure;
			}
		}

		if (_OutputBin)
		{
			const auto DestPath = OutPath / (TaskName + ".scn");
			if (!ParamsUtils::SaveParamsByScheme(DestPath.string().c_str(), Result, CStrID("SceneNode"), _SceneSchemes))
			{
				Task.Log.LogError("Error serializing " + TaskName + " to binary");
				return ETaskResult::Failure;
			}
		}

		return ETaskResult::Success;
	}
};

int main(int argc, const char** argv)
{
	CL3DTTool Tool("cf-l3dt", "L3DT (Large 3D Terrain) to DeusExMachina scene asset converter", { 0, 2, 0 });
	return Tool.Execute(argc, argv);
}
