#include "ModelRenderer.h"

#include <Scene/Model.h>
#include <Scene/SceneNode.h>
#include <Render/RenderServer.h>
#include <Data/Params.h>

namespace Render
{
ImplementRTTI(Render::IModelRenderer, Render::IRenderer);

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

		CModelRecord* pRec = Models.Reserve(1);
		pRec->pModel = pModel;
		pRec->FeatFlags = pModel->FeatureFlags | pModel->Material->GetFeatureFlags() | FeatFlags;
		pRec->LightCount = 0;
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