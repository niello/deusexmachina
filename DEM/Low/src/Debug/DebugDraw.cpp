#include "DebugDraw.h"
#include <Frame/GraphicsResourceManager.h>
#include <Resources/ResourceManager.h>
#include <Resources/Resource.h>
#include <Render/MeshGenerators.h>
#include <Render/Mesh.h>
#include <Render/MeshData.h>
#include <Render/Effect.h>
#include <Render/GPUDriver.h>
#include <Render/RenderTarget.h>
#include <Render/ShaderParamStorage.h>
#include <Render/VertexComponent.h>
#include <Data/Buffer.h>
#include <rtm/matrix4x4f.h>

namespace Debug
{
constexpr UPTR MAX_SHAPE_INSTANCES_PER_DIP = 1024;
constexpr UPTR MAX_PRIMITIVE_VERTICES_PER_DIP = 65536;

CDebugDraw::CDebugDraw(Frame::CGraphicsResourceManager& GraphicsMgr)
	: _GraphicsMgr(&GraphicsMgr)
{
	GraphicsMgr.GetResourceManager()->RegisterResource("#Mesh_BoxCCW", n_new(Resources::CMeshGeneratorBox()));
	auto ShpereRsrc = GraphicsMgr.GetResourceManager()->RegisterResource("#Mesh_SphereCCW12", n_new(Resources::CMeshGeneratorSphere(12)));
	if (auto pMeshData = ShpereRsrc->ValidateObject<Render::CMeshData>())
		pMeshData->UseBuffer(); // Used for drawing wireframe sphere
	GraphicsMgr.GetResourceManager()->RegisterResource("#Mesh_CylinderCCW12", n_new(Resources::CMeshGeneratorCylinder(12)));
	GraphicsMgr.GetResourceManager()->RegisterResource("#Mesh_ConeCCW12", n_new(Resources::CMeshGeneratorCone(12)));

	_Shapes[Box] = GraphicsMgr.GetMesh(CStrID("#Mesh_BoxCCW"));
	_Shapes[Sphere] = GraphicsMgr.GetMesh(CStrID("#Mesh_SphereCCW12"));
	_Shapes[Cylinder] = GraphicsMgr.GetMesh(CStrID("#Mesh_CylinderCCW12"));
	_Shapes[Cone] = GraphicsMgr.GetMesh(CStrID("#Mesh_ConeCCW12"));

	// Position, transformation matrix44, color
	Render::CVertexComponent ShapeComponents[] = {
		{ Render::EVertexComponentSemantic::Position, nullptr, 0, Render::EVertexComponentFormat::Float32_3, 0, Render::VertexComponentOffsetAuto, false },
		{ Render::EVertexComponentSemantic::TexCoord, nullptr, 4, Render::EVertexComponentFormat::Float32_4, 1, Render::VertexComponentOffsetAuto, true },
		{ Render::EVertexComponentSemantic::TexCoord, nullptr, 5, Render::EVertexComponentFormat::Float32_4, 1, Render::VertexComponentOffsetAuto, true },
		{ Render::EVertexComponentSemantic::TexCoord, nullptr, 6, Render::EVertexComponentFormat::Float32_4, 1, Render::VertexComponentOffsetAuto, true },
		{ Render::EVertexComponentSemantic::TexCoord, nullptr, 7, Render::EVertexComponentFormat::Float32_4, 1, Render::VertexComponentOffsetAuto, true },
		{ Render::EVertexComponentSemantic::Color, nullptr, 0, Render::EVertexComponentFormat::UInt8_4_Norm, 1, Render::VertexComponentOffsetAuto, true } };

	_ShapeVertexLayout = GraphicsMgr.GetGPU()->CreateVertexLayout(ShapeComponents, sizeof_array(ShapeComponents));

	// TODO: dynamically growing buffers!
	auto ShapeInstanceVertexLayout = GraphicsMgr.GetGPU()->CreateVertexLayout(ShapeComponents + 1, sizeof_array(ShapeComponents) - 1);
	_ShapeInstanceBuffer = GraphicsMgr.GetGPU()->CreateVertexBuffer(*ShapeInstanceVertexLayout, MAX_SHAPE_INSTANCES_PER_DIP, Render::Access_CPU_Write | Render::Access_GPU_Read);

	// Position with size in W, color
	Render::CVertexComponent PrimitiveComponents[] = {
		{ Render::EVertexComponentSemantic::Position, nullptr, 0, Render::EVertexComponentFormat::Float32_4, 0, Render::VertexComponentOffsetAuto, false },
		{ Render::EVertexComponentSemantic::Color, nullptr, 0, Render::EVertexComponentFormat::UInt8_4_Norm, 0, Render::VertexComponentOffsetAuto, false } };

	auto PrimitiveVertexLayout = GraphicsMgr.GetGPU()->CreateVertexLayout(PrimitiveComponents, sizeof_array(PrimitiveComponents));
	_PrimitiveBuffer = GraphicsMgr.GetGPU()->CreateVertexBuffer(*PrimitiveVertexLayout, MAX_PRIMITIVE_VERTICES_PER_DIP, Render::Access_CPU_Write | Render::Access_GPU_Read);
}
//---------------------------------------------------------------------

CDebugDraw::~CDebugDraw() = default;
//---------------------------------------------------------------------

// TODO: check visibility, skip invisible
void CDebugDraw::Render(Render::CEffect& Effect, const rtm::matrix4x4f& ViewProj)
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

					std::memcpy(pInstData, ShapeInsts[i].data() + Offset, BatchSize * sizeof(CDDShapeInst));
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

				std::memcpy(pInstData, TriVertices.pData + Offset, BatchSize * sizeof(CDDVertex));
				GPU.UnmapResource(*_PrimitiveBuffer);

				for (const auto& Pass : Passes)
				{
					GPU.SetRenderState(Pass);
					GPU.Draw({ {}, 0, BatchSize, 0, 0, Render::Prim_TriList });
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

				std::memcpy(pInstData, LineVertices.pData + Offset, BatchSize * sizeof(CDDVertex));
				GPU.UnmapResource(*_PrimitiveBuffer);

				for (const auto& Pass : Passes)
				{
					GPU.SetRenderState(Pass);
					GPU.Draw({ {}, 0, BatchSize, 0, 0, Render::Prim_LineList });
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

				std::memcpy(pInstData, Points.pData + Offset, BatchSize * sizeof(CDDVertex));
				GPU.UnmapResource(*_PrimitiveBuffer);

				for (const auto& Pass : Passes)
				{
					GPU.SetRenderState(Pass);
					GPU.Draw({ {}, 0, BatchSize, 0, 0, Render::Prim_PointList });
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

void CDebugDraw::DrawBox(const rtm::matrix3x4f& Tfm, U32 Color)
{
	ShapeInsts[Box].push_back({ rtm::matrix_cast(Tfm), Color });
}
//---------------------------------------------------------------------

void CDebugDraw::DrawSphere(const rtm::vector4f& Pos, float R, U32 Color)
{
	const rtm::matrix4x4f ObjectTfm = rtm::matrix_set(
		rtm::vector_set(R, 0.f, 0.f, 0.f),
		rtm::vector_set(0.f, R, 0.f, 0.f),
		rtm::vector_set(0.f, 0.f, R, 0.f),
		rtm::vector_set_w(Pos, 1.f));

	ShapeInsts[Sphere].push_back({ ObjectTfm, Color });
}
//---------------------------------------------------------------------

void CDebugDraw::DrawCylinder(const rtm::matrix3x4f& Tfm, float R, float Length, U32 Color)
{
	const rtm::matrix3x4f ObjectTfm = rtm::matrix_set(
		rtm::vector_set(R, 0.f, 0.f, 0.f),
		rtm::vector_set(0.f, Length, 0.f, 0.f),
		rtm::vector_set(0.f, 0.f, R, 0.f),
		rtm::vector_set(0.f, 0.f, 0.f, 1.f));

	ShapeInsts[Cylinder].push_back({ rtm::matrix_cast(rtm::matrix_mul(ObjectTfm, Tfm)), Color });
}
//---------------------------------------------------------------------

void CDebugDraw::DrawCapsule(const rtm::matrix3x4f& Tfm, float R, float Length, U32 Color)
{
	DrawSphere(rtm::matrix_mul_point3(rtm::vector_set(0.f, Length * -0.5f, 0.f), Tfm), R, Color);
	DrawSphere(rtm::matrix_mul_point3(rtm::vector_set(0.f, Length * 0.5f, 0.f), Tfm), R, Color);
	DrawCylinder(Tfm, R, Length, Color);
}
//---------------------------------------------------------------------

void CDebugDraw::DrawLine(const vector3& P1, const vector3& P2, U32 Color, float Thickness)
{
	AddLineVertex(P1, Color, Thickness);
	AddLineVertex(P2, Color, Thickness);
}
//---------------------------------------------------------------------

void CDebugDraw::DrawLine(const rtm::vector4f& P1, const rtm::vector4f& P2, U32 Color, float Thickness)
{
	AddLineVertex(P1, Color, Thickness);
	AddLineVertex(P2, Color, Thickness);
}
//---------------------------------------------------------------------

void CDebugDraw::DrawBoxWireframe(const Math::CAABB& Box, U32 Color, float Thickness)
{
	const rtm::vector4f mmm = rtm::vector_sub(Box.Center, Box.Extent);
	const rtm::vector4f MMM = rtm::vector_add(Box.Center, Box.Extent);
	const rtm::vector4f mMm = Math::vector_mix_xbzw(mmm, MMM);
	const rtm::vector4f MMm = Math::vector_mix_abzw(mmm, MMM);
	const rtm::vector4f Mmm = Math::vector_mix_ayzw(mmm, MMM);
	const rtm::vector4f mMM = Math::vector_mix_ayzw(MMM, mmm);
	const rtm::vector4f mmM = Math::vector_mix_abzw(MMM, mmm);
	const rtm::vector4f MmM = Math::vector_mix_xbzw(MMM, mmm);

	DrawLine(mmm, Mmm, Color, Thickness);
	DrawLine(mmm, mMm, Color, Thickness);
	DrawLine(mmm, mmM, Color, Thickness);
	DrawLine(MMM, MMm, Color, Thickness);
	DrawLine(MMM, mMM, Color, Thickness);
	DrawLine(MMM, MmM, Color, Thickness);
	DrawLine(mMm, mMM, Color, Thickness);
	DrawLine(mMm, MMm, Color, Thickness);
	DrawLine(mMM, mmM, Color, Thickness);
	DrawLine(Mmm, MmM, Color, Thickness);
	DrawLine(mmM, MmM, Color, Thickness);
	DrawLine(MMm, Mmm, Color, Thickness);
}
//---------------------------------------------------------------------

void CDebugDraw::DrawSphereWireframe(const vector3& Pos, float R, U32 Color, float Thickness)
{
	// NB: here we exploit internal knowledge about sphere mesh generator. Could write more robust logic later. Vertex & triangle iterators in CMeshData?
	if (auto pMeshData = _Shapes[Sphere]->GetMeshData().Get())
	{
		const auto pVertices = static_cast<vector3*>(pMeshData->VBData->GetPtr());
		const auto pIndices = static_cast<U16*>(pMeshData->IBData->GetPtr());
		for (UPTR i = 0; i < pMeshData->IndexCount; i += 3)
		{
			const vector3 v0 = pVertices[pIndices[i + 0]] * R + Pos;
			const vector3 v1 = pVertices[pIndices[i + 1]] * R + Pos;
			const vector3 v2 = pVertices[pIndices[i + 2]] * R + Pos;
			DrawLine(v0, v1, Color, Thickness);
			DrawLine(v1, v2, Color, Thickness);
			DrawLine(v2, v0, Color, Thickness);
		}
	}
}
//---------------------------------------------------------------------

void CDebugDraw::DrawFrustumWireframe(const rtm::matrix4x4f& Frustum, U32 Color, float Thickness)
{
	constexpr rtm::vector4f Corners[] =
	{
		{ -1.f, -1.f, -1.f, 1.f },
		{ -1.f, +1.f, -1.f, 1.f },
		{ +1.f, +1.f, -1.f, 1.f },
		{ +1.f, -1.f, -1.f, 1.f },
		{ -1.f, -1.f, +1.f, 1.f },
		{ -1.f, +1.f, +1.f, 1.f },
		{ +1.f, +1.f, +1.f, 1.f },
		{ +1.f, -1.f, +1.f, 1.f }
	};

	const rtm::matrix4x4f InvFrustum = rtm::matrix_inverse(Frustum);

	rtm::vector4f TransformedCorners[8];
	for (UPTR i = 0; i < 8; ++i)
	{
		const auto Tmp = rtm::matrix_mul_vector(Corners[i], InvFrustum);
		TransformedCorners[i] = rtm::vector_div(Tmp, rtm::vector_dup_w(Tmp));
	}

	// Near plane
	DrawLine(TransformedCorners[0], TransformedCorners[1], Color, Thickness);
	DrawLine(TransformedCorners[1], TransformedCorners[2], Color, Thickness);
	DrawLine(TransformedCorners[2], TransformedCorners[3], Color, Thickness);
	DrawLine(TransformedCorners[3], TransformedCorners[0], Color, Thickness);

	// Far plane
	DrawLine(TransformedCorners[4], TransformedCorners[5], Color, Thickness);
	DrawLine(TransformedCorners[5], TransformedCorners[6], Color, Thickness);
	DrawLine(TransformedCorners[6], TransformedCorners[7], Color, Thickness);
	DrawLine(TransformedCorners[7], TransformedCorners[4], Color, Thickness);

	// Connections
	DrawLine(TransformedCorners[0], TransformedCorners[4], Color, Thickness);
	DrawLine(TransformedCorners[1], TransformedCorners[5], Color, Thickness);
	DrawLine(TransformedCorners[2], TransformedCorners[6], Color, Thickness);
	DrawLine(TransformedCorners[3], TransformedCorners[7], Color, Thickness);
}
//---------------------------------------------------------------------

void CDebugDraw::DrawCircleXZ(const vector3& Pos, float Radius, float SegmentCount, U32 Color)
{
	const float StepInRadians = (2.f * PI) / static_cast<float>(SegmentCount);

	AddLineVertex(rtm::vector_set(0.f, 0.f, Radius));

	for (U16 i = 1; i < SegmentCount; ++i)
	{
		const float Longitude = StepInRadians * i;
		vector3 Vertex(Radius * std::sinf(Longitude), 0.f, Radius * std::cosf(Longitude)); 
		AddLineVertex(Vertex);
		AddLineVertex(Vertex);
	}

	AddLineVertex(rtm::vector_set(0.f, 0.f, Radius));
}
//---------------------------------------------------------------------

void CDebugDraw::DrawCoordAxes(const rtm::matrix3x4f& Tfm, bool DrawX, bool DrawY, bool DrawZ)
{
	if (DrawX) DrawLine(Tfm.w_axis, rtm::vector_add(Tfm.w_axis, Tfm.x_axis), Render::Color_Red);
	if (DrawY) DrawLine(Tfm.w_axis, rtm::vector_add(Tfm.w_axis, Tfm.y_axis), Render::Color_Green);
	if (DrawZ) DrawLine(Tfm.w_axis, rtm::vector_add(Tfm.w_axis, Tfm.z_axis), Render::Color_Blue);
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
