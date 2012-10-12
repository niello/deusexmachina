#pragma once
#ifndef __CIDE_EDITOR_TOOL_NAV_OFFMESH_H__
#define __CIDE_EDITOR_TOOL_NAV_OFFMESH_H__

#include <App/EditorTool.h>
#include <Events/Events.h>

// Offmesh connection tool for the navigation mesh.
// This tool is based on ConvexVolumeTool from Recast Navigation.

namespace App
{

class CToolNavOffmesh: public IEditorTool
{
	//DeclareRTTI;

protected:

	static int OffmeshUID;

	vector3	HitPos;
	bool	HitPosSet;
	bool	Bidirectional;

	int		MBDownX;
	int		MBDownY;

	DECLARE_EVENT_HANDLER(MouseBtnDown, OnMBDown);
	DECLARE_EVENT_HANDLER(MouseBtnUp, OnClick);

public:

	CToolNavOffmesh(): HitPosSet(false), Bidirectional(true) {}
	//virtual ~CToolNavOffmesh() {}

	virtual void Activate();
	virtual void Deactivate();
	virtual void Render();
};

}

#endif
