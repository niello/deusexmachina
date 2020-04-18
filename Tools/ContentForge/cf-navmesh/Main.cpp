#include <ContentForgeTool.h>
#include <Utils.h>
#include <ParamsUtils.h>
//#include <CLI11.hpp>
#include <Recast.h>
#include <DetourNavMesh.h> // For max vertices per polygon constant
#include <DetourNavMeshBuilder.h>

namespace fs = std::filesystem;

// Set working directory to $(TargetDir)
// Example args:
// -s src/game/levels

// FIXME: to utils!
struct float3
{
	union
	{
		struct { float x, y, z; };
		float v[3];
	};
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
		if (ParamsUtils::GetParam(Task.Params, "Tools", std::string{}) != "cf-navmesh")
		{
			// FIXME: skip sliently without error. To CF tool base class???
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

		//???use level + world or incoming data must be a ready-made list of .scn + root world tfm for each?
		//can feed from game or editor, usiversal, requires no ECS knowledge and suports ofmeshes well
		// iterate all scn with static data for the level
		// collect collision shapes and/or visible geometry
		// add geometry from static entities (static is a flag in a CSceneComponent)
		// add offmesh links
		// add typed areas

		struct CConvexVolume
		{
			std::vector<float3> verts;
			float hmin, hmax;
			unsigned char areaId;
		};

		struct COffmeshConnection
		{
			float3 start;
			float3 end;
			float radius;
			bool isBidirectional;
			unsigned char areaId;
			unsigned short flags;
			unsigned int userId;
		};

		std::vector<float3> verts;
		std::vector<int> tris;
		std::vector<CConvexVolume> vols;
		std::vector<COffmeshConnection> offmesh;

		// Collect geometry

		Data::CDataArray* pGeometryList;
		if (ParamsUtils::TryGetParam(pGeometryList, Desc, "Geometry"))
		{
			for (const auto& GeometryRecord : *pGeometryList)
			{
				// parse world transformation, identity by default

				const auto& GeometryDesc = GeometryRecord.GetValue<Data::CParams>();
				if (ParamsUtils::HasParam(GeometryDesc, CStrID("Mesh")))
				{
					// add mesh geometry
				}
				else if (ParamsUtils::HasParam(GeometryDesc, CStrID("Terrain")))
				{
					if (!ProcessTerrainGeometry(GeometryDesc, verts, tris))
						Task.Log.LogWarning("Couldn't export terrain geometry");
				}
				else if (ParamsUtils::HasParam(GeometryDesc, CStrID("Shape")))
				{
					// add shape geometry
				}
			}
		}

		const int ntris = tris.size() / 3;

		// TODO: can add optional bounds to config, to use only interactive level part for example
		float bmax[3] = {};
		float bmin[3] = {};
		if (!verts.empty()) rcCalcBounds(verts.data()->v, verts.size(), bmin, bmax);

		// NB: the code below is copied from RecastDemo (Sample_SoloMesh.cpp) with slight changes

		// Step 1. Initialize build config.

		// TODO: attach task log to the context
		rcContext ctx;

		//!!!build navmesh for each agent's R+h! agent selects all h >= its h and from them the closest R >= its R
		const float agentHeight = ParamsUtils::GetParam(Desc, "AgentHeight", 1.8f);
		const float agentRadius = ParamsUtils::GetParam(Desc, "AgentRadius", 0.3f);
		const float agentMaxClimb = ParamsUtils::GetParam(Desc, "AgentMaxClimb", 0.2f);
		const float agentWalkableSlope = ParamsUtils::GetParam(Desc, "AgentWalkableSlope", 60.f);

		//!!!TODO: most of these things must be in settings!
		const float cellSize = ParamsUtils::GetParam(Desc, "CellSize", agentRadius / 3.f);
		const float cellHeight = ParamsUtils::GetParam(Desc, "CellHeight", cellSize);
		const float edgeMaxLen = ParamsUtils::GetParam(Desc, "EdgeMaxLength", 12.f);
		const float edgeMaxError = ParamsUtils::GetParam(Desc, "EdgeMaxError", 1.3f);
		const int regionMinSize = ParamsUtils::GetParam(Desc, "RegionMinSize", 8);
		const int regionMergeSize = ParamsUtils::GetParam(Desc, "RegionMergeSize", 8);
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

		auto solid = rcAllocHeightfield();
		if (!rcCreateHeightfield(&ctx, *solid, cfg.width, cfg.height, cfg.bmin, cfg.bmax, cfg.cs, cfg.ch))
		{
			Task.Log.LogError("buildNavigation: Could not create solid heightfield.");
			return false;
		}

		// TODO: there is also rcClearUnwalkableTriangles, was used with non-RC_NULL_AREA in old CIDE
		std::unique_ptr<unsigned char[]> triareas(new unsigned char[ntris]);
		memset(triareas.get(), 0, ntris * sizeof(unsigned char));
		rcMarkWalkableTriangles(&ctx, cfg.walkableSlopeAngle, verts.data()->v, verts.size(), tris.data(), ntris, triareas.get());
		if (!rcRasterizeTriangles(&ctx, verts.data()->v, verts.size(), tris.data(), triareas.get(), ntris, *solid, cfg.walkableClimb))
		{
			Task.Log.LogError("buildNavigation: Could not rasterize triangles.");
			return false;
		}

		triareas.reset();

		// Step 3. Filter walkables surfaces.

		rcFilterLowHangingWalkableObstacles(&ctx, cfg.walkableClimb, *solid);
		rcFilterLedgeSpans(&ctx, cfg.walkableHeight, cfg.walkableClimb, *solid);
		rcFilterWalkableLowHeightSpans(&ctx, cfg.walkableHeight, *solid);

		// Step 4. Partition walkable surface to simple regions.

		auto chf = rcAllocCompactHeightfield();
		if (!rcBuildCompactHeightfield(&ctx, cfg.walkableHeight, cfg.walkableClimb, *solid, *chf))
		{
			Task.Log.LogError("buildNavigation: Could not build compact data.");
			return false;
		}

		rcFreeHeightField(solid);

		if (!rcErodeWalkableArea(&ctx, cfg.walkableRadius, *chf))
		{
			Task.Log.LogError("buildNavigation: Could not erode.");
			return false;
		}

		for (const auto& vol : vols)
			rcMarkConvexPolyArea(&ctx, vol.verts.data()->v, vol.verts.size(), vol.hmin, vol.hmax, vol.areaId, *chf);

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

		auto cset = rcAllocContourSet();
		if (!rcBuildContours(&ctx, *chf, cfg.maxSimplificationError, cfg.maxEdgeLen, *cset))
		{
			Task.Log.LogError("buildNavigation: Could not create contours.");
			return false;
		}

		// Step 6. Build polygons mesh from contours.

		auto pmesh = rcAllocPolyMesh();
		if (!rcBuildPolyMesh(&ctx, *cset, cfg.maxVertsPerPoly, *pmesh))
		{
			Task.Log.LogError("buildNavigation: Could not triangulate contours.");
			return false;
		}

		// Step 7. Create detail mesh which allows to access approximate height on each polygon.
		//???optional?

		auto dmesh = rcAllocPolyMeshDetail();
		if (!rcBuildPolyMeshDetail(&ctx, *pmesh, *chf, cfg.detailSampleDist, cfg.detailSampleMaxError, *dmesh))
		{
			Task.Log.LogError("buildNavigation: Could not build detail mesh.");
			return false;
		}

		rcFreeCompactHeightfield(chf);
		rcFreeContourSet(cset);

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

		// Copy offmesh connection data into a format recognizable by Detour navmesh builder
		std::vector<float> offMeshConVerts;
		std::vector<float> offMeshConRads;
		std::vector<unsigned char> offMeshConDirs;
		std::vector<unsigned char> offMeshConAreas;
		std::vector<unsigned short> offMeshConFlags;
		std::vector<unsigned int> offMeshConId;
		if (!offmesh.empty())
		{
			offMeshConVerts.reserve(offmesh.size() * 3 * 2);
			offMeshConRads.reserve(offmesh.size());
			offMeshConDirs.reserve(offmesh.size());
			offMeshConAreas.reserve(offmesh.size());
			offMeshConFlags.reserve(offmesh.size());
			offMeshConId.reserve(offmesh.size());

			for (const auto& conn : offmesh)
			{
				offMeshConVerts.push_back(conn.start.x);
				offMeshConVerts.push_back(conn.start.y);
				offMeshConVerts.push_back(conn.start.z);
				offMeshConVerts.push_back(conn.end.x);
				offMeshConVerts.push_back(conn.end.y);
				offMeshConVerts.push_back(conn.end.z);
				offMeshConRads.push_back(conn.radius);
				offMeshConDirs.push_back(conn.isBidirectional ? 1 : 0);
				offMeshConAreas.push_back(conn.areaId);
				offMeshConFlags.push_back(conn.flags);
				offMeshConId.push_back(conn.userId);
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
		params.detailMeshes = dmesh->meshes;
		params.detailVerts = dmesh->verts;
		params.detailVertsCount = dmesh->nverts;
		params.detailTris = dmesh->tris;
		params.detailTriCount = dmesh->ntris;
		params.offMeshConVerts = offMeshConVerts.data();
		params.offMeshConRad = offMeshConRads.data();
		params.offMeshConDir = offMeshConDirs.data();
		params.offMeshConAreas = offMeshConAreas.data();
		params.offMeshConFlags = offMeshConFlags.data();
		params.offMeshConUserID = offMeshConId.data();
		params.offMeshConCount = offmesh.size();
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

		// create output folder
		// save separate nm file for each R+h
		// Old format was:
		// Magic
		// Version
		// NM count
		// Per NM:
		//  R
		//  h
		//  Data size
		//  Data
		//  Named region count
		//  Per named convex volume:
		//   Poly count
		//   Poly refs

		dtFree(navData);
			
		return true;
	}

	//!!!add transform arg!
	bool ProcessTerrainGeometry(const Data::CParams& Desc, std::vector<float3>& OutVertices, std::vector<int>& OutIndices)
	{
		return false;
	}
};

int main(int argc, const char** argv)
{
	CNavmeshTool Tool("cf-navmesh", "Navigation mesh generator for static level geometry", { 0, 1, 0 });
	return Tool.Execute(argc, argv);
}
