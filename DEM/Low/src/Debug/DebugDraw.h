#pragma once
#include <Render/RenderFwd.h>
#include <Math/AABB.h>

// Utility class for drawing common debug shapes. It buffers all shapes and text requested and
// then can pass them to a renderers. DebugDraw doesn't render anything itself.

namespace Frame
{
	typedef Ptr<class CGraphicsResourceManager> PGraphicsResourceManager;
}

namespace Debug
{

/*
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
*/

class CDebugDraw final
{
public:

	enum EShape
	{
		Box = 0,
		Sphere,
		Cylinder,
		Cone,
		SHAPE_TYPE_COUNT
	};

protected:

#pragma pack(push, 1) // Since we want to write them into hardware buffers as is, when possible
	struct CDDShapeInst
	{
		matrix44 World;
		U32      Color;
	};

	struct CDDVertex
	{
		vector4 Pos;   // w - point size
		U32     Color;
	};
#pragma pack(pop)

	/*
	struct CDDText
	{
		std::string	Text;
		U32     Color;
		float	Left;
		float	Top;
		float	Width;
		EHAlign	HAlign;
		EVAlign	VAlign;
		bool	Wrap;
	};
	*/

	Frame::PGraphicsResourceManager _GraphicsMgr;
	Render::PMesh                   _Shapes[SHAPE_TYPE_COUNT];
	Render::PVertexLayout           _ShapeVertexLayout;
	Render::PVertexBuffer           _ShapeInstanceBuffer;
	Render::PVertexBuffer           _PrimitiveBuffer;

	std::vector<CDDShapeInst>	ShapeInsts[SHAPE_TYPE_COUNT];
	std::vector<CDDVertex>		Points;
	std::vector<CDDVertex>		Lines;
	std::vector<CDDVertex>		Tris;
	//std::vector<CDDText>		Texts;

public:

	CDebugDraw(Frame::CGraphicsResourceManager& GraphicsMgr);
	~CDebugDraw();

	void Render(Render::CEffect& Effect, const matrix44& ViewProj);

	void DrawTriangle(const vector3& P1, const vector3& P2, const vector3& P3, U32 Color = Render::Color_White);
	void DrawBox(const matrix44& Tfm, U32 Color = Render::Color_White);
	void DrawSphere(const vector3& Pos, float R, U32 Color = Render::Color_White);
	void DrawCylinder(const matrix44& Tfm, float R, float Length, U32 Color = Render::Color_White);
	void DrawCapsule(const matrix44& Tfm, float R, float Length, U32 Color = Render::Color_White);

	void DrawLine(const vector3& P1, const vector3& P2, U32 Color = Render::Color_White);
	void DrawArrow(const vector3& From, const vector3& To, float Radius, U32 Color = Render::Color_White);
	void DrawCross();
	void DrawBoxWireframe(const CAABB& Box, U32 Color = Render::Color_White);
	void DrawArc();
	void DrawCircleXZ(const vector3& Pos, float Radius, float SegmentCount = 16, U32 Color = Render::Color_White);
	void DrawGridXZ(); //???or pass arbitrary axes?
	void DrawCoordAxes(const matrix44& Tfm, bool DrawX = true, bool DrawY = true, bool DrawZ = true);

	void DrawPoint(const vector3& Pos, float Size, U32 Color = Render::Color_White);

	void AddLineVertex(const vector3& Pos, U32 Color = Render::Color_White);
	void AddTriangleVertex(const vector3& Pos, U32 Color = Render::Color_White);

	//bool	DrawText(const char* pText, float Left, float Top, U32 Color = Render::Color_White, float Width = 1.f, bool Wrap = true, EHAlign HAlign = Align_Left, EVAlign VAlign = Align_Top);
};

inline void CDebugDraw::DrawPoint(const vector3& Pos, float Size, U32 Color)
{
	Points.push_back({ vector4(Pos, Size), Color });
}
//---------------------------------------------------------------------

inline void CDebugDraw::AddLineVertex(const vector3& Pos, U32 Color)
{
	Lines.push_back({ Pos, Color });
}
//---------------------------------------------------------------------

inline void CDebugDraw::AddTriangleVertex(const vector3& Pos, U32 Color)
{
	Tris.push_back({ Pos, Color });
}
//---------------------------------------------------------------------

}
