#include <ContentForgeTool.h>
#include <Utils.h>
#include <ParamsUtils.h>
#include <Recast.h>
#include <DetourNavMesh.h> // For max vertices per polygon constant
#include <DetourNavMeshBuilder.h>
#include <acl/math/transform_32.h> // FIXME: use RTM only when it becomes available as a separate library

namespace fs = std::filesystem;

// Set working directory to $(TargetDir)
// Example args:
// -s src/game/levels --path Data ../../../content

// FIXME: to utils!
struct float3
{
	union
	{
		struct { float x, y, z; };
		float v[3];
	};
};

class CFRCContext : public rcContext
{
public:

	CThreadSafeLog& _Log;

	CFRCContext(CThreadSafeLog& Log) : _Log(Log) {}

	virtual void doLog(const rcLogCategory category, const char* msg, const int len) override
	{
		switch (category)
		{
			case RC_LOG_PROGRESS: _Log.LogInfo(std::string(msg, len)); break;
			case RC_LOG_WARNING:  _Log.LogWarning(std::string(msg, len)); break;
			case RC_LOG_ERROR:    _Log.LogError(std::string(msg, len)); break;
		}
	}
};

class CNavmeshTool : public CContentForgeTool
{
protected:

public:

	CNavmeshTool(const std::string& Name, const std::string& Desc, CVersion Version)
		: CContentForgeTool(Name, Desc, Version)
	{
	}

	virtual bool SupportsMultithreading() const override
	{
		// FIXME: must handle duplicate targets (for example 2 metafiles writing the same resulting file, like depth_atest_ps)
		// FIXME: CStrID
		return false;
	}

	virtual int Init() override
	{
		return 0;
	}

	virtual void ProcessCommandLine(CLI::App& CLIApp) override
	{
		CContentForgeTool::ProcessCommandLine(CLIApp);
	}

	virtual bool ProcessTask(CContentForgeTask& Task) override
	{
		// TODO: check whether the metafile can be processed by this tool
		if (ParamsUtils::GetParam(Task.Params, "Tools", std::string{}) != _Name)
		{
			// FIXME: skip sliently without error. To CF tool base class??? If no tools specified, try in each.
			return false;
		}

		const std::string TaskName = GetValidResourceName(Task.TaskID.ToString());

		// Read navmesh source HRD

		Data::CParams Desc;
		if (!ParamsUtils::LoadParamsFromHRD(Task.SrcFilePath.string().c_str(), Desc))
		{
			if (_LogVerbosity >= EVerbosity::Errors)
				Task.Log.LogError(Task.SrcFilePath.generic_string() + " HRD loading or parsing error");
			return false;
		}

		struct CConvexVolume
		{
			std::vector<float3> verts;
			float hmin, hmax;
			unsigned char areaId;
		};

		std::vector<float> verts;
		std::vector<int> tris;
		std::vector<CConvexVolume> vols;

		// Collect geometry

		// FIXME: rasterize trinagles per geometry, don't store all vertices in one array before processing?
		Data::CDataArray* pGeometryList;
		if (ParamsUtils::TryGetParam(pGeometryList, Desc, "Geometry"))
		{
			for (const auto& GeometryRecord : *pGeometryList)
			{
				const auto& GeometryDesc = GeometryRecord.GetValue<Data::CParams>();

				acl::Transform_32 WorldTfm = acl::transform_identity_32();

				const Data::CParams* pTfmParams;
				if (ParamsUtils::TryGetParam(pTfmParams, GeometryDesc, "Transform"))
				{
					vector4 Value;
					if (ParamsUtils::TryGetParam(Value, *pTfmParams, "S"))
						WorldTfm.scale = { Value.x, Value.y, Value.z, 0.f };
					if (ParamsUtils::TryGetParam(Value, *pTfmParams, "R"))
						WorldTfm.rotation = { Value.x, Value.y, Value.z, Value.w };
					if (ParamsUtils::TryGetParam(Value, *pTfmParams, "T"))
						WorldTfm.translation = { Value.x, Value.y, Value.z, 1.f };
				}

				if (ParamsUtils::HasParam(GeometryDesc, CStrID("Mesh")))
				{
					// add mesh geometry
				}
				else if (ParamsUtils::HasParam(GeometryDesc, CStrID("Terrain")))
				{
					if (!ProcessTerrainGeometry(GeometryDesc, WorldTfm, verts, tris, Task.Log))
						Task.Log.LogWarning("Couldn't export terrain geometry");
				}
				else if (ParamsUtils::HasParam(GeometryDesc, CStrID("Shape")))
				{
					// add shape geometry
				}
			}
		}

		const int nverts = verts.size() / 3; // 3 floats per vertex
		const int ntris = tris.size() / 3;   // 3 indices per triangle

		// TODO: can add optional bounds to config, to use only interactive level part for example
		float bmax[3] = {};
		float bmin[3] = {};
		if (nverts) rcCalcBounds(verts.data(), nverts, bmin, bmax);

		// NB: the code below is copied from RecastDemo (Sample_SoloMesh.cpp) with slight changes

		// Step 1. Initialize build config.

		CFRCContext ctx(Task.Log);

		const float agentHeight = ParamsUtils::GetParam(Desc, "AgentHeight", 1.8f);
		const float agentRadius = ParamsUtils::GetParam(Desc, "AgentRadius", 0.3f);
		const float agentMaxClimb = ParamsUtils::GetParam(Desc, "AgentMaxClimb", 0.2f);
		const float agentWalkableSlope = ParamsUtils::GetParam(Desc, "AgentWalkableSlope", 60.f);

		const float cellSize = ParamsUtils::GetParam(Desc, "CellSize", agentRadius / 3.f);
		const float cellHeight = ParamsUtils::GetParam(Desc, "CellHeight", cellSize);
		const float edgeMaxLen = ParamsUtils::GetParam(Desc, "EdgeMaxLength", 12.f);
		const float edgeMaxError = ParamsUtils::GetParam(Desc, "EdgeMaxError", 1.3f);
		const int regionMinSize = ParamsUtils::GetParam(Desc, "RegionMinSize", 8);
		const int regionMergeSize = ParamsUtils::GetParam(Desc, "RegionMergeSize", 8);
		const bool buildDetailMesh = ParamsUtils::GetParam(Desc, "BuildDetailMesh", true);
		const float detailSampleDist = ParamsUtils::GetParam(Desc, "DetailSampleDistance", 6.f);
		const float detailSampleMaxError = ParamsUtils::GetParam(Desc, "DetailSampleMaxError", 1.f);

		rcConfig cfg;
		memset(&cfg, 0, sizeof(cfg));
		cfg.cs = cellSize;
		cfg.ch = cellHeight;
		cfg.walkableSlopeAngle = agentWalkableSlope;
		cfg.walkableHeight = (int)ceilf(agentHeight / cfg.ch);
		cfg.walkableClimb = (int)floorf(agentMaxClimb / cfg.ch);
		cfg.walkableRadius = (int)ceilf(agentRadius / cfg.cs);
		cfg.maxEdgeLen = (int)(edgeMaxLen / cfg.cs);
		cfg.maxSimplificationError = edgeMaxError;
		cfg.minRegionArea = rcSqr(regionMinSize); // Note: area = size*size
		cfg.mergeRegionArea = rcSqr(regionMergeSize); // Note: area = size*size
		cfg.maxVertsPerPoly = DT_VERTS_PER_POLYGON;
		cfg.detailSampleDist = detailSampleDist < 0.9f ? 0.f : cfg.cs * detailSampleDist;
		cfg.detailSampleMaxError = cfg.ch * detailSampleMaxError;

		rcVcopy(cfg.bmin, bmin);
		rcVcopy(cfg.bmax, bmax);
		rcCalcGridSize(cfg.bmin, cfg.bmax, cfg.cs, &cfg.width, &cfg.height);

		// Step 2. Rasterize input polygon soup.

		auto rcHeightfieldDeleter = [](rcHeightfield* ptr) { rcFreeHeightField(ptr); };
		std::unique_ptr<rcHeightfield, decltype(rcHeightfieldDeleter)> solid(rcAllocHeightfield(), rcHeightfieldDeleter);
		if (!rcCreateHeightfield(&ctx, *solid, cfg.width, cfg.height, cfg.bmin, cfg.bmax, cfg.cs, cfg.ch))
		{
			Task.Log.LogError("buildNavigation: Could not create solid heightfield.");
			return false;
		}

		// TODO: can do per geometry object instead of collecting all geometry at once!
		// TODO: there is also rcClearUnwalkableTriangles, was used with non-RC_NULL_AREA in old CIDE
		std::unique_ptr<unsigned char[]> triareas(new unsigned char[ntris]);
		memset(triareas.get(), 0, ntris * sizeof(unsigned char));
		rcMarkWalkableTriangles(&ctx, cfg.walkableSlopeAngle, verts.data(), nverts, tris.data(), ntris, triareas.get());
		if (!rcRasterizeTriangles(&ctx, verts.data(), nverts, tris.data(), triareas.get(), ntris, *solid, cfg.walkableClimb))
		{
			Task.Log.LogError("buildNavigation: Could not rasterize triangles.");
			return false;
		}

		verts.clear();
		verts.shrink_to_fit();
		tris.clear();
		tris.shrink_to_fit();
		triareas.reset();

		// Step 3. Filter walkables surfaces.

		rcFilterLowHangingWalkableObstacles(&ctx, cfg.walkableClimb, *solid);
		rcFilterLedgeSpans(&ctx, cfg.walkableHeight, cfg.walkableClimb, *solid);
		rcFilterWalkableLowHeightSpans(&ctx, cfg.walkableHeight, *solid);

		// Step 4. Partition walkable surface to simple regions.

		auto rcCompactHeightfieldDeleter = [](rcCompactHeightfield* ptr) { rcFreeCompactHeightfield(ptr); };
		std::unique_ptr<rcCompactHeightfield, decltype(rcCompactHeightfieldDeleter)> chf(rcAllocCompactHeightfield(), rcCompactHeightfieldDeleter);
		if (!rcBuildCompactHeightfield(&ctx, cfg.walkableHeight, cfg.walkableClimb, *solid, *chf))
		{
			Task.Log.LogError("buildNavigation: Could not build compact data.");
			return false;
		}

		solid.reset();

		if (!rcErodeWalkableArea(&ctx, cfg.walkableRadius, *chf))
		{
			Task.Log.LogError("buildNavigation: Could not erode.");
			return false;
		}

		for (const auto& vol : vols)
			rcMarkConvexPolyArea(&ctx, vol.verts.data()->v, vol.verts.size(), vol.hmin, vol.hmax, vol.areaId, *chf);

		vols.clear();
		vols.shrink_to_fit();

		if (!rcBuildDistanceField(&ctx, *chf))
		{
			Task.Log.LogError("buildNavigation: Could not build distance field.");
			return false;
		}

		if (!rcBuildRegions(&ctx, *chf, 0, cfg.minRegionArea, cfg.mergeRegionArea))
		{
			Task.Log.LogError("buildNavigation: Could not build watershed regions.");
			return false;
		}

		// Step 5. Trace and simplify region contours.

		auto rcContourSetDeleter = [](rcContourSet* ptr) { rcFreeContourSet(ptr); };
		std::unique_ptr<rcContourSet, decltype(rcContourSetDeleter)> cset(rcAllocContourSet(), rcContourSetDeleter);
		if (!rcBuildContours(&ctx, *chf, cfg.maxSimplificationError, cfg.maxEdgeLen, *cset))
		{
			Task.Log.LogError("buildNavigation: Could not create contours.");
			return false;
		}

		// Step 6. Build polygons mesh from contours.

		auto rcPolyMeshDeleter = [](rcPolyMesh* ptr) { rcFreePolyMesh(ptr); };
		std::unique_ptr<rcPolyMesh, decltype(rcPolyMeshDeleter)> pmesh(rcAllocPolyMesh(), rcPolyMeshDeleter);
		if (!rcBuildPolyMesh(&ctx, *cset, cfg.maxVertsPerPoly, *pmesh))
		{
			Task.Log.LogError("buildNavigation: Could not triangulate contours.");
			return false;
		}

		cset.reset();

		// Step 7. Create detail mesh which allows to access approximate height on each polygon.

		auto rcPolyMeshDetailDeleter = [](rcPolyMeshDetail* ptr) { rcFreePolyMeshDetail(ptr); };
		std::unique_ptr<rcPolyMeshDetail, decltype(rcPolyMeshDetailDeleter)> dmesh(nullptr, rcPolyMeshDetailDeleter);
		if (buildDetailMesh)
		{
			dmesh.reset(rcAllocPolyMeshDetail());
			if (!rcBuildPolyMeshDetail(&ctx, *pmesh, *chf, cfg.detailSampleDist, cfg.detailSampleMaxError, *dmesh))
			{
				Task.Log.LogError("buildNavigation: Could not build detail mesh.");
				return false;
			}
		}

		chf.reset();

		// (Optional) Step 8. Create Detour data from Recast poly mesh.

		// Update poly flags from areas.
		for (int i = 0; i < pmesh->npolys; ++i)
		{
			/*
			if (pmesh->areas[i] == RC_WALKABLE_AREA)
				pmesh->areas[i] = SAMPLE_POLYAREA_GROUND;

			if (pmesh->areas[i] == SAMPLE_POLYAREA_GROUND ||
				pmesh->areas[i] == SAMPLE_POLYAREA_GRASS ||
				pmesh->areas[i] == SAMPLE_POLYAREA_ROAD)
			{
				pmesh->flags[i] = SAMPLE_POLYFLAGS_WALK;
			}
			else if (pmesh->areas[i] == SAMPLE_POLYAREA_WATER)
			{
				pmesh->flags[i] = SAMPLE_POLYFLAGS_SWIM;
			}
			else if (pmesh->areas[i] == SAMPLE_POLYAREA_DOOR)
			{
				pmesh->flags[i] = SAMPLE_POLYFLAGS_WALK | SAMPLE_POLYFLAGS_DOOR;
			}
			*/
		}

		// Read offmesh connections

		std::vector<float> offMeshConVerts;
		std::vector<float> offMeshConRads;
		std::vector<unsigned char> offMeshConDirs;
		std::vector<unsigned char> offMeshConAreas;
		std::vector<unsigned short> offMeshConFlags;
		std::vector<unsigned int> offMeshConId;

		Data::CDataArray* pOffmeshList;
		if (ParamsUtils::TryGetParam(pOffmeshList, Desc, "OffmeshConnections") && !pOffmeshList->empty())
		{
			offMeshConVerts.reserve(pOffmeshList->size() * 3 * 2);
			offMeshConRads.reserve(pOffmeshList->size());
			offMeshConDirs.reserve(pOffmeshList->size());
			offMeshConAreas.reserve(pOffmeshList->size());
			offMeshConFlags.reserve(pOffmeshList->size());
			offMeshConId.reserve(pOffmeshList->size());

			for (const auto& Record : *pOffmeshList)
			{
				const auto& OffmeshDesc = Record.GetValue<Data::CParams>();
				const auto Start = ParamsUtils::GetParam(OffmeshDesc, "Start", vector4{});
				const auto End = ParamsUtils::GetParam(OffmeshDesc, "End", vector4{});

				offMeshConVerts.push_back(Start.x);
				offMeshConVerts.push_back(Start.y);
				offMeshConVerts.push_back(Start.z);
				offMeshConVerts.push_back(End.x);
				offMeshConVerts.push_back(End.y);
				offMeshConVerts.push_back(End.z);
				offMeshConRads.push_back(ParamsUtils::GetParam(OffmeshDesc, "Radius", agentRadius));
				offMeshConDirs.push_back(ParamsUtils::GetParam(OffmeshDesc, "Bidirectional", true) ? 1 : 0);
				offMeshConAreas.push_back(ParamsUtils::GetParam<int>(OffmeshDesc, "AreaType", RC_WALKABLE_AREA));
				offMeshConFlags.push_back(ParamsUtils::GetParam(OffmeshDesc, "Flags", 0)); //???or calc based on area, or combine calc and explicit?
				offMeshConId.push_back(ParamsUtils::GetParam(OffmeshDesc, "UserID", 0));
			}
		}

		dtNavMeshCreateParams params;
		memset(&params, 0, sizeof(params));
		params.verts = pmesh->verts;
		params.vertCount = pmesh->nverts;
		params.polys = pmesh->polys;
		params.polyAreas = pmesh->areas;
		params.polyFlags = pmesh->flags;
		params.polyCount = pmesh->npolys;
		params.nvp = pmesh->nvp;
		if (dmesh)
		{
			params.detailMeshes = dmesh->meshes;
			params.detailVerts = dmesh->verts;
			params.detailVertsCount = dmesh->nverts;
			params.detailTris = dmesh->tris;
			params.detailTriCount = dmesh->ntris;
		}
		if (!offMeshConRads.empty())
		{
			params.offMeshConVerts = offMeshConVerts.data();
			params.offMeshConRad = offMeshConRads.data();
			params.offMeshConDir = offMeshConDirs.data();
			params.offMeshConAreas = offMeshConAreas.data();
			params.offMeshConFlags = offMeshConFlags.data();
			params.offMeshConUserID = offMeshConId.data();
			params.offMeshConCount = offMeshConRads.size();
		}
		params.walkableHeight = agentHeight;
		params.walkableRadius = agentRadius;
		params.walkableClimb = agentMaxClimb;
		rcVcopy(params.bmin, pmesh->bmin);
		rcVcopy(params.bmax, pmesh->bmax);
		params.cs = cfg.cs;
		params.ch = cfg.ch;
		params.buildBvTree = true;

		unsigned char* navData = nullptr;
		int navDataSize = 0;
		if (!dtCreateNavMeshData(&params, &navData, &navDataSize))
		{
			Task.Log.LogError("Could not build Detour navmesh.");
			return false;
		}

		pmesh.reset();
		dmesh.reset();

		// Write resulting NM file
		{
			auto DestPath = GetPath(Task.Params, "Output") / (TaskName + ".nm");
			fs::create_directories(DestPath.parent_path());

			std::ofstream File(DestPath, std::ios_base::binary | std::ios_base::trunc);
			if (!File)
			{
				Task.Log.LogError("Error opening an output file " + DestPath.generic_string());
				return false;
			}

			WriteStream<uint32_t>(File, 'NAVM');     // Format magic value
			WriteStream<uint32_t>(File, 0x00010000); // Version 0.1.0.0
			WriteStream(File, agentRadius);
			WriteStream(File, agentHeight);
			//???save other agent params too?

			WriteStream<uint32_t>(File, navDataSize);
			File.write(reinterpret_cast<const char*>(navData), navDataSize);

			// Named region count
			// Per named convex volume:
			//  Poly count
			//  Poly refs
		}

		dtFree(navData);
			
		return true;
	}

	bool ProcessTerrainGeometry(const Data::CParams& Desc, const acl::Transform_32& WorldTfm, std::vector<float>& OutVertices, std::vector<int>& OutIndices, CThreadSafeLog& Log)
	{
		const auto ResourceID = ParamsUtils::GetParam(Desc, "Terrain", std::string{});
		const auto Path = ResolvePathAliases(ResourceID);

		std::ifstream File(Path, std::ios_base::binary);
		if (!File)
		{
			Log.LogError("Can't open terrain file " + Path.generic_string());
			return false;
		}

#pragma pack(push, 1)
		struct CCDLODHeader
		{
			uint32_t Magic;
			uint32_t Version;
			uint32_t Width;
			uint32_t Height;
			uint32_t PatchSize;
			uint32_t LODCount;
			uint32_t TotalMinMaxDataCount;
			float VerticalScale;
			float MinX;
			float MaxX;
			float MinZ;
			float MaxZ;
			float MinY;
			float MaxY;
		};
#pragma pack(pop)

		CCDLODHeader Header;
		ReadStream(File, Header);

		if (Header.Magic != 'CDLD' || Header.Version != 0x00010000)
		{
			Log.LogError("Incorrect format or version: " + Path.generic_string());
			return false;
		}

		const auto SizeX = Header.MaxX - Header.MinX;
		const auto SizeZ = Header.MaxZ - Header.MinZ;
		const auto QuadCountX = Header.Width - 1;
		const auto QuadCountZ = Header.Height - 1;
		const auto QuadSizeX = SizeX / static_cast<float>(QuadCountX);
		const auto QuadSizeZ = SizeZ / static_cast<float>(QuadCountZ);
		const auto TriCount = QuadCountX * QuadCountZ * 2;

		{
			std::vector<uint16_t> HeightMap(Header.Width * Header.Height);
			File.read(reinterpret_cast<char*>(HeightMap.data()), HeightMap.size() * sizeof(uint16_t));

			const auto PrevVertexFloatCount = OutVertices.size();
			OutVertices.resize(PrevVertexFloatCount + 3 * HeightMap.size());

			// Fill vertices
			float* pCurrVtx = OutVertices.data() + PrevVertexFloatCount;
			for (uint32_t j = 0; j < Header.Height; ++j)
			{
				for (uint32_t i = 0; i < Header.Width; ++i)
				{
					const auto Vertex = acl::transform_position(WorldTfm,
						// Local vertex
						{
							Header.MinX + i * QuadSizeX,
							(static_cast<int>(HeightMap[j * Header.Width + i]) - 32767) * Header.VerticalScale,
							Header.MinZ + j * QuadSizeZ
						});
					*pCurrVtx++ = acl::vector_get_x(Vertex);
					*pCurrVtx++ = acl::vector_get_y(Vertex);
					*pCurrVtx++ = acl::vector_get_z(Vertex);
				}
			}
		}

		// Fill indices
		const auto PrevIndexCount = OutIndices.size();
		OutIndices.resize(PrevIndexCount + 3 * TriCount);
		int* pCurrIdx = OutIndices.data() + PrevIndexCount;
		for (uint32_t j = 0; j < QuadCountZ; ++j)
		{
			for (uint32_t i = 0; i < QuadCountX; ++i)
			{
				*pCurrIdx++ = j * Header.Width + i;
				*pCurrIdx++ = (j + 1) * Header.Width + i;
				*pCurrIdx++ = j * Header.Width + (i + 1);
				*pCurrIdx++ = j * Header.Width + (i + 1);
				*pCurrIdx++ = (j + 1) * Header.Width + i;
				*pCurrIdx++ = (j + 1) * Header.Width + (i + 1);
			}
		}

		return true;
	}
};

int main(int argc, const char** argv)
{
	CNavmeshTool Tool("cf-navmesh", "Navigation mesh generator for static level geometry", { 0, 1, 0 });
	return Tool.Execute(argc, argv);
}
