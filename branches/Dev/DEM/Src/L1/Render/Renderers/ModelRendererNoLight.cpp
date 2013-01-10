#include "ModelRendererNoLight.h"

#include <Scene/SceneNode.h>
#include <Render/RenderServer.h>

namespace Render
{
ImplementRTTI(Render::CModelRendererNoLight, Render::IModelRenderer);
ImplementFactory(Render::CModelRendererNoLight);

void CModelRendererNoLight::Render()
{
	Models.Sort<CCmpRecords>();

	CShader::HTech	hTech = NULL;
	CMaterial*		pMaterial = NULL;
	CMesh*			pMesh = NULL;
	bool			InstancingIsActive = false;
	CShader::HVar	hWorld;
	CShader::HVar	hWVP;

	for (int i = 0; i < Models.Size(); /*NB: i is incremented inside*/)
	{
		CModelRecord& Rec = Models[i];

		bool NewTech = (hTech != Rec.hTech);
		bool NeedToApplyStaticVars = false;
		bool ShaderVarsChanged = false;

		if (NewTech)
		{
			if (pMaterial)
			{
				pMaterial->GetShader()->EndPass();
				pMaterial->GetShader()->End();
			}

			hTech = Rec.hTech;
			n_assert_dbg(Rec.pModel->Material->GetShader()->SetTech(hTech));
			NeedToApplyStaticVars = true;

			// PPL shaders use World tfm on position, so we can then mul it on VP, which is
			// set once per frame. But some rare non-PPL shaders use WVP matrix, and for this
			// case we set WVP and not World.
			hWorld = pMaterial->GetShader()->GetVarHandleByName(CStrID("World"));
			hWVP = pMaterial->GetShader()->GetVarHandleByName(CStrID("WorldViewProjection"));
		}

		if (pMaterial != Rec.pModel->Material)
		{
			pMaterial = Rec.pModel->Material.get_unsafe();
			NeedToApplyStaticVars = true;
		}

		n_assert_dbg(pMaterial);

		if (NeedToApplyStaticVars)
		{
			//!!!Apply() as method to CShaderVarMap! Mb even store shader ref in it.
			for (int VarIdx = 0; VarIdx < pMaterial->GetStaticVars().Size(); ++VarIdx)
				n_assert_dbg(pMaterial->GetStaticVars().ValueAtIndex(VarIdx).Apply(*pMaterial->GetShader()));
			ShaderVarsChanged = (pMaterial->GetStaticVars().Size() > 0);
		}

		//!!!Apply() as method to CShaderVarMap! Mb even store shader ref in it.
		for (int VarIdx = 0; VarIdx < Rec.pModel->ShaderVars.Size(); ++VarIdx)
			n_assert_dbg(Rec.pModel->ShaderVars.ValueAtIndex(VarIdx).Apply(*pMaterial->GetShader()));
		ShaderVarsChanged = ShaderVarsChanged || (Rec.pModel->ShaderVars.Size() > 0);

		// All objects with the same material must use the same set of per-object variables.
		// Since we sort by geometry, there can't be deformed and static geometry in the same set.
		// Per-object vars are sorted by ID in dictionary, so order is maintained implicitly.
		// Given that, we can compare per-object vars one by one if we want to find instances.
		// Now I use fast instancing approach, when instancing is allowed only for objects
		// without per-object vars. Maybe per-variable value comparison will lead to performance
		// hit because of additional work that brings no significant effect. Needs profiling.

		CMesh* pMesh = Rec.pModel->Mesh.get_unsafe();
		DWORD GroupIdx = Rec.pModel->MeshGroupIndex;

		n_assert_dbg(pMesh);

		int j = i + 1;

		// Instance only objects without per-object vars (fast way)
		if (InstanceBuffer.isvalid() && !Rec.pModel->ShaderVars.Size())
			while (	j < Models.Size() &&
					Models[j].pModel->MeshGroupIndex == GroupIdx &&
					Models[j].pModel->Mesh.get_unsafe() == pMesh &&
					Models[j].pModel->Material.get_unsafe() == pMaterial &&
					Models[j].hTech == hTech)
				++j;

		DWORD InstanceCount = j - i;
		if (InstanceCount > 1)
		{
			matrix44* pInstData = (matrix44*)InstanceBuffer->Map(MapWriteDiscard);
			n_assert_dbg(pInstData);
			for (int InstIdx = i; InstIdx < j; ++InstIdx)
				*pInstData++ = Models[InstIdx].pModel->GetNode()->GetWorldMatrix();
			InstanceBuffer->Unmap();

			//!!!make sure that there are no redundant concatenations inside one frame or at least in succession!
			nArray<CVertexComponent> InstComponents = pMesh->GetVertexBuffer()->GetVertexLayout()->GetComponents();
			InstComponents.AppendArray(InstanceBuffer->GetVertexLayout()->GetComponents());
			RenderSrv->SetVertexLayout(RenderSrv->GetVertexLayout(InstComponents));
			RenderSrv->SetInstanceBuffer(1, InstanceBuffer, InstanceCount); //???don't hardcode instance stream index?
			InstancingIsActive = true;
		}
		else
		{
			const matrix44& World = Rec.pModel->GetNode()->GetWorldMatrix();

			if (hWorld) pMaterial->GetShader()->SetMatrix(hWorld, World);

			if (hWVP)
			{
				//!!!get VP from render srv!
				matrix44 ViewProj;
				pMaterial->GetShader()->SetMatrix(hWVP, World * ViewProj);
			}

			ShaderVarsChanged = ShaderVarsChanged || hWorld || hWVP;

			if (InstancingIsActive)
			{
				RenderSrv->SetVertexLayout(pMesh->GetVertexBuffer()->GetVertexLayout());
				RenderSrv->SetInstanceBuffer(1, NULL, 0); //???don't hardcode instance stream index?
				InstancingIsActive = false;
			}
		}

		i = j;

		if (NewTech)
		{
			DWORD PassCount = pMaterial->GetShader()->Begin(false); //!!!savestate!
			n_assert_dbg(PassCount == 1); //???allow multipass shaders?
			pMaterial->GetShader()->BeginPass(0);
		}
		else if (ShaderVarsChanged) pMaterial->GetShader()->CommitChanges();

		RenderSrv->SetVertexBuffer(0, pMesh->GetVertexBuffer());
		RenderSrv->SetIndexBuffer(pMesh->GetIndexBuffer());
		RenderSrv->SetPrimitiveGroup(pMesh->GetGroup(GroupIdx));
		RenderSrv->Draw();
	}

	Models.Clear();
}
//---------------------------------------------------------------------

}