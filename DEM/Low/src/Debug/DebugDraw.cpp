#include "DebugDraw.h"
#include <Frame/GraphicsResourceManager.h>
#include <Resources/ResourceManager.h>
#include <Resources/Resource.h>
#include <Render/MeshGenerators.h>
#include <Render/Mesh.h>
#include <Render/Effect.h>
#include <Render/GPUDriver.h>
#include <Render/VertexComponent.h>

namespace Debug
{
__ImplementSingleton(CDebugDraw);

constexpr UPTR MAX_SHAPE_INSTANCES_PER_DIP = 256;

CDebugDraw::CDebugDraw(Frame::CGraphicsResourceManager& GraphicsMgr, Render::PEffect Effect)
	: _GraphicsMgr(&GraphicsMgr)
{
	__ConstructSingleton;

	_Shapes[Box] = GraphicsMgr.GetResourceManager()->RegisterResource(CStrID("#Mesh_BoxCCW"), n_new(Resources::CMeshGeneratorBox()))->ValidateObject<Render::CMesh>();
	_Shapes[Sphere] = GraphicsMgr.GetResourceManager()->RegisterResource(CStrID("#Mesh_SphereCCW12"), n_new(Resources::CMeshGeneratorSphere(12)))->ValidateObject<Render::CMesh>();
	_Shapes[Cylinder] = GraphicsMgr.GetResourceManager()->RegisterResource(CStrID("#Mesh_CylinderCCW12"), n_new(Resources::CMeshGeneratorCylinder(12)))->ValidateObject<Render::CMesh>();
	_Shapes[Cone] = GraphicsMgr.GetResourceManager()->RegisterResource(CStrID("#Mesh_ConeCCW12"), n_new(Resources::CMeshGeneratorCone(12)))->ValidateObject<Render::CMesh>();

	// Position, transformation matrix44, color
	Render::CVertexComponent ShapeComponents[] = {
		{ Render::VCSem_Position, nullptr, 0, Render::VCFmt_Float32_3, 0, DEM_VERTEX_COMPONENT_OFFSET_DEFAULT, false },
		{ Render::VCSem_TexCoord, nullptr, 4, Render::VCFmt_Float32_4, 1, DEM_VERTEX_COMPONENT_OFFSET_DEFAULT, true },
		{ Render::VCSem_TexCoord, nullptr, 5, Render::VCFmt_Float32_4, 1, DEM_VERTEX_COMPONENT_OFFSET_DEFAULT, true },
		{ Render::VCSem_TexCoord, nullptr, 6, Render::VCFmt_Float32_4, 1, DEM_VERTEX_COMPONENT_OFFSET_DEFAULT, true },
		{ Render::VCSem_TexCoord, nullptr, 7, Render::VCFmt_Float32_4, 1, DEM_VERTEX_COMPONENT_OFFSET_DEFAULT, true },
		{ Render::VCSem_Color, nullptr, 0, Render::VCFmt_Float32_4, 1, DEM_VERTEX_COMPONENT_OFFSET_DEFAULT, true } };

	_ShapeVertexLayout = GraphicsMgr.GetGPU()->CreateVertexLayout(ShapeComponents, sizeof_array(ShapeComponents));

	// TODO: dynamically growing buffers!
	auto ShapeInstanceVertexLayout = GraphicsMgr.GetGPU()->CreateVertexLayout(ShapeComponents + 1, sizeof_array(ShapeComponents) - 1);
	_ShapeInstanceBuffer = GraphicsMgr.GetGPU()->CreateVertexBuffer(*ShapeInstanceVertexLayout, MAX_SHAPE_INSTANCES_PER_DIP, Render::Access_CPU_Write | Render::Access_GPU_Read);

	// Position with size in W, color
	Render::CVertexComponent PrimitiveComponents[] = {
		{ Render::VCSem_Position, nullptr, 0, Render::VCFmt_Float32_4, 0, DEM_VERTEX_COMPONENT_OFFSET_DEFAULT, false },
		{ Render::VCSem_Color, nullptr, 0, Render::VCFmt_Float32_4, 0, DEM_VERTEX_COMPONENT_OFFSET_DEFAULT, false } };

	_PrimitiveVertexLayout = GraphicsMgr.GetGPU()->CreateVertexLayout(PrimitiveComponents, sizeof_array(PrimitiveComponents));
}
//---------------------------------------------------------------------

CDebugDraw::~CDebugDraw() { __DestructSingleton; }
//---------------------------------------------------------------------

void CDebugDraw::Render()
{
//	if (!InstanceBuffer->IsValid())
//		n_assert(InstanceBuffer->Create(InstVL, MaxShapesPerDIP, Usage_Dynamic, CPU_Write));
//
//	UPTR TotalShapeCount = 0;
//	for (int i = 0; i < ShapeCount; ++i)
//		TotalShapeCount += ShapeInsts[i].GetCount();
//
//	if (TotalShapeCount)
//	{
//		ShapeShader->SetTech(ShapeShader->GetTechByFeatures(RenderSrv->GetFeatureFlagInstanced()));
//
//		n_assert(ShapeShader->Begin(true) == 1);
//		ShapeShader->BeginPass(0);
//
//		RenderSrv->SetVertexBuffer(0, Shapes->GetVertexBuffer());
//		RenderSrv->SetVertexLayout(ShapeInstVL);
//		RenderSrv->SetIndexBuffer(Shapes->GetIndexBuffer());
//		for (int i = 0; i < ShapeCount; ++i)
//		{
//			CArray<CDDShapeInst>& Insts = ShapeInsts[i];
//			if (!Insts.GetCount()) continue;
//
//			RenderSrv->SetPrimitiveGroup(Shapes->GetGroup(i));
//
//			UPTR Remain = Insts.GetCount();
//			while (Remain > 0)
//			{
//				UPTR Count = std::min(MaxShapesPerDIP, Remain);
//				Remain -= Count;
//				void* pInstData = InstanceBuffer->Map(Map_WriteDiscard);
//				memcpy(pInstData, Insts.Begin(), Count * sizeof(CDDShapeInst));
//				InstanceBuffer->Unmap();
//
//				RenderSrv->SetInstanceBuffer(1, InstanceBuffer, Count);
//				RenderSrv->Draw();
//			}
//		}
//
//		RenderSrv->SetInstanceBuffer(1, nullptr, 0);
//
//		ShapeShader->EndPass();
//		ShapeShader->End();
//
//		for (int i = 0; i < ShapeCount; ++i)
//			ShapeInsts[i].Clear();
//	}
//
//	if (Lines.GetCount() || Tris.GetCount() || Points.GetCount())
//		RenderSrv->SetVertexLayout(PrimVL);
//
//	if (Lines.GetCount() || Tris.GetCount())
//	{
//		UPTR FeatFlagDefault = RenderSrv->ShaderFeatures.GetMask("Default");
//		ShapeShader->SetTech(ShapeShader->GetTechByFeatures(FeatFlagDefault));
//
//		n_assert(ShapeShader->Begin(true) == 1);
//		ShapeShader->BeginPass(0);
//
//		if (Tris.GetCount())
//		{
//			RenderSrv->GetD3DDevice()->DrawPrimitiveUP(D3DPT_TRIANGLELIST, Tris.GetCount() / 3, Tris.Begin(), sizeof(CDDVertex));
//			Tris.Clear();
//		}
//
//		if (Lines.GetCount())
//		{
//			RenderSrv->GetD3DDevice()->DrawPrimitiveUP(D3DPT_LINELIST, Lines.GetCount() / 2, Lines.Begin(), sizeof(CDDVertex));
//			Lines.Clear();
//		}
//
//		ShapeShader->EndPass();
//		ShapeShader->End();
//	}
//
//	if (Points.GetCount())
//	{
//		UPTR FeatFlag = RenderSrv->ShaderFeatures.GetMask("Point");
//		ShapeShader->SetTech(ShapeShader->GetTechByFeatures(FeatFlag));
//
//		n_assert(ShapeShader->Begin(true) == 1);
//		ShapeShader->BeginPass(0);
//
//		RenderSrv->GetD3DDevice()->DrawPrimitiveUP(D3DPT_POINTLIST, Points.GetCount(), Points.Begin(), sizeof(CDDVertex));
//		Points.Clear();
//
//		ShapeShader->EndPass();
//		ShapeShader->End();
//	}
}
//---------------------------------------------------------------------

void CDebugDraw::DrawTriangle(const vector3& P1, const vector3& P2, const vector3& P3, const vector4& Color)
{
	AddTriangleVertex(P1, Color);
	AddTriangleVertex(P2, Color);
	AddTriangleVertex(P3, Color);
}
//---------------------------------------------------------------------

void CDebugDraw::DrawBox(const matrix44& Tfm, const vector4& Color)
{
	ShapeInsts[Box].push_back({ Tfm, Color });
}
//---------------------------------------------------------------------

void CDebugDraw::DrawSphere(const vector3& Pos, float R, const vector4& Color)
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

void CDebugDraw::DrawCylinder(const matrix44& Tfm, float R, float Length, const vector4& Color)
{
	ShapeInsts[Sphere].push_back(
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

void CDebugDraw::DrawCapsule(const matrix44& Tfm, float R, float Length, const vector4& Color)
{
	DrawSphere(Tfm * vector3(0.0f, Length * -0.5f, 0.0f), R, Color);
	DrawSphere(Tfm * vector3(0.0f, Length * 0.5f, 0.0f), R, Color);
	DrawCylinder(Tfm, R, Length, Color);
}
//---------------------------------------------------------------------

void CDebugDraw::DrawLine(const vector3& P1, const vector3& P2, const vector4& Color)
{
	AddLineVertex(P1, Color);
	AddLineVertex(P2, Color);
}
//---------------------------------------------------------------------

void CDebugDraw::DrawBoxWireframe(const CAABB& Box, const vector4& Color)
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
}
//---------------------------------------------------------------------

void CDebugDraw::DrawCoordAxes(const matrix44& Tfm, bool DrawX, bool DrawY, bool DrawZ)
{
	if (DrawX) DrawLine(Tfm.Translation(), Tfm.Translation() + Tfm.AxisX(), vector4::Red);
	if (DrawY) DrawLine(Tfm.Translation(), Tfm.Translation() + Tfm.AxisY(), vector4::Green);
	if (DrawZ) DrawLine(Tfm.Translation(), Tfm.Translation() + Tfm.AxisZ(), vector4::Blue);
}
//---------------------------------------------------------------------

/*
bool CDebugDraw::DrawText(const char* pText, float Left, float Top, const vector4& Color, float Width, bool Wrap, EHAlign HAlign, EVAlign VAlign)
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