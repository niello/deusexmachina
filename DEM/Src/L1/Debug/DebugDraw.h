#pragma once
#ifndef __DEM_L1_RENDER_DEBUG_DRAW_H__
#define __DEM_L1_RENDER_DEBUG_DRAW_H__

#include <Render/Renderer.h>
#include <Render/Mesh.h>
#include <Render/Shader.h>
#include <Data/Singleton.h>

// Utility class for drawing common debug shapes

//!!!can write IRenderer and move all render-related stuff there!
//!!!and move this facility into a Debug namespace and folder then!

namespace Render
{
#define DebugDraw Render::CDebugDraw::Instance()

#pragma pack(push, 1) // Since we want write them into hardware buffers as is, when possible
struct CDDShapeInst
{
	matrix44	World;
	vector4		Color;
};

struct CDDVertex
{
	vector4		Pos;		// w - point size
	vector4		Color;
};
#pragma pack(pop)

enum EHAlign
{
	Align_Left,
	Align_HCenter,
	Align_Right,
	Align_Justify
};

enum EVAlign
{
	Align_Top,
	Align_VCenter,
	Align_Bottom
};

struct CDDText
{
	CString	Text;
	vector4	Color;
	float	Left;
	float	Top;
	float	Width;
	EHAlign	HAlign;
	EVAlign	VAlign;
	bool	Wrap;
};

class CDebugDraw: public Core::CObject
{
	__DeclareSingleton(CDebugDraw);

public:

	enum { MaxShapesPerDIP = 128 };

	enum EShape
	{
		Box = 0,
		Sphere,
		Cylinder,
		ShapeCount
	};

protected:

	PVertexLayout			ShapeInstVL;
	PVertexLayout			InstVL;
	PVertexLayout			PrimVL;
	PMesh					Shapes;
	PVertexBuffer			InstanceBuffer;

	PShader					ShapeShader;

	ID3DXFont*				pD3DXFont;
	ID3DXSprite*			pD3DXSprite;

	CArray<CDDShapeInst>	ShapeInsts[ShapeCount];
	CArray<CDDVertex>		Points;
	CArray<CDDVertex>		Lines;
	CArray<CDDVertex>		Tris;
	CArray<CDDText>			Texts;

public:

	CDebugDraw(): Lines(0, 256) { __ConstructSingleton; }
	~CDebugDraw() { __DestructSingleton; }

	bool	Open();
	void	Close();

	//???!!!move code to renderers?!
	void	RenderGeometry();
	void	RenderText();

	bool	DrawTriangle(const vector3& P1, const vector3& P2, const vector3& P3, const vector4& Color = vector4::White);
	bool	DrawBox(const matrix44& Tfm, const vector4& Color = vector4::White);
	bool	DrawSphere(const vector3& Pos, float R, const vector4& Color = vector4::White);
	bool	DrawCylinder(const matrix44& Tfm, float R, float Length, const vector4& Color = vector4::White);
	bool	DrawCapsule(const matrix44& Tfm, float R, float Length, const vector4& Color = vector4::White);

	bool	DrawLine(const vector3& P1, const vector3& P2, const vector4& Color = vector4::White);
	bool	DrawArrow();
	bool	DrawCross();
	bool	DrawBoxWireframe(const CAABB& Box, const vector4& Color = vector4::White);
	bool	DrawArc();
	bool	DrawCircle();
	bool	DrawGridXZ(); //???or pass arbitrary axes?
	bool	DrawCoordAxes(const matrix44& Tfm, bool DrawX = true, bool DrawY = true, bool DrawZ = true);

	bool	DrawPoint(const vector3& Pos, float Size, const vector4& Color = vector4::White);

	void	AddLineVertex(const vector3& Pos, const vector4& Color = vector4::White);
	void	AddTriangleVertex(const vector3& Pos, const vector4& Color = vector4::White);

	bool	DrawText(const char* pText, float Left, float Top, const vector4& Color = vector4::White, float Width = 1.f, bool Wrap = true, EHAlign HAlign = Align_Left, EVAlign VAlign = Align_Top);
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
	CDDShapeInst& Inst = *ShapeInsts[Box].Reserve(1);
	Inst.World = Tfm;
	Inst.Color = Color;
	OK;
}
//---------------------------------------------------------------------

inline bool CDebugDraw::DrawSphere(const vector3& Pos, float R, const vector4& Color)
{
	CDDShapeInst& Inst = *ShapeInsts[Sphere].Reserve(1);
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
	// Cylinder axis is Z, by we draw Y-aligned one by default as the most useful
	CDDShapeInst& Inst = *ShapeInsts[Cylinder].Reserve(1);
	Inst.World.set(	R, 0.f, 0.f, 0.f,
					0.f, 0.f, R, 0.f,
					0.f, -Length, 0.f, 0.f,
					0.f, 0.f, 0.f, 1.f);
	Inst.World *= Tfm;
	Inst.Color = Color;
	OK;
}
//---------------------------------------------------------------------

inline bool CDebugDraw::DrawCapsule(const matrix44& Tfm, float R, float Length, const vector4& Color)
{
	DrawSphere(Tfm * vector3(0.0f, Length * -0.5f, 0.0f), R, Color);
	DrawSphere(Tfm * vector3(0.0f, Length * 0.5f, 0.0f), R, Color);
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

inline bool CDebugDraw::DrawBoxWireframe(const CAABB& Box, const vector4& Color)
{
	vector3 mmm = Box.GetCorner(0);
	vector3 mMm = Box.GetCorner(1);
	vector3 MMm = Box.GetCorner(2);
	vector3 Mmm = Box.GetCorner(3);
	vector3 MMM = Box.GetCorner(4);
	vector3 mMM = Box.GetCorner(5);
	vector3 mmM = Box.GetCorner(6);
	vector3 MmM = Box.GetCorner(7);
	DrawLine(mmm, Mmm, Color);
	DrawLine(mmm, mMm, Color);
	DrawLine(mmm, mmM, Color);
	DrawLine(MMM, MMm, Color);
	DrawLine(MMM, mMM, Color);
	DrawLine(MMM, MmM, Color);
	DrawLine(mMm, mMM, Color);
	DrawLine(mMm, MMm, Color);
	DrawLine(mMM, mmM, Color);
	DrawLine(Mmm, MmM, Color);
	DrawLine(mmM, MmM, Color);
	DrawLine(MMm, Mmm, Color);
	OK;
}
//---------------------------------------------------------------------

inline bool CDebugDraw::DrawCoordAxes(const matrix44& Tfm, bool DrawX, bool DrawY, bool DrawZ)
{
	if (DrawX) DrawLine(Tfm.Translation(), Tfm.Translation() + Tfm.AxisX(), vector4::Red);
	if (DrawY) DrawLine(Tfm.Translation(), Tfm.Translation() + Tfm.AxisY(), vector4::Green);
	if (DrawZ) DrawLine(Tfm.Translation(), Tfm.Translation() + Tfm.AxisZ(), vector4::Blue);
	OK;
}
//---------------------------------------------------------------------

inline bool CDebugDraw::DrawPoint(const vector3& Pos, float Size, const vector4& Color)
{
	CDDVertex* pV = Points.Reserve(1);
	pV->Pos = Pos;
	pV->Pos.w = Size;
	pV->Color = Color;
	OK;
}
//---------------------------------------------------------------------

inline void CDebugDraw::AddLineVertex(const vector3& Pos, const vector4& Color)
{
	CDDVertex* pV = Lines.Reserve(1);
	pV->Pos = Pos;
	pV->Color = Color;
}
//---------------------------------------------------------------------

inline void CDebugDraw::AddTriangleVertex(const vector3& Pos, const vector4& Color)
{
	CDDVertex* pV = Tris.Reserve(1);
	pV->Pos = Pos;
	pV->Color = Color;
}
//---------------------------------------------------------------------

inline bool CDebugDraw::DrawText(const char* pText, float Left, float Top, const vector4& Color, float Width, bool Wrap, EHAlign HAlign, EVAlign VAlign)
{
	CDDText* pT = Texts.Reserve(1);
	pT->Text = pText;
	pT->Color = Color;
	pT->Left = Left;
	pT->Top = Top;
	pT->Width = Width;
	pT->Wrap = Wrap;
	pT->HAlign = HAlign;
	pT->VAlign = VAlign;
	OK;
}
//---------------------------------------------------------------------

}

#endif
