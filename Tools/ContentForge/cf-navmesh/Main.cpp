#include <ContentForgeTool.h>
#include <Utils.h>
#include <ParamsUtils.h>
#include <Render/RenderEnums.h>
#include <Recast.h>
#include <DetourCommon.h>
#include <DetourNavMesh.h>
#include <DetourNavMeshQuery.h>
#include <DetourNavMeshBuilder.h>
#include <rtm/qvvf.h>
#include <set>

namespace fs = std::filesystem;

// Set working directory to $(TargetDir)
// Example args:
// -s src/game/levels --path Data ../../../content

std::vector<float> OffsetPoly(const float* pSrcVerts, int SrcCount, float Offset);
bool GetPointInPoly(const std::vector<float>& In, float Out[3]);

struct CRegion
{
	std::vector<float> verts;
	float hmin;
	float hmax;
	std::set<unsigned int> offmeshIds;
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

	virtual ETaskResult ProcessTask(CContentForgeTask& Task) override
	{
		const std::string TaskName = GetValidResourceName(Task.TaskID.ToString());

		// Read navmesh source HRD

		Data::CParams Desc;
		if (!ParamsUtils::LoadParamsFromHRD(Task.SrcFilePath.string().c_str(), Desc))
		{
			if (_LogVerbosity >= EVerbosity::Errors)
				Task.Log.LogError(Task.SrcFilePath.generic_string() + " HRD loading or parsing error");
			return ETaskResult::Failure;
		}

		// Collect geometry

		std::vector<float> verts;
		std::vector<int> tris;

		// FIXME: rasterize trinagles per geometry, don't store all vertices in one array before processing?
		// rcCalcBounds requires that now, otherwise how do we know navmesh bounds?
		const Data::CDataArray* pGeometryList;
		if (ParamsUtils::TryGetParam(pGeometryList, Desc, "Geometry"))
		{
			for (const auto& GeometryRecord : *pGeometryList)
			{
				const auto& GeometryDesc = GeometryRecord.GetValue<Data::CParams>();

				rtm::qvvf WorldTfm = rtm::qvv_identity();

				const Data::CParams* pTfmParams;
				if (ParamsUtils::TryGetParam(pTfmParams, GeometryDesc, "Transform"))
				{
					float3 Vec3Value;
					float4 Vec4Value;
					if (ParamsUtils::TryGetParam(Vec3Value, *pTfmParams, "S"))
						WorldTfm.scale = { Vec3Value.x, Vec3Value.y, Vec3Value.z, 0.f };
					if (ParamsUtils::TryGetParam(Vec4Value, *pTfmParams, "R"))
						WorldTfm.rotation = { Vec4Value.x, Vec4Value.y, Vec4Value.z, Vec4Value.w };
					if (ParamsUtils::TryGetParam(Vec3Value, *pTfmParams, "T"))
						WorldTfm.translation = { Vec3Value.x, Vec3Value.y, Vec3Value.z, 1.f };
				}

				if (ParamsUtils::HasParam(GeometryDesc, CStrID("Mesh")))
				{
					if (!ProcessMeshGeometry(GeometryDesc, WorldTfm, verts, tris, Task.Log))
						Task.Log.LogWarning("Couldn't export mesh geometry");
				}
				else if (ParamsUtils::HasParam(GeometryDesc, CStrID("Terrain")))
				{
					if (!ProcessTerrainGeometry(GeometryDesc, WorldTfm, verts, tris, Task.Log))
						Task.Log.LogWarning("Couldn't export terrain geometry");
				}
				else if (ParamsUtils::HasParam(GeometryDesc, CStrID("Shape")))
				{
					if (!ProcessShapeGeometry(GeometryDesc, WorldTfm, verts, tris, Task.Log))
						Task.Log.LogWarning("Couldn't export shape geometry");
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
			return ETaskResult::Failure;
		}

		// TODO: can do per geometry object instead of collecting all geometry at once!
		// TODO: there is also rcClearUnwalkableTriangles, was used with non-RC_NULL_AREA in old CIDE:
		/* Area is associated with current geometry chunk
		if (Area == RC_NULL_AREA)
			rcMarkWalkableTriangles(&Ctx, Cfg.walkableSlopeAngle, pVerts, VertexCount, pTris, TriCount, pAreas);
		else
			rcClearUnwalkableTriangles(&Ctx, Cfg.walkableSlopeAngle, pVerts, VertexCount, pTris, TriCount, pAreas);
		*/
		std::unique_ptr<unsigned char[]> triareas(new unsigned char[ntris]);
		memset(triareas.get(), RC_NULL_AREA, ntris * sizeof(unsigned char));
		rcMarkWalkableTriangles(&ctx, cfg.walkableSlopeAngle, verts.data(), nverts, tris.data(), ntris, triareas.get());
		if (!rcRasterizeTriangles(&ctx, verts.data(), nverts, tris.data(), triareas.get(), ntris, *solid, cfg.walkableClimb))
		{
			Task.Log.LogError("buildNavigation: Could not rasterize triangles.");
			return ETaskResult::Failure;
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
			return ETaskResult::Failure;
		}

		solid.reset();

		if (!rcErodeWalkableArea(&ctx, cfg.walkableRadius, *chf))
		{
			Task.Log.LogError("buildNavigation: Could not erode.");
			return ETaskResult::Failure;
		}

		std::map<std::string, CRegion> NamedRegions;

		const Data::CDataArray* pRegionList;
		if (ParamsUtils::TryGetParam(pRegionList, Desc, "Regions") && !pRegionList->empty())
		{
			std::vector<float> vertsR;
			for (const auto& Record : *pRegionList)
			{
				const auto& RegionDesc = Record.GetValue<Data::CParams>();

				vertsR.clear();

				const auto hmin = ParamsUtils::GetParam(RegionDesc, "HeightStart", bmin[1]);
				const auto hmax = ParamsUtils::GetParam(RegionDesc, "HeightEnd", bmax[1]);

				// If vertices defined, mark the region with area
				const Data::CDataArray* pVertexList;
				if (ParamsUtils::TryGetParam(pVertexList, RegionDesc, "Vertices") && !pVertexList->empty())
				{
					vertsR.reserve(3 * pVertexList->size());
					for (const auto& VertexData : *pVertexList)
					{
						const auto& Vertex = VertexData.GetValue<float3>();
						vertsR.push_back(Vertex.x);
						vertsR.push_back(Vertex.y);
						vertsR.push_back(Vertex.z);
					}

					// NB: there are also rcMarkBoxArea, rcMarkCylinderArea
					int areaId = RC_WALKABLE_AREA;
					if (ParamsUtils::TryGetParam<int>(areaId, RegionDesc, "AreaType"))
						rcMarkConvexPolyArea(&ctx, vertsR.data(), pVertexList->size(), hmin, hmax, static_cast<uint8_t>(areaId), *chf);
				}

				// If ID is defined, remember this region as named for runtime access
				auto ID = ParamsUtils::GetParam(RegionDesc, "ID", std::string{});
				if (!ID.empty())
				{
					CRegion NewRegion{ vertsR, hmin, hmax };

					const Data::CDataArray* pOffmeshCons;
					if (ParamsUtils::TryGetParam(pOffmeshCons, RegionDesc, "OffmeshIds") && !pOffmeshCons->empty())
					{
						for (const auto& OffmeshIDData : *pOffmeshCons)
							NewRegion.offmeshIds.emplace(static_cast<unsigned int>(OffmeshIDData.GetValue<int>()));
					}

					NamedRegions.emplace(std::move(ID), std::move(NewRegion));
				}
			}
		}

		if (!rcBuildDistanceField(&ctx, *chf))
		{
			Task.Log.LogError("buildNavigation: Could not build distance field.");
			return ETaskResult::Failure;
		}

		if (!rcBuildRegions(&ctx, *chf, 0, cfg.minRegionArea, cfg.mergeRegionArea))
		{
			Task.Log.LogError("buildNavigation: Could not build watershed regions.");
			return ETaskResult::Failure;
		}

		// Step 5. Trace and simplify region contours.

		auto rcContourSetDeleter = [](rcContourSet* ptr) { rcFreeContourSet(ptr); };
		std::unique_ptr<rcContourSet, decltype(rcContourSetDeleter)> cset(rcAllocContourSet(), rcContourSetDeleter);
		if (!rcBuildContours(&ctx, *chf, cfg.maxSimplificationError, cfg.maxEdgeLen, *cset))
		{
			Task.Log.LogError("buildNavigation: Could not create contours.");
			return ETaskResult::Failure;
		}

		// Step 6. Build polygons mesh from contours.

		auto rcPolyMeshDeleter = [](rcPolyMesh* ptr) { rcFreePolyMesh(ptr); };
		std::unique_ptr<rcPolyMesh, decltype(rcPolyMeshDeleter)> pmesh(rcAllocPolyMesh(), rcPolyMeshDeleter);
		if (!rcBuildPolyMesh(&ctx, *cset, cfg.maxVertsPerPoly, *pmesh))
		{
			Task.Log.LogError("buildNavigation: Could not triangulate contours.");
			return ETaskResult::Failure;
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
				return ETaskResult::Failure;
			}
		}

		chf.reset();

		// (Optional) Step 8. Create Detour data from Recast poly mesh.

		constexpr unsigned char POLY_FLAGS_DEFAULT_ENABLED = 1;

		// Update poly flags from areas.
		for (int i = 0; i < pmesh->npolys; ++i)
		{
			//!!!DBG TMP! Zero flags pass no filter in detour, so fill with any default for now.
			//if (pmesh->areas[i] == RC_WALKABLE_AREA)
				pmesh->flags[i] = POLY_FLAGS_DEFAULT_ENABLED;

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

		const Data::CDataArray* pOffmeshList;
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
				const auto Start = ParamsUtils::GetParam(OffmeshDesc, "Start", float3{});
				const auto End = ParamsUtils::GetParam(OffmeshDesc, "End", float3{});

				offMeshConVerts.push_back(Start.x);
				offMeshConVerts.push_back(Start.y);
				offMeshConVerts.push_back(Start.z);
				offMeshConVerts.push_back(End.x);
				offMeshConVerts.push_back(End.y);
				offMeshConVerts.push_back(End.z);
				offMeshConRads.push_back(ParamsUtils::GetParam(OffmeshDesc, "Radius", agentRadius));
				offMeshConDirs.push_back(ParamsUtils::GetParam(OffmeshDesc, "Bidirectional", true) ? 1 : 0);
				offMeshConAreas.push_back(ParamsUtils::GetParam<int>(OffmeshDesc, "AreaType", RC_WALKABLE_AREA));
				offMeshConId.push_back(ParamsUtils::GetParam(OffmeshDesc, "UserID", 0));

				//???or calc based on area, or combine calc and explicit?
				offMeshConFlags.push_back(ParamsUtils::GetParam<int>(OffmeshDesc, "Flags", POLY_FLAGS_DEFAULT_ENABLED));
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
			return ETaskResult::Failure;
		}

		pmesh.reset();
		dmesh.reset();

		// Process named regions

		std::vector<std::pair<std::string, std::vector<dtPolyRef>>> NamedRegionPolys;

		if (!NamedRegions.empty())
		{
			auto dtNavMeshDeleter = [](dtNavMesh* ptr) { dtFreeNavMesh(ptr); };
			std::unique_ptr<dtNavMesh, decltype(dtNavMeshDeleter)> NavMesh(dtAllocNavMesh(), dtNavMeshDeleter);

			// Zero flags mean that NavMesh doesn't own navData
			if (dtStatusFailed(NavMesh->init(navData, navDataSize, 0)))
			{
				Task.Log.LogError("Could not load Detour navmesh for named region processing");
				return ETaskResult::Failure;
			}

			dtNavMeshQuery Query;
			if (dtStatusFailed(Query.init(NavMesh.get(), 512)))
			{
				Task.Log.LogError("Could not initialize navmesh query for named region processing");
				return ETaskResult::Failure;
			}

			const dtQueryFilter NavFilter;

			NamedRegionPolys.reserve(NamedRegions.size());

			for (auto& [ID, Region] : NamedRegions)
			{
				std::vector<dtPolyRef> Polys;
				if (!FillRegionPolyRefs(Region, cellSize, Query, Polys, Task.Log))
				{
					Task.Log.LogError("Failed to process named region polys for " + ID);
					return ETaskResult::Failure;
				}

				if (!Polys.empty())
					NamedRegionPolys.emplace_back(ID, std::move(Polys));
			}
		}

		// Write resulting NM file
		{
			auto DestPath = GetOutputPath(Task.Params) / (TaskName + ".nm");
			fs::create_directories(DestPath.parent_path());

			std::ofstream File(DestPath, std::ios_base::binary | std::ios_base::trunc);
			if (!File)
			{
				Task.Log.LogError("Error opening an output file " + DestPath.generic_string());
				return ETaskResult::Failure;
			}

			WriteStream<uint32_t>(File, 'NAVM');     // Format magic value
			WriteStream<uint32_t>(File, 0x00010000); // Version 0.1.0.0
			WriteStream(File, agentRadius);
			WriteStream(File, agentHeight);
			//???save other agent params too?

			WriteStream<uint32_t>(File, navDataSize);
			File.write(reinterpret_cast<const char*>(navData), navDataSize);

			WriteStream(File, static_cast<uint32_t>(NamedRegionPolys.size()));
			for (const auto& [ID, Polys] : NamedRegionPolys)
			{
				WriteStream(File, ID);
				WriteStream(File, static_cast<uint32_t>(Polys.size()));
				File.write(reinterpret_cast<const char*>(Polys.data()), Polys.size() * sizeof(dtPolyRef));
			}
		}

		dtFree(navData);

		return ETaskResult::Success;
	}

	static inline int GetVertexIndex(const char* pRawData, size_t Stride, size_t Index)
	{
		if (!pRawData) return Index;
		if (Stride == 4) return *reinterpret_cast<const uint32_t*>(pRawData + Stride * Index);
		if (Stride == 2) return *reinterpret_cast<const uint16_t*>(pRawData + Stride * Index);
		return -1;
	}

	// TODO: can optimize by reading mesh once even if it is referenced multiple times
	bool ProcessMeshGeometry(const Data::CParams& Desc, const rtm::qvvf& WorldTfm, std::vector<float>& OutVertices, std::vector<int>& OutIndices, CThreadSafeLog& Log)
	{
		const auto ResourceID = ParamsUtils::GetParam(Desc, "Mesh", std::string{});
		const auto Path = ResolvePathAliases(ResourceID, _PathAliases);

		std::ifstream File(Path, std::ios_base::binary);
		if (!File)
		{
			Log.LogError("Can't open mesh file " + Path.generic_string());
			return false;
		}

#pragma pack(push, 1)
		struct CMSHMeshHeader
		{
			uint32_t Magic;
			uint32_t Version;
			uint32_t GroupCount;
			uint32_t VertexCount;
			uint32_t IndexCount;
			uint8_t  IndexSize;
			uint32_t VertexComponentCount;
		};
		struct CMSHMeshGroup
		{
			uint32_t FirstVertex;
			uint32_t VertexCount;
			uint32_t FirstIndex;
			uint32_t IndexCount;
			uint8_t  TopologyCode;
			float    AABBMin[3];
			float    AABBMax[3];
		};
		struct CMSHVertexComponent
		{
			uint8_t SemanticCode;
			uint8_t FormatCode;
			uint8_t Index;
			uint8_t StreamIndex;
		};
#pragma pack(pop)

		CMSHMeshHeader Header;
		ReadStream(File, Header);

		if (Header.Magic != 'MESH' || Header.Version != 0x00010000)
		{
			Log.LogError("Incorrect format or version: " + Path.generic_string());
			return false;
		}

		if (!Header.VertexCount)
		{
			Log.LogWarning("Empty mesh: " + Path.generic_string());
			return true;
		}

		const auto MeshGroupIndex = static_cast<uint32_t>(ParamsUtils::GetParam(Desc, "MeshGroupIndex", 0));
		if (MeshGroupIndex >= Header.GroupCount)
		{
			Log.LogError("No group " + std::to_string(MeshGroupIndex) + " in a mesh: " + Path.generic_string());
			return false;
		}

		// Based on a vertex format, calculate offset and stride of the position
		size_t PositionOffset = 0;
		size_t VertexStride = 0;
		for (uint32_t i = 0; i < Header.VertexComponentCount; ++i)
		{
			CMSHVertexComponent Component;
			ReadStream(File, Component);

			if (Component.SemanticCode == VCSem_Position) PositionOffset = VertexStride;

			VertexStride += GetVertexComponentSize(static_cast<EVertexComponentFormat>(Component.FormatCode));
		}

		CMSHMeshGroup Group;
		size_t TriCount = 0;
		for (uint32_t i = 0; i < Header.GroupCount; ++i)
		{
			if (i == MeshGroupIndex)
			{
				ReadStream(File, Group);

				const auto IndexCount = Group.IndexCount ? Group.IndexCount : Group.VertexCount;
				if (Group.TopologyCode == Prim_TriStrip)
					TriCount = IndexCount - 2;
				else if (Group.TopologyCode == Prim_TriList)
					TriCount = IndexCount / 3;

				if (!Group.VertexCount || !TriCount)
				{
					Log.LogWarning("Empty or unsupported mesh group " + std::to_string(MeshGroupIndex) + ": " + Path.generic_string());
					return true;
				}
			}
			else
			{
				// Skip other groups
				ReadStream<CMSHMeshGroup>(File);
			}
		}

		uint32_t VertexStartPos, IndexStartPos;
		ReadStream(File, VertexStartPos);
		ReadStream(File, IndexStartPos);

		{
			// Read group vertices as raw bytes
			std::vector<char> Vertices(Group.VertexCount * VertexStride);
			File.seekg(VertexStartPos + Group.FirstVertex * VertexStride);
			File.read(Vertices.data(), Vertices.size() * sizeof(char));

			// Fill vertices
			const auto PrevVertexFloatCount = OutVertices.size();
			OutVertices.resize(PrevVertexFloatCount + 3 * Group.VertexCount);
			float* pCurrVtx = OutVertices.data() + PrevVertexFloatCount;
			const char* pSrc = Vertices.data() + PositionOffset;
			for (uint32_t i = 0; i < Group.VertexCount; ++i)
			{
				auto pPos = reinterpret_cast<const float*>(pSrc);
				const auto Vertex = rtm::qvv_mul_point3({ pPos[0], pPos[1], pPos[2] }, WorldTfm);
				*pCurrVtx++ = rtm::vector_get_x(Vertex);
				*pCurrVtx++ = rtm::vector_get_y(Vertex);
				*pCurrVtx++ = rtm::vector_get_z(Vertex);

				pSrc += VertexStride;
			}
		}

		{
			// Read group indices as raw bytes
			std::vector<char> Indices(Group.IndexCount * Header.IndexSize);
			File.seekg(IndexStartPos + Group.FirstIndex * Header.IndexSize);
			File.read(Indices.data(), Indices.size() * sizeof(char));

			// Copy indices with conversion to TriList
			// NB: this supports non-indexed geometry too through pSrc == nullptr
			const auto PrevIndexCount = OutIndices.size();
			OutIndices.resize(PrevIndexCount + 3 * TriCount);
			int* pCurrIdx = OutIndices.data() + PrevIndexCount;
			const char* pSrc = Indices.data();
			if (Group.TopologyCode == Prim_TriStrip)
			{
				bool Odd = true;
				for (uint32_t i = 0; i < TriCount; ++i, Odd = !Odd)
				{
					*pCurrIdx++ = GetVertexIndex(pSrc, Header.IndexSize, i);
					if (Odd)
					{
						*pCurrIdx++ = GetVertexIndex(pSrc, Header.IndexSize, i + 1);
						*pCurrIdx++ = GetVertexIndex(pSrc, Header.IndexSize, i + 2);
					}
					else
					{
						*pCurrIdx++ = GetVertexIndex(pSrc, Header.IndexSize, i + 2);
						*pCurrIdx++ = GetVertexIndex(pSrc, Header.IndexSize, i + 1);
					}
				}
			}
			else if (Group.TopologyCode == Prim_TriList)
			{
				for (uint32_t i = 0; i < TriCount; ++i)
				{
					*pCurrIdx++ = GetVertexIndex(pSrc, Header.IndexSize, i * 3);
					*pCurrIdx++ = GetVertexIndex(pSrc, Header.IndexSize, i * 3 + 1);
					*pCurrIdx++ = GetVertexIndex(pSrc, Header.IndexSize, i * 3 + 2);
				}
			}
		}

		return true;
	}

	bool ProcessTerrainGeometry(const Data::CParams& Desc, const rtm::qvvf& WorldTfm, std::vector<float>& OutVertices, std::vector<int>& OutIndices, CThreadSafeLog& Log)
	{
		const auto ResourceID = ParamsUtils::GetParam(Desc, "Terrain", std::string{});
		const auto Path = ResolvePathAliases(ResourceID, _PathAliases);

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
					const auto Vertex = rtm::qvv_mul_point3(
						// Local vertex
						{
							Header.MinX + i * QuadSizeX,
							(static_cast<int>(HeightMap[j * Header.Width + i]) - 32767) * Header.VerticalScale,
							Header.MinZ + j * QuadSizeZ
						},
						WorldTfm);
					*pCurrVtx++ = rtm::vector_get_x(Vertex);
					*pCurrVtx++ = rtm::vector_get_y(Vertex);
					*pCurrVtx++ = rtm::vector_get_z(Vertex);
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

	bool ProcessShapeGeometry(const Data::CParams& Desc, const rtm::qvvf& WorldTfm, std::vector<float>& OutVertices, std::vector<int>& OutIndices, CThreadSafeLog& Log)
	{
		// TODO: add shape geometry
		return false;
	}

	// NB: region is mutable for optimization (remove found elements from set, stop when empty)
	// TODO: check if it really optimizes!
	bool FillRegionPolyRefs(CRegion& Region, float cellSize, dtNavMeshQuery& Query, std::vector<dtPolyRef>& Out, CThreadSafeLog& Log)
	{
		// Add regular polys
		if (!Region.verts.empty())
		{
			// Slightly reduce region to avoid including neighbour polys
			Region.verts = OffsetPoly(Region.verts.data(), Region.verts.size() / 3, -0.95f * cellSize);
			float* vertsR = Region.verts.data();
			const size_t nvertsR = Region.verts.size() / 3;

			// TODO: ensure that region is CCW!

			float centerPos[3] = { vertsR[0], vertsR[1], vertsR[2] };
			for (size_t i = 1; i < nvertsR; ++i)
				dtVadd(centerPos, centerPos, &vertsR[i * 3]);
			dtVscale(centerPos, centerPos, 1.0f / nvertsR);

			const float midY = (Region.hmax + Region.hmin) * 0.5f;

			centerPos[1] = midY;
			for (size_t i = 0; i < nvertsR; ++i)
				vertsR[i * 3 + 1] = midY;

			// Build AABB
			float bmin[3], bmax[3];
			rcVcopy(bmin, vertsR);
			rcVcopy(bmax, vertsR);
			for (size_t i = 1; i < nvertsR; ++i)
			{
				rcVmin(bmin, &vertsR[i * 3]);
				rcVmax(bmax, &vertsR[i * 3]);
			}

			const float halfExtents[3] = { (bmax[0] - bmin[0]) * 0.5f, (Region.hmax - Region.hmin) * 0.5f, (bmax[2] - bmin[2]) * 0.5f };

			// TODO: box, sphere/circle?
			class CConvexRegionQuery : public dtPolyQuery
			{
			public:

				const CRegion& _Region;
				std::vector<dtPolyRef>& _Polys;

				CConvexRegionQuery(const CRegion& Region, std::vector<dtPolyRef>& Polys) : _Region(Region), _Polys(Polys) {}

				void process(const dtMeshTile* tile, dtPoly** polys, dtPolyRef* refs, int count)
				{
					float polyVerts[DT_VERTS_PER_POLYGON * 3];

					for (int i = 0; i < count; ++i)
					{
						const dtPoly* poly = polys[i];
						for (int j = 0; j < poly->vertCount; ++j)
						{
							const float* v = &tile->verts[poly->verts[j] * 3];
							polyVerts[j * 3] = v[0];
							polyVerts[j * 3 + 1] = v[1];
							polyVerts[j * 3 + 2] = v[2];
						}

						//???need to check Y too against _Region.hmin/hmax?
						if (dtOverlapPolyPoly2D(_Region.verts.data(), _Region.verts.size() / 3, polyVerts, poly->vertCount))
							_Polys.push_back(refs[i]);
					}
				}
			};

			// Query regular polys
			CConvexRegionQuery PolyQuery(Region, Out);
			const dtQueryFilter NavFilter;
			if (dtStatusFailed(Query.queryPolygons(centerPos, halfExtents, &NavFilter, &PolyQuery)))
			{
				Log.LogError("Failed to enumerate polys in a named region AABB");
				return false;
			}
		}

		// Add offmesh connections
		if (!Region.offmeshIds.empty())
		{
			const dtNavMesh* pNavMesh = Query.getAttachedNavMesh();
			for (int tileIdx = 0; tileIdx < pNavMesh->getMaxTiles(); ++tileIdx)
			{
				const dtMeshTile* tile = pNavMesh->getTile(tileIdx);
				if (!tile || !tile->header) continue;

				const dtPolyRef base = Query.getAttachedNavMesh()->getPolyRefBase(tile);

				for (int i = 0; i < tile->header->offMeshConCount; ++i)
				{
					const dtOffMeshConnection& offmesh = tile->offMeshCons[i];
					auto it = Region.offmeshIds.find(offmesh.userId);
					if (it != Region.offmeshIds.cend())
					{
						dtPolyRef ref = base | (dtPolyRef)(offmesh.poly);
						Out.push_back(ref);
						Region.offmeshIds.erase(it);
						if (Region.offmeshIds.empty()) return true;
					}
				}
			}
		}

		return true;
	}
};

int main(int argc, const char** argv)
{
	CNavmeshTool Tool("cf-navmesh", "Navigation mesh generator for static level geometry", { 1, 0, 0 });
	return Tool.Execute(argc, argv);
}
