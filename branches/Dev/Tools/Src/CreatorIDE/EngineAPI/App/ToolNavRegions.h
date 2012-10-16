#pragma once
#ifndef __CIDE_EDITOR_TOOL_NAV_REGIONS_H__
#define __CIDE_EDITOR_TOOL_NAV_REGIONS_H__

#include <App/EditorTool.h>
#include <Events/Events.h>
#include <AI/Navigation/NavMesh.h> // For MAX_CONVEXVOL_PTS

// Navigation regions tool. Use it to mark areas (AABB, cylinder, convex volume) on the navmesh
// and then work with these areas through entities.
// This tool is based on ConvexVolumeTool from Recast Navigation.

namespace App
{

class CToolNavRegions: public IEditorTool
{
	//DeclareRTTI;

protected:

	vector3	Points[MAX_CONVEXVOL_PTS];
	int		PointCount;
	int		Hull[MAX_CONVEXVOL_PTS];
	int		HullSize;

	int		MBDownX;
	int		MBDownY;

	DECLARE_EVENT_HANDLER(MouseBtnDown, OnMBDown);
	DECLARE_EVENT_HANDLER(MouseBtnUp, OnClick);

public:

	uchar	Area;
	//float	PolyOffset = 0.f;
	float	BoxHeight;
	float	BoxDescent;

	CToolNavRegions(): Area(NAV_AREA_NAMED), BoxHeight(6.0f), BoxDescent(1.0f), PointCount(0), HullSize(0) {}
	//virtual ~CToolNavRegions() {}

	virtual void Activate();
	virtual void Deactivate();
	virtual void Render();
};

}

#endif
