#pragma once
#ifndef __DEM_L1_RENDER_DEBUG_DRAW_H__
#define __DEM_L1_RENDER_DEBUG_DRAW_H__

#include <Render/Renderer.h>
#include <Render/Geometry/Mesh.h>
#include <Render/Materials/Shader.h>

// Utility class for drawing common debug shapes

//!!!MOVE LOWLEVEL code from CNavMeshDebugDraw!

//!!!can write IRenderer and move all render-related stuff there!

namespace Render
{
#define DebugDraw Render::CDebugDraw::Instance()

class CDebugDraw: public Core::CRefCounted
{
	__DeclareSingleton(CDebugDraw);

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

	struct CVertex
	{
		vector4		Pos;		// w - point size
		vector4		Color;
	};
	#pragma pack(pop)

	PVertexLayout		ShapeInstVL;
	PVertexLayout		InstVL;
	PVertexLayout		PrimVL;
	PMesh				Shapes;
	PVertexBuffer		InstanceBuffer;

	PShader				ShapeShader;

	nArray<CShapeInst>	ShapeInsts[ShapeCount];
	nArray<CVertex>		Points;
	nArray<CVertex>		Lines;
	nArray<CVertex>		Tris;

public:

	CDebugDraw() { __ConstructSingleton; }
	~CDebugDraw() { __DestructSingleton; }

	bool	Open();
	void	Close();
	void	Render();

	bool	DrawTriangle(const vector3& P1, const vector3& P2, const vector3& P3, const vector4& Color);
	bool	DrawBox(const matrix44& Tfm, const vector4& Color);
	bool	DrawSphere(vector3 Pos, float R, const vector4& Color);
	bool	DrawCylinder(const matrix44& Tfm, float R, float Length, const vector4& Color);
	bool	DrawCapsule(const matrix44& Tfm, float R, float Length, const vector4& Color);

	bool	DrawLine(const vector3& P1, const vector3& P2, const vector4& Color);
	bool	DrawArrow();
	bool	DrawCross();
	bool	DrawBoxWireframe();
	bool	DrawArc();
	bool	DrawCircle();
	bool	DrawGridXZ();
	bool	DrawCoordAxes(const matrix44& Tfm, bool DrawX = true, bool DrawY = true, bool DrawZ = true);

	bool	DrawPoint(const vector3& Pos, const vector4& Color, float Size);

	void	AddLineVertex(const vector3& Pos, const vector4& Color);
	void	AddTriangleVertex(const vector3& Pos, const vector4& Color);

	//???DrawText();?
	//???GetTextExtent();?
};

inline bool CDebugDraw::DrawTriangle(const vector3& P1, const vector3& P2, const vector3& P3, const vector4& Color)
{
	AddTriangleVertex(P1, Color);
	AddTriangleVertex(P2, Color);
	AddTriangleVertex(P3, Color);
	OK;
}
//---------------------------------------------------------------------

inline bool CDebugDraw::DrawBox(const matrix44& Tfm, const vector4& Color)
{
	CShapeInst& Inst = *ShapeInsts[Box].Reserve(1);
	Inst.World = Tfm;
	Inst.Color = Color;
	OK;
}
//---------------------------------------------------------------------

inline bool CDebugDraw::DrawSphere(vector3 Pos, float R, const vector4& Color)
{
	CShapeInst& Inst = *ShapeInsts[Sphere].Reserve(1);
	Inst.World.set(	R, 0.f, 0.f, 0.f,
					0.f, R, 0.f, 0.f,
					0.f, 0.f, R, 0.f,
					Pos.x, Pos.y, Pos.z, 1.f);
	Inst.Color = Color;
	OK;
}
//---------------------------------------------------------------------

inline bool CDebugDraw::DrawCylinder(const matrix44& Tfm, float R, float Length, const vector4& Color)
{
	CShapeInst& Inst = *ShapeInsts[Cylinder].Reserve(1);
	Inst.World.set(	R, 0.f, 0.f, 0.f,
					0.f, R, 0.f, 0.f,
					0.f, 0.f, Length, 0.f,
					0.f, 0.f, 0.f, 1.f);
	Inst.World *= Tfm;
	Inst.Color = Color;
	OK;
}
//---------------------------------------------------------------------

inline bool CDebugDraw::DrawCapsule(const matrix44& Tfm, float R, float Length, const vector4& Color)
{
	DrawSphere(Tfm * vector3(0.0f, 0.0f, Length * 0.5f), R, Color);
	DrawSphere(Tfm * vector3(0.0f, 0.0f, Length * -0.5f), R, Color);
	DrawCylinder(Tfm, R, Length, Color);
	OK;
}
//---------------------------------------------------------------------

inline bool CDebugDraw::DrawLine(const vector3& P1, const vector3& P2, const vector4& Color)
{
	AddLineVertex(P1, Color);
	AddLineVertex(P2, Color);
	OK;
}
//---------------------------------------------------------------------

inline bool CDebugDraw::DrawCoordAxes(const matrix44& Tfm, bool DrawX, bool DrawY, bool DrawZ)
{
	if (DrawX) DrawLine(Tfm.pos_component(), Tfm.pos_component() + Tfm.x_component(), vector4::Red);
	if (DrawY) DrawLine(Tfm.pos_component(), Tfm.pos_component() + Tfm.y_component(), vector4::Green);
	if (DrawZ) DrawLine(Tfm.pos_component(), Tfm.pos_component() + Tfm.z_component(), vector4::Blue);
	OK;
}
//---------------------------------------------------------------------

inline bool CDebugDraw::DrawPoint(const vector3& Pos, const vector4& Color, float Size)
{
	CVertex* pV = Points.Reserve(1);
	pV->Pos = Pos;
	pV->Pos.w = Size;
	pV->Color = Color;
	OK;
}
//---------------------------------------------------------------------

inline void CDebugDraw::AddLineVertex(const vector3& Pos, const vector4& Color)
{
	CVertex* pV = Lines.Reserve(1);
	pV->Pos = Pos;
	pV->Color = Color;
}
//---------------------------------------------------------------------

inline void CDebugDraw::AddTriangleVertex(const vector3& Pos, const vector4& Color)
{
	CVertex* pV = Tris.Reserve(1);
	pV->Pos = Pos;
	pV->Color = Color;
}
//---------------------------------------------------------------------

}

#endif
