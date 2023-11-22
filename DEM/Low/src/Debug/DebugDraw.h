#pragma once
#include <Render/RenderFwd.h>

// Utility class for drawing common debug shapes. It buffers all shapes and text requested and
// then can pass them to a renderers. DebugDraw doesn't render anything itself.

class CAABB;

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
		rtm::matrix4x4f World;
		U32             Color;
	};

	struct CDDVertex
	{
		rtm::vector4f Pos;   // w - point or line thickness
		U32           Color;
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

	// Optimized for debug builds, where debug rendering is typically used
	struct CVertexArray final
	{
		CDDVertex* pData = nullptr;
		UPTR       Capacity = 0;
		UPTR       Count = 0;

		CVertexArray() { if (pData) n_free_aligned(pData); }

		CDDVertex& Add()
		{
			if (Count >= Capacity)
			{
				constexpr UPTR GROW_SIZE = 1000;
				pData = static_cast<CDDVertex*>(n_realloc_aligned(pData, (Count + GROW_SIZE) * sizeof(CDDVertex), 16));
				//constexpr rtm::vector4f DefaultValue{ 0.f, 0.f, 0.f, 1.f };
				//for (UPTR i = Count; i < Count + GROW_SIZE; ++i)
				//	pData[i].Pos = DefaultValue;
				Capacity = Count + GROW_SIZE;
			}
			return pData[Count++];
		}
	};

	Frame::PGraphicsResourceManager _GraphicsMgr;
	Render::PMesh                   _Shapes[SHAPE_TYPE_COUNT];
	Render::PVertexLayout           _ShapeVertexLayout;
	Render::PVertexBuffer           _ShapeInstanceBuffer;
	Render::PVertexBuffer           _PrimitiveBuffer;

	std::vector<CDDShapeInst> ShapeInsts[SHAPE_TYPE_COUNT];
	CVertexArray              Points;
	CVertexArray              LineVertices;
	CVertexArray              TriVertices;
	//std::vector<CDDText>    Texts;

	void DrawPointInternal(const rtm::vector4f& PosSize, U32 Color)
	{
		auto& Vertex = Points.Add();
		Vertex.Pos = PosSize;
		Vertex.Color = Color;
	}

	void AddLineVertexInternal(const rtm::vector4f& PosThickness, U32 Color)
	{
		auto& Vertex = LineVertices.Add();
		Vertex.Pos = PosThickness;
		Vertex.Color = Color;
	}

	void AddTriangleVertexInternal(const rtm::vector4f& Pos, U32 Color)
	{
		auto& Vertex = TriVertices.Add();
		Vertex.Pos = Pos;
		Vertex.Color = Color;
	}

public:

	CDebugDraw(Frame::CGraphicsResourceManager& GraphicsMgr);
	~CDebugDraw();

	void Render(Render::CEffect& Effect, const rtm::matrix4x4f& ViewProj);

	void DrawTriangle(const vector3& P1, const vector3& P2, const vector3& P3, U32 Color = Render::Color_White);
	void DrawBox(const rtm::matrix3x4f& Tfm, U32 Color = Render::Color_White);
	void DrawSphere(const rtm::vector4f& Pos, float R, U32 Color = Render::Color_White);
	void DrawCylinder(const rtm::matrix3x4f& Tfm, float R, float Length, U32 Color = Render::Color_White);
	void DrawCapsule(const rtm::matrix3x4f& Tfm, float R, float Length, U32 Color = Render::Color_White);

	void DrawLine(const vector3& P1, const vector3& P2, U32 Color = Render::Color_White, float Thickness = 1.f);
	void DrawLine(const rtm::vector4f& P1, const rtm::vector4f& P2, U32 Color = Render::Color_White, float Thickness = 1.f);
	void DrawArrow(const vector3& From, const vector3& To, float Radius, U32 Color = Render::Color_White);
	void DrawCross();
	void DrawBoxWireframe(const Math::CAABB& Box, U32 Color = Render::Color_White, float Thickness = 1.f);
	void DrawSphereWireframe(const vector3& Pos, float R, U32 Color = Render::Color_White, float Thickness = 1.f);
	void DrawFrustumWireframe(const rtm::matrix4x4f& Frustum, U32 Color = Render::Color_White, float Thickness = 1.f);
	void DrawArc();
	void DrawCircleXZ(const vector3& Pos, float Radius, float SegmentCount = 16, U32 Color = Render::Color_White);
	void DrawGridXZ(); //???or pass arbitrary axes?
	void DrawCoordAxes(const rtm::matrix3x4f& Tfm, bool DrawX = true, bool DrawY = true, bool DrawZ = true);

	void DrawPoint(const vector3& Pos, U32 Color = Render::Color_White, float Size = 1.f) { DrawPointInternal(rtm::vector_set(Pos.x, Pos.y, Pos.z, Size), Color); }
	void DrawPoint(float x, float y, float z, U32 Color = Render::Color_White, float Size = 1.f) { DrawPointInternal(rtm::vector_set(x, y, z, Size), Color); }
	void DrawPoint(const rtm::vector4f& Pos, U32 Color = Render::Color_White, float Size = 1.f) { DrawPointInternal(rtm::vector_set_w(Pos, Size), Color); }

	void AddLineVertex(const vector3& Pos, U32 Color = Render::Color_White, float Thickness = 1.f) { AddLineVertexInternal(rtm::vector_set(Pos.x, Pos.y, Pos.z, Thickness), Color); }
	void AddLineVertex(float x, float y, float z, U32 Color = Render::Color_White, float Thickness = 1.f) { AddLineVertexInternal(rtm::vector_set(x, y, z, Thickness), Color); }
	void AddLineVertex(const rtm::vector4f& Pos, U32 Color = Render::Color_White, float Thickness = 1.f) { AddLineVertexInternal(rtm::vector_set_w(Pos, Thickness), Color); }

	void AddTriangleVertex(const vector3& Pos, U32 Color = Render::Color_White) { AddTriangleVertexInternal(rtm::vector_set(Pos.x, Pos.y, Pos.z, 1.f), Color); }
	void AddTriangleVertex(float x, float y, float z, U32 Color = Render::Color_White) { AddTriangleVertexInternal(rtm::vector_set(x, y, z, 1.f), Color); }
	void AddTriangleVertex(const rtm::vector4f& Pos, U32 Color = Render::Color_White) { AddTriangleVertexInternal(Pos/*rtm::vector_set_w(Pos, 1.f)*/, Color); }

	//bool	DrawText(const char* pText, float Left, float Top, U32 Color = Render::Color_White, float Width = 1.f, bool Wrap = true, EHAlign HAlign = Align_Left, EVAlign VAlign = Align_Top);
};

}
