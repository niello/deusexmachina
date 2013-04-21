#pragma once
#ifndef __DEM_L1_RENDER_DEBUG_DRAW_H__
#define __DEM_L1_RENDER_DEBUG_DRAW_H__

#include <Render/Renderer.h>
#include <Render/Geometry/Mesh.h>

// Utility class for drawing common debug shapes
// Inspired by duDebugDraw from recastnavigation library

//!!!use instancing per shape type, use one instance buffer with offset for shape types!

//!!!MOVE LOWLEVEL code from CNavMeshDebugDraw!

namespace Render
{

//!!!singleton!
class CDebugDraw
{
protected:

	enum { MaxShapesPerDIP = 128 };
	enum EShape
	{
		Box = 0,
		Sphere,
		Cylinder,
		ShapeCount
	};

	#pragma pack(push, 1)
	struct CShapeInst
	{
		matrix44	World;
		vector4		Color;
	};
	#pragma pack(pop)

	PVertexLayout		ShapeInstVL;
	PVertexLayout		InstVL;
	PMesh				Shapes;
	PVertexBuffer		InstanceBuffer;

	//!!!shape shader!
	//???or in renderer? renderer must get data from this singleton

	nArray<CShapeInst>	ShapeInsts[ShapeCount];

public:

	bool	Open();
	void	Close();

	bool	DrawBox();
	bool	DrawSphere();
	bool	DrawCylinder();
	bool	DrawCapsule();

	bool	DrawLine();
	bool	DrawArrow();
	bool	DrawCross();
	bool	DrawBoxWireframe();
	bool	DrawArc();
	bool	DrawCircle();
	bool	DrawGridXZ();
	bool	DrawCoordAxes(/*tfm, flags x, y, z*/);

	//???DrawText();?
	//???GetTextExtent();?
};

//typedef Ptr<CShapeRenderer> PShapeRenderer;

}

#endif
