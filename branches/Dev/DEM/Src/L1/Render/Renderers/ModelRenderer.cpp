#include "ModelRenderer.h"

#include <Scene/Model.h>
#include <Scene/SceneNode.h>
#include <Render/RenderServer.h>
#include <Data/Params.h>

namespace Render
{
ImplementRTTI(Render::IModelRenderer, Render::IRenderer);

// Forward rendering:
// - Render solid objects to depth buffer, front to back (only if render to texture?)
// - Render atest objects to depth buffer, front to back (only if render to texture?)
// - Occlusion (against z-buffer filled by 1 and 2)
// - Render sky without zwrite and mb without ztest //???better to render sky after all other non-alpha/additive geometry?
// - Render terrain (lightmapped/unlit/...?) FTB //???render after all opaque except skybox?
// - Render opaque geometry (static, skinned, blended, envmapped) FTB
// - Render alpha-tested geometry (static, leaf, tree) FTB
// - Render alpha-blended geometry (alpha, alpha_soft, skinned_alpha, env_alpha, water) BTF
// - Render particles (alpha, then additive) BTF?
// - HDR

bool IModelRenderer::Init(const Data::CParams& Desc)
{
	CStrID ShaderID = Desc.Get(CStrID("Shader"), CStrID::Empty);
	if (ShaderID.IsValid())
	{
		Shader = RenderSrv->ShaderMgr.GetTypedResource(ShaderID);
		if (!Shader->IsLoaded()) FAIL;
	}

	//!!!DUPLICATE CODE!+
	ShaderVars.BeginAdd();

	//???to shadervarmap method?
	Data::CParam* pPrm;
	if (Desc.Get(pPrm, CStrID("ShaderVars")))
	{
		Data::CParams& Vars = *pPrm->GetValue<Data::PParams>();
		for (int i = 0; i < Vars.GetCount(); ++i)
		{
			Data::CParam& PrmVar = Vars.Get(i);
			CShaderVar& Var = ShaderVars.Add(PrmVar.GetName());
			Var.SetName(PrmVar.GetName());
			Var.Value = PrmVar.GetRawValue();
		}
	}

	//???to shadervarmap method?
	if (Desc.Get(pPrm, CStrID("Textures"))) //!!!can use string vars in main block instead!
	{
		Data::CParams& Vars = *pPrm->GetValue<Data::PParams>();
		for (int i = 0; i < Vars.GetCount(); ++i)
		{
			Data::CParam& PrmVar = Vars.Get(i);
			CShaderVar& Var = ShaderVars.Add(PrmVar.GetName());
			Var.SetName(PrmVar.GetName());
			Var.Value = RenderSrv->TextureMgr.GetTypedResource(CStrID(PrmVar.GetValue<nString>().Get()));
		}
	}

	ShaderVars.EndAdd();
	//!!!DUPLICATE CODE!-

	BatchType = Desc.Get<CStrID>(CStrID("BatchType"));
	n_assert(BatchType.IsValid());

	FeatFlags = RenderSrv->ShaderFeatureStringToMask(Desc.Get<nString>(CStrID("FeatFlags"), NULL));

	nString SortType;
	if (Desc.Get<nString>(SortType, CStrID("Sort")))
	{
		SortType = SortType.Trim(N_WHITESPACE);
		SortType.ToLower();
		if (SortType == "fronttoback" || SortType == "ftb")
			DistanceSorting = Sort_FrontToBack;
		else if (SortType == "backtofront" || SortType == "btf")
			DistanceSorting = Sort_BackToFront;
		else DistanceSorting = Sort_None;
	}

	//???add InitialInstanceCount + AllowGrowInstanceBuffer or MaxInstanceCount or both?
	MaxInstanceCount = Desc.Get<int>(CStrID("MaxInstanceCount"), 0);
	if (MaxInstanceCount)
	{
		nArray<CVertexComponent> InstCmps(4, 0);
		for (int i = 0; i < 4; ++i)
		{
			CVertexComponent& Cmp = InstCmps.At(i);
			Cmp.Format = CVertexComponent::Float4;
			Cmp.Semantic = CVertexComponent::TexCoord;
			Cmp.Index = i + 4; // TEXCOORD 4, 5, 6, 7 are used
			Cmp.Stream = 1;
		}
		InstanceBuffer.Create();
		n_assert(InstanceBuffer->Create(RenderSrv->GetVertexLayout(InstCmps), MaxInstanceCount, UsageDynamic, AccessWrite));
	}

	OK;
}
//---------------------------------------------------------------------

void IModelRenderer::AddRenderObjects(const nArray<Scene::CRenderObject*>& Objects)
{
	for (int i = 0; i < Objects.Size(); ++i)
	{
		if (!Objects[i]->IsA(Scene::CModel::RTTI)) continue;
		Scene::CModel* pModel = (Scene::CModel*)Objects[i];

		n_assert_dbg(pModel->BatchType.IsValid());
		if (pModel->BatchType != BatchType) continue;

		//!!!in light renderers must collect lights here and add Ln feat flags to models!

		CModelRecord* pRec = Models.Reserve(1);
		pRec->pModel = pModel;
		pRec->FeatFlags = pModel->FeatureFlags | pModel->Material->GetFeatureFlags() | FeatFlags;
		pRec->hTech = pModel->Material->GetShader()->GetTechByFeatures(pRec->FeatFlags);
		n_assert(pRec->hTech);
	}

	if (DistanceSorting != Sort_None && Models.Size() > 1)
	{
		vector3 EyePos = RenderSrv->GetCameraPosition();
		for (int i = 0; i < Models.Size(); ++i)
		{
			CModelRecord& Rec = Models[i];
			Rec.SqDistanceToCamera = vector3::SqDistance(Rec.pModel->GetNode()->GetWorldMatrix().pos_component(), EyePos);
		}
	}
}
//---------------------------------------------------------------------

// NB: It is overriden to empty method in CModelRendererNoLight
void IModelRenderer::AddLights(const nArray<Scene::CLight*>& Lights)
{
	pLights = &Lights;
	//for (int i = 0; i < Lights.Size(); ++i)
	//{
	//	// Perform something with lights or just store array ref
	//}
}
//---------------------------------------------------------------------

}