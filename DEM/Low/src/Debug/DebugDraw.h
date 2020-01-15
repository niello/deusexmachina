#pragma once
#include <Data/RefCounted.h>
#include <Data/Singleton.h>
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

#define DebugDraw Debug::CDebugDraw::Instance()

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

class CDebugDraw: public Data::CRefCounted
{
	__DeclareSingleton(CDebugDraw);

public:

	enum EShape
	{
		Box = 0,
		Sphere,
		Cylinder,
		Cone,
		ShapeCount
	};

protected:

#pragma pack(push, 1) // Since we want write them into hardware buffers as is, when possible
	struct CDDShapeInst
	{
		matrix44 World;
		vector4  Color;
	};

	struct CDDVertex
	{
		vector4 Pos;   // w - point size
		vector4 Color;
	};
#pragma pack(pop)

	/*
	struct CDDText
	{
		std::string	Text;
		vector4	Color;
		float	Left;
		float	Top;
		float	Width;
		EHAlign	HAlign;
		EVAlign	VAlign;
		bool	Wrap;
	};
	*/

	Frame::PGraphicsResourceManager _GraphicsMgr;
	Render::PEffect                 _Effect;
	Render::PMesh                   _Shapes[ShapeCount];
	Render::PVertexLayout           _ShapeVertexLayout;
	Render::PVertexLayout           _PrimitiveVertexLayout;
	Render::PVertexBuffer           _ShapeInstanceBuffer;

	//PVertexLayout			ShapeInstVL;
	//PVertexLayout			InstVL;
	//PVertexLayout			PrimVL;
	//PVertexBuffer			InstanceBuffer;

	std::vector<CDDShapeInst>	ShapeInsts[ShapeCount];
	std::vector<CDDVertex>		Points;
	std::vector<CDDVertex>		Lines;
	std::vector<CDDVertex>		Tris;
	//std::vector<CDDText>		Texts;

public:

	CDebugDraw(Frame::CGraphicsResourceManager& GraphicsMgr, Render::PEffect Effect);
	~CDebugDraw();

	void Render();

	void DrawTriangle(const vector3& P1, const vector3& P2, const vector3& P3, const vector4& Color = vector4::White);
	void DrawBox(const matrix44& Tfm, const vector4& Color = vector4::White);
	void DrawSphere(const vector3& Pos, float R, const vector4& Color = vector4::White);
	void DrawCylinder(const matrix44& Tfm, float R, float Length, const vector4& Color = vector4::White);
	void DrawCapsule(const matrix44& Tfm, float R, float Length, const vector4& Color = vector4::White);

	void DrawLine(const vector3& P1, const vector3& P2, const vector4& Color = vector4::White);
	void DrawArrow(const vector3& From, const vector3& To, float Radius, const vector4& Color = vector4::White);
	void DrawCross();
	void DrawBoxWireframe(const CAABB& Box, const vector4& Color = vector4::White);
	void DrawArc();
	void DrawCircle(const vector3& Pos, float Radius, const vector4& Color = vector4::White);
	void DrawGridXZ(); //???or pass arbitrary axes?
	void DrawCoordAxes(const matrix44& Tfm, bool DrawX = true, bool DrawY = true, bool DrawZ = true);

	void DrawPoint(const vector3& Pos, float Size, const vector4& Color = vector4::White);

	void AddLineVertex(const vector3& Pos, const vector4& Color = vector4::White);
	void AddTriangleVertex(const vector3& Pos, const vector4& Color = vector4::White);

	//bool	DrawText(const char* pText, float Left, float Top, const vector4& Color = vector4::White, float Width = 1.f, bool Wrap = true, EHAlign HAlign = Align_Left, EVAlign VAlign = Align_Top);
};

inline void CDebugDraw::DrawPoint(const vector3& Pos, float Size, const vector4& Color)
{
	Points.push_back({ vector4(Pos, Size), Color });
}
//---------------------------------------------------------------------

inline void CDebugDraw::AddLineVertex(const vector3& Pos, const vector4& Color)
{
	Lines.push_back({ Pos, Color });
}
//---------------------------------------------------------------------

inline void CDebugDraw::AddTriangleVertex(const vector3& Pos, const vector4& Color)
{
	Tris.push_back({ Pos, Color });
}
//---------------------------------------------------------------------

}
