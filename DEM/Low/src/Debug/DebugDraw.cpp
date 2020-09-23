#include "DebugDraw.h"
#include <Frame/GraphicsResourceManager.h>
#include <Resources/ResourceManager.h>
#include <Resources/Resource.h>
#include <Render/MeshGenerators.h>
#include <Render/Mesh.h>
#include <Render/Effect.h>
#include <Render/GPUDriver.h>
#include <Render/RenderTarget.h>
#include <Render/ShaderParamStorage.h>
#include <Render/VertexComponent.h>

namespace Debug
{
constexpr UPTR MAX_SHAPE_INSTANCES_PER_DIP = 1024;
constexpr UPTR MAX_PRIMITIVE_VERTICES_PER_DIP = 65536;

CDebugDraw::CDebugDraw(Frame::CGraphicsResourceManager& GraphicsMgr)
	: _GraphicsMgr(&GraphicsMgr)
{
	GraphicsMgr.GetResourceManager()->RegisterResource("#Mesh_BoxCCW", n_new(Resources::CMeshGeneratorBox()));
	GraphicsMgr.GetResourceManager()->RegisterResource("#Mesh_SphereCCW12", n_new(Resources::CMeshGeneratorSphere(12)));
	GraphicsMgr.GetResourceManager()->RegisterResource("#Mesh_CylinderCCW12", n_new(Resources::CMeshGeneratorCylinder(12)));
	GraphicsMgr.GetResourceManager()->RegisterResource("#Mesh_ConeCCW12", n_new(Resources::CMeshGeneratorCone(12)));

	_Shapes[Box] = GraphicsMgr.GetMesh(CStrID("#Mesh_BoxCCW"));
	_Shapes[Sphere] = GraphicsMgr.GetMesh(CStrID("#Mesh_SphereCCW12"));
	_Shapes[Cylinder] = GraphicsMgr.GetMesh(CStrID("#Mesh_CylinderCCW12"));
	_Shapes[Cone] = GraphicsMgr.GetMesh(CStrID("#Mesh_ConeCCW12"));

	// Position, transformation matrix44, color
	Render::CVertexComponent ShapeComponents[] = {
		{ Render::VCSem_Position, nullptr, 0, Render::VCFmt_Float32_3, 0, DEM_VERTEX_COMPONENT_OFFSET_DEFAULT, false },
		{ Render::VCSem_TexCoord, nullptr, 4, Render::VCFmt_Float32_4, 1, DEM_VERTEX_COMPONENT_OFFSET_DEFAULT, true },
		{ Render::VCSem_TexCoord, nullptr, 5, Render::VCFmt_Float32_4, 1, DEM_VERTEX_COMPONENT_OFFSET_DEFAULT, true },
		{ Render::VCSem_TexCoord, nullptr, 6, Render::VCFmt_Float32_4, 1, DEM_VERTEX_COMPONENT_OFFSET_DEFAULT, true },
		{ Render::VCSem_TexCoord, nullptr, 7, Render::VCFmt_Float32_4, 1, DEM_VERTEX_COMPONENT_OFFSET_DEFAULT, true },
		{ Render::VCSem_Color, nullptr, 0, Render::VCFmt_UInt8_4_Norm, 1, DEM_VERTEX_COMPONENT_OFFSET_DEFAULT, true } };

	_ShapeVertexLayout = GraphicsMgr.GetGPU()->CreateVertexLayout(ShapeComponents, sizeof_array(ShapeComponents));

	// TODO: dynamically growing buffers!
	auto ShapeInstanceVertexLayout = GraphicsMgr.GetGPU()->CreateVertexLayout(ShapeComponents + 1, sizeof_array(ShapeComponents) - 1);
	_ShapeInstanceBuffer = GraphicsMgr.GetGPU()->CreateVertexBuffer(*ShapeInstanceVertexLayout, MAX_SHAPE_INSTANCES_PER_DIP, Render::Access_CPU_Write | Render::Access_GPU_Read);

	// Position with size in W, color
	Render::CVertexComponent PrimitiveComponents[] = {
		{ Render::VCSem_Position, nullptr, 0, Render::VCFmt_Float32_4, 0, DEM_VERTEX_COMPONENT_OFFSET_DEFAULT, false },
		{ Render::VCSem_Color, nullptr, 0, Render::VCFmt_UInt8_4_Norm, 0, DEM_VERTEX_COMPONENT_OFFSET_DEFAULT, false } };

	auto PrimitiveVertexLayout = GraphicsMgr.GetGPU()->CreateVertexLayout(PrimitiveComponents, sizeof_array(PrimitiveComponents));
	_PrimitiveBuffer = GraphicsMgr.GetGPU()->CreateVertexBuffer(*PrimitiveVertexLayout, MAX_PRIMITIVE_VERTICES_PER_DIP, Render::Access_CPU_Write | Render::Access_GPU_Read);
}
//---------------------------------------------------------------------

CDebugDraw::~CDebugDraw() = default;
//---------------------------------------------------------------------

// TODO: check visibility, skip invisible
void CDebugDraw::Render(Render::CEffect& Effect, const matrix44& ViewProj)
{
	auto& GPU = *_GraphicsMgr->GetGPU();

	auto pRT = GPU.GetRenderTarget(0);
	if (!pRT) return;

	GPU.SetDepthStencilBuffer(nullptr);

	bool HasShapes = false;
	for (int i = 0; i < SHAPE_TYPE_COUNT; ++i)
	{
		if (!ShapeInsts[i].empty())
		{
			HasShapes = true;
			break;
		}
	}

	if (HasShapes)
	{
		if (auto pTech = Effect.GetTechByInputSet(CStrID("DebugShape")))
		{
			Render::CShaderParamStorage Globals(pTech->GetParamTable(), GPU);
			Globals.SetMatrix(pTech->GetParamTable().GetConstant(CStrID("ViewProj")), ViewProj);
			Globals.Apply();

			GPU.SetVertexLayout(_ShapeVertexLayout);
			GPU.SetVertexBuffer(1, _ShapeInstanceBuffer);

			const auto& Passes = pTech->GetPasses();

			for (int i = 0; i < SHAPE_TYPE_COUNT; ++i)
			{
				const auto pMesh = _Shapes[i].Get();
				n_assert(pMesh);
				const auto* pGroup = pMesh->GetGroup(0);
				n_assert(pGroup);

				GPU.SetVertexBuffer(0, pMesh->GetVertexBuffer());
				GPU.SetIndexBuffer(pMesh->GetIndexBuffer().Get());

				const UPTR ShapeCount = ShapeInsts[i].size();
				for (UPTR Offset = 0; Offset < ShapeCount; Offset += MAX_SHAPE_INSTANCES_PER_DIP)
				{
					const UPTR BatchSize = std::min(MAX_SHAPE_INSTANCES_PER_DIP, ShapeCount - Offset);

					void* pInstData;
					if (!GPU.MapResource(&pInstData, *_ShapeInstanceBuffer, Render::Map_WriteDiscard))
					{
						::Sys::Error("CDebugDraw::Render() > can't map shape instance VB!");
						return;
					}

					memcpy(pInstData, ShapeInsts[i].data() + Offset, BatchSize * sizeof(CDDShapeInst));
					GPU.UnmapResource(*_ShapeInstanceBuffer);

					for (const auto& Pass : Passes)
					{
						GPU.SetRenderState(Pass);
						GPU.DrawInstanced(*pGroup, BatchSize);
					}
				}
			}

			GPU.SetVertexBuffer(1, nullptr);
			GPU.SetIndexBuffer(nullptr);
		}

		for (int i = 0; i < SHAPE_TYPE_COUNT; ++i)
			ShapeInsts[i].clear();
	}

	if (TriVertices.Count)
	{
		if (auto pTech = Effect.GetTechByInputSet(CStrID("DebugTris")))
		{
			Render::CShaderParamStorage Globals(pTech->GetParamTable(), GPU);
			Globals.SetMatrix(pTech->GetParamTable().GetConstant(CStrID("ViewProj")), ViewProj);
			Globals.Apply();

			GPU.SetVertexLayout(_PrimitiveBuffer->GetVertexLayout());
			GPU.SetVertexBuffer(0, _PrimitiveBuffer);

			const auto& Passes = pTech->GetPasses();

			constexpr auto MAX_TRI_VERTICES_PER_DIP = (MAX_PRIMITIVE_VERTICES_PER_DIP / 3) * 3;
			static_assert(MAX_TRI_VERTICES_PER_DIP > 2);
			for (UPTR Offset = 0; Offset < TriVertices.Count; Offset += MAX_TRI_VERTICES_PER_DIP)
			{
				const UPTR BatchSize = std::min(MAX_TRI_VERTICES_PER_DIP, TriVertices.Count - Offset);

				void* pInstData;
				if (!GPU.MapResource(&pInstData, *_PrimitiveBuffer, Render::Map_WriteDiscard))
				{
					::Sys::Error("CDebugDraw::Render() > can't map primitive VB!");
					return;
				}

				memcpy(pInstData, TriVertices.pData + Offset, BatchSize * sizeof(CDDVertex));
				GPU.UnmapResource(*_PrimitiveBuffer);

				for (const auto& Pass : Passes)
				{
					GPU.SetRenderState(Pass);
					GPU.Draw({ 0, BatchSize, 0, 0, Render::Prim_TriList, {} });
				}
			}
		}

		TriVertices.Count = 0;
	}

	if (LineVertices.Count)
	{
		if (auto pTech = Effect.GetTechByInputSet(CStrID("DebugLines")))
		{
			Render::CShaderParamStorage Globals(pTech->GetParamTable(), GPU);
			Globals.SetMatrix(pTech->GetParamTable().GetConstant(CStrID("ViewProj")), ViewProj);
			Globals.SetVector(pTech->GetParamTable().GetConstant(CStrID("RTSize")),
				vector2(static_cast<float>(pRT->GetDesc().Width), static_cast<float>(pRT->GetDesc().Height)));
			Globals.Apply();

			GPU.SetVertexLayout(_PrimitiveBuffer->GetVertexLayout());
			GPU.SetVertexBuffer(0, _PrimitiveBuffer);

			const auto& Passes = pTech->GetPasses();

			constexpr auto MAX_LINE_VERTICES_PER_DIP = (MAX_PRIMITIVE_VERTICES_PER_DIP / 2) * 2;
			static_assert(MAX_LINE_VERTICES_PER_DIP > 1);
			for (UPTR Offset = 0; Offset < LineVertices.Count; Offset += MAX_LINE_VERTICES_PER_DIP)
			{
				const UPTR BatchSize = std::min(MAX_LINE_VERTICES_PER_DIP, LineVertices.Count - Offset);

				void* pInstData;
				if (!GPU.MapResource(&pInstData, *_PrimitiveBuffer, Render::Map_WriteDiscard))
				{
					::Sys::Error("CDebugDraw::Render() > can't map primitive VB!");
					return;
				}

				memcpy(pInstData, LineVertices.pData + Offset, BatchSize * sizeof(CDDVertex));
				GPU.UnmapResource(*_PrimitiveBuffer);

				for (const auto& Pass : Passes)
				{
					GPU.SetRenderState(Pass);
					GPU.Draw({ 0, BatchSize, 0, 0, Render::Prim_LineList, {} });
				}
			}
		}

		LineVertices.Count = 0;
	}

	if (Points.Count)
	{
		if (auto pTech = Effect.GetTechByInputSet(CStrID("DebugPoints")))
		{
			Render::CShaderParamStorage Globals(pTech->GetParamTable(), GPU);
			Globals.SetMatrix(pTech->GetParamTable().GetConstant(CStrID("ViewProj")), ViewProj);
			Globals.SetVector(pTech->GetParamTable().GetConstant(CStrID("RTSize")),
				vector2(static_cast<float>(pRT->GetDesc().Width), static_cast<float>(pRT->GetDesc().Height)));
			Globals.Apply();

			GPU.SetVertexLayout(_PrimitiveBuffer->GetVertexLayout());
			GPU.SetVertexBuffer(0, _PrimitiveBuffer);

			const auto& Passes = pTech->GetPasses();

			static_assert(MAX_PRIMITIVE_VERTICES_PER_DIP > 0);
			for (UPTR Offset = 0; Offset < Points.Count; Offset += MAX_PRIMITIVE_VERTICES_PER_DIP)
			{
				const UPTR BatchSize = std::min(MAX_PRIMITIVE_VERTICES_PER_DIP, Points.Count - Offset);

				void* pInstData;
				if (!GPU.MapResource(&pInstData, *_PrimitiveBuffer, Render::Map_WriteDiscard))
				{
					::Sys::Error("CDebugDraw::Render() > can't map primitive VB!");
					return;
				}

				memcpy(pInstData, Points.pData + Offset, BatchSize * sizeof(CDDVertex));
				GPU.UnmapResource(*_PrimitiveBuffer);

				for (const auto& Pass : Passes)
				{
					GPU.SetRenderState(Pass);
					GPU.Draw({ 0, BatchSize, 0, 0, Render::Prim_PointList, {} });
				}
			}
		}

		Points.Count = 0;
	}

	GPU.SetVertexBuffer(0, nullptr);
}
//---------------------------------------------------------------------

void CDebugDraw::DrawTriangle(const vector3& P1, const vector3& P2, const vector3& P3, U32 Color)
{
	AddTriangleVertex(P1, Color);
	AddTriangleVertex(P2, Color);
	AddTriangleVertex(P3, Color);
}
//---------------------------------------------------------------------

void CDebugDraw::DrawBox(const matrix44& Tfm, U32 Color)
{
	ShapeInsts[Box].push_back({ Tfm, Color });
}
//---------------------------------------------------------------------

void CDebugDraw::DrawSphere(const vector3& Pos, float R, U32 Color)
{
	ShapeInsts[Sphere].push_back(
	{
		{
			R, 0.f, 0.f, 0.f,
			0.f, R, 0.f, 0.f,
			0.f, 0.f, R, 0.f,
			Pos.x, Pos.y, Pos.z, 1.f
		},
		Color
	});
}
//---------------------------------------------------------------------

void CDebugDraw::DrawCylinder(const matrix44& Tfm, float R, float Length, U32 Color)
{
	ShapeInsts[Cylinder].push_back(
	{
		matrix44
		{
			R, 0.f, 0.f, 0.f,
			0.f, Length, 0.f, 0.f,
			0.f, 0.f, R, 0.f,
			0.f, 0.f, 0.f, 1.f
		} * Tfm,
		Color
	});
}
//---------------------------------------------------------------------

void CDebugDraw::DrawCapsule(const matrix44& Tfm, float R, float Length, U32 Color)
{
	DrawSphere(Tfm * vector3(0.0f, Length * -0.5f, 0.0f), R, Color);
	DrawSphere(Tfm * vector3(0.0f, Length * 0.5f, 0.0f), R, Color);
	DrawCylinder(Tfm, R, Length, Color);
}
//---------------------------------------------------------------------

void CDebugDraw::DrawLine(const vector3& P1, const vector3& P2, U32 Color, float Size)
{
	AddLineVertex(P1, Color, Size);
	AddLineVertex(P2, Color, Size);
}
//---------------------------------------------------------------------

void CDebugDraw::DrawBoxWireframe(const CAABB& Box, U32 Color)
{
	const vector3 mmm = Box.GetCorner(0);
	const vector3 mMm = Box.GetCorner(1);
	const vector3 MMm = Box.GetCorner(2);
	const vector3 Mmm = Box.GetCorner(3);
	const vector3 MMM = Box.GetCorner(4);
	const vector3 mMM = Box.GetCorner(5);
	const vector3 mmM = Box.GetCorner(6);
	const vector3 MmM = Box.GetCorner(7);
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
}
//---------------------------------------------------------------------

void CDebugDraw::DrawCircleXZ(const vector3& Pos, float Radius, float SegmentCount, U32 Color)
{
	const float StepInRadians = (2.f * PI) / static_cast<float>(SegmentCount);

	AddLineVertex({ 0.f, 0.f, Radius });

	for (U16 i = 1; i < SegmentCount; ++i)
	{
		const float Longitude = StepInRadians * i;
		vector3 Vertex(Radius * std::sinf(Longitude), 0.f, Radius * std::cosf(Longitude)); 
		AddLineVertex(Vertex);
		AddLineVertex(Vertex);
	}

	AddLineVertex({ 0.f, 0.f, Radius });
}
//---------------------------------------------------------------------

void CDebugDraw::DrawCoordAxes(const matrix44& Tfm, bool DrawX, bool DrawY, bool DrawZ)
{
	if (DrawX) DrawLine(Tfm.Translation(), Tfm.Translation() + Tfm.AxisX(), Render::Color_Red);
	if (DrawY) DrawLine(Tfm.Translation(), Tfm.Translation() + Tfm.AxisY(), Render::Color_Green);
	if (DrawZ) DrawLine(Tfm.Translation(), Tfm.Translation() + Tfm.AxisZ(), Render::Color_Blue);
}
//---------------------------------------------------------------------

/*
bool CDebugDraw::DrawText(const char* pText, float Left, float Top, U32 Color, float Width, bool Wrap, EHAlign HAlign, EVAlign VAlign)
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
*/

}
