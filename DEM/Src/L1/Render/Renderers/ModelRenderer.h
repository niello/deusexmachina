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

	struct CModelRecord
	{
		Scene::CModel*	pModel;
		DWORD			FeatFlags;
		CShader::HTech	hTech;
	};

	struct CCmpRecords
	{
		// Sort tech, then material, then geometry
		inline bool operator() (const CModelRecord& R1, const CModelRecord& R2) const
		{
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

	CStrID							BatchType;
	nArray<CModelRecord>			Models;
	const nArray<Scene::CLight*>*	pLights;
	PVertexBuffer					InstanceBuffer;

public:

	IModelRenderer(): pLights(NULL) {}

	virtual void Init(const Data::CParams& Desc);

	virtual void AddRenderObjects(const nArray<Scene::CRenderObject*>& Objects);
	virtual void AddLights(const nArray<Scene::CLight*>& Lights);
};

typedef Ptr<IModelRenderer> PModelRenderer;

}

#endif
