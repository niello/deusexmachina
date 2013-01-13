#pragma once
#ifndef __DEM_L1_RENDER_MODEL_RENDERER_H__
#define __DEM_L1_RENDER_MODEL_RENDERER_H__

#include <Render/Renderer.h>
#include <Render/Materials/Shader.h>
#include <Scene/Model.h>

// Model renderer is an abstract class for different model renderers. This is intended
// for different lighting implementations.

namespace Render
{

class IModelRenderer: public IRenderer
{
	DeclareRTTI;

protected:

	enum ESortingType
	{
		Sort_None,
		Sort_FrontToBack,
		Sort_BackToFront
	};

	struct CModelRecord
	{
		Scene::CModel*	pModel;
		DWORD			FeatFlags;
		CShader::HTech	hTech;
		float			SqDistanceToCamera;
	};

	struct CRecCmp_TechMtlGeom
	{
		// Sort tech, then material, then geometry
		inline bool operator()(const CModelRecord& R1, const CModelRecord& R2) const
		{
			//!!!sort by distance if required!

			if (R1.hTech == R2.hTech)
			{
				if (R1.pModel->Material.get_unsafe() == R2.pModel->Material.get_unsafe())
				{
					if (R1.pModel->Mesh.get_unsafe() == R2.pModel->Mesh.get_unsafe())
						return R1.pModel->MeshGroupIndex < R2.pModel->MeshGroupIndex;
					return R1.pModel->Mesh.get_unsafe() < R2.pModel->Mesh.get_unsafe();
				}

				return R1.pModel->Material.get_unsafe() < R2.pModel->Material.get_unsafe();
			}

			return (int)R1.hTech < (int)R2.hTech;
		}
	};

	struct CRecCmp_DistFtB
	{
		inline bool operator()(const CModelRecord& R1, const CModelRecord& R2) const
		{
			return (int)R1.SqDistanceToCamera < (int)R2.SqDistanceToCamera;
		}
	};

	struct CRecCmp_DistBtF
	{
		inline bool operator()(const CModelRecord& R1, const CModelRecord& R2) const
		{
			return (int)R1.SqDistanceToCamera > (int)R2.SqDistanceToCamera;
		}
	};

	PShader							Shader;
	CShaderVarMap					ShaderVars;
	CStrID							BatchType;
	DWORD							FeatFlags;
	ESortingType					DistanceSorting;

	nArray<CModelRecord>			Models;
	const nArray<Scene::CLight*>*	pLights;

	PVertexBuffer					InstanceBuffer;
	DWORD							MaxInstanceCount;

public:

	IModelRenderer(): pLights(NULL), FeatFlags(0), DistanceSorting(Sort_None) {}

	virtual bool Init(const Data::CParams& Desc);
	virtual void AddRenderObjects(const nArray<Scene::CRenderObject*>& Objects);
	virtual void AddLights(const nArray<Scene::CLight*>& Lights);
};

typedef Ptr<IModelRenderer> PModelRenderer;

}

#endif
