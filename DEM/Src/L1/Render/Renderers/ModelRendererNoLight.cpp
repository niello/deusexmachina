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

	for (int i = 0; i < Models.Size(); /*NB: i is incremented inside*/)
	{
		CModelRecord& Rec = Models[i];

		bool NewTech = (hTech != Rec.hTech);
		bool NeedToApplyVars = false;

		if (NewTech)
		{
			if (pMaterial)
			{
				pMaterial->GetShader()->EndPass();
				pMaterial->GetShader()->End();
			}

			hTech = Rec.hTech;
			n_assert_dbg(Rec.pModel->Material->GetShader()->SetTech(hTech));
			NeedToApplyVars = true;
		}

		if (pMaterial != Rec.pModel->Material)
		{
			pMaterial = Rec.pModel->Material.get_unsafe();
			NeedToApplyVars = true;
		}

		n_assert_dbg(pMaterial);

		if (NeedToApplyVars) pMaterial->ApplyStaticVars();

		// All objects with the same material must use the same set of per-object variables
		// Since we sort by geometry, there can't be deformed and static geometry in the same set
		// Per-object vars are sorted by ID in dictionary, so order is maintained implicitly
		// Given that, we can compare per-object vars one by one if we want to find instances
		// Now I use fast instancing approach, when instancing is allowed only for objects
		// without per-object vars

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
			for (int InstIdx = i; InstIdx < j; ++InstIdx)
				*pInstData++ = Models[InstIdx].pModel->GetNode()->GetWorldMatrix();
			InstanceBuffer->Unmap();

			//!!!make sure that there are no redundant concatenations inside one frame or at least in succession!
			nArray<CVertexComponent> InstComponents = pMesh->GetVertexBuffer()->GetVertexLayout()->GetComponents();
			InstComponents.AppendArray(InstanceBuffer->GetVertexLayout()->GetComponents());
			RenderSrv->SetVertexLayout(RenderSrv->GetVertexLayout(InstComponents));
			RenderSrv->SetInstanceBuffer(1, InstanceBuffer, InstanceCount);
			InstancingIsActive = true;
		}
		else if (InstancingIsActive)
		{
			const matrix44& World = Rec.pModel->GetNode()->GetWorldMatrix();

			//!!!set to shared shader to avoid searching for handle!

			CShader::HVar hWorld = pMaterial->GetShader()->GetVarHandleByName(CStrID("World"));
			if (hWorld) pMaterial->GetShader()->SetMatrix(hWorld, World);

			//???mb set this not to shared? PPL shaders use World tfm on position, so we can
			// then mul it on VP, which is set once per frame. But some rare non-PPL shaders use WVP
			// matrix, and for this case we can try to find WVP each time tech changes and avoid
			// setting WVP when not necessary!
			CShader::HVar hWVP = pMaterial->GetShader()->GetVarHandleByName(CStrID("WorldViewProjection"));
			if (hWVP)
			{
				//!!!get VP from render srv! (if needed)
				matrix44 ViewProj;
				pMaterial->GetShader()->SetMatrix(hWVP, World * ViewProj);
			}

			RenderSrv->SetVertexLayout(pMesh->GetVertexBuffer()->GetVertexLayout());
			RenderSrv->SetInstanceBuffer(1, NULL, 0);
			InstancingIsActive = false;
		}

		i = j;

		if (NewTech)
		{
			DWORD PassCount = pMaterial->GetShader()->Begin(false); //!!!savestate!
			n_assert_dbg(PassCount == 1); //???allow multipass shaders?
			pMaterial->GetShader()->BeginPass(0);
		}
		else
		{
			pMaterial->GetShader()->CommitChanges();
			//!!!assert at least one param var set!
		}

		RenderSrv->SetVertexBuffer(0, pMesh->GetVertexBuffer());
		RenderSrv->SetIndexBuffer(pMesh->GetIndexBuffer());
		RenderSrv->SetPrimitiveGroup(pMesh->GetGroup(GroupIdx));
		RenderSrv->Draw();
	}

	Models.Clear();
}
//---------------------------------------------------------------------

}