#pragma once
#ifndef __DEM_L1_RENDER_MODEL_RENDERER_H__
#define __DEM_L1_RENDER_MODEL_RENDERER_H__

#include <Render/Renderer.h>
#include <Render/Materials/Shader.h>
#include <Scene/Model.h>

// Model renderer is suited for rendering different 3D models, either static or skinned.
// It is the most common renderer, which is used when some special LOD or rendering
// techniques aren't needed.

//!!!============================================================
// Unity batch types: Background, Geometry, AlphaTest, Transparent, Overlay
// Mine batch types: Background, Solid, ATest, Alpha, Additive

// In most cases transparent water should be drawn after opaque objects but before transparent objects.
// Can solve this by CWaterRenderer, it will have its own order in frame shader

// Unity sorting: Geometry render queue optimizes the drawing order of the objects for best performance.
// All other render queues sort objects by distance, starting rendering from the furthest ones
// and ending with the closest ones.

//Particles:
//http://realtimecollisiondetection.net/blog/?p=91
//http://zeuxcg.blogspot.ru/2007/09/particle-rendering-revisited.html
//!!!additive particles can be drawn in any order!

//!!!light scissors for multipass (unified rect for all lights of the pass)!

namespace Render
{

class CModelRenderer: public IRenderer
{
	__DeclareClass(CModelRenderer);

public:

	enum { MaxLightsPerObject = 4 };

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
		Scene::CLight*	Lights[MaxLightsPerObject];
		float			LightPriorities[MaxLightsPerObject];
		DWORD			LightCount;
	};

	struct CLightRecord
	{
		Scene::CLight*	pLight;
		matrix44		Frustum;
	};

	//???sort by distance at the end? or separate sorting mode. may be good. at least for a early Z pass!
	struct CRecCmp_TechMtlGeom
	{
		// Sort tech, then material, then geometry
		inline bool operator()(const CModelRecord& R1, const CModelRecord& R2) const
		{
			if (R1.hTech == R2.hTech)
			{
				if (R1.pModel->Material.GetUnsafe() == R2.pModel->Material.GetUnsafe())
				{
					if (R1.pModel->Mesh.GetUnsafe() == R2.pModel->Mesh.GetUnsafe())
						return R1.pModel->MeshGroupIndex < R2.pModel->MeshGroupIndex;
					return R1.pModel->Mesh.GetUnsafe() < R2.pModel->Mesh.GetUnsafe();
				}
				return R1.pModel->Material.GetUnsafe() < R2.pModel->Material.GetUnsafe();
			}
			return R1.hTech < R2.hTech;
		}
	};

	struct CRecCmp_DistFtB
	{
		inline bool operator()(const CModelRecord& R1, const CModelRecord& R2) const
		{
			return R1.SqDistanceToCamera < R2.SqDistanceToCamera;
		}
	};

	struct CRecCmp_DistBtF
	{
		inline bool operator()(const CModelRecord& R1, const CModelRecord& R2) const
		{
			return R1.SqDistanceToCamera > R2.SqDistanceToCamera;
		}
	};

	PShader							Shader;
	CShaderVarMap					ShaderVars;
	CStrID							BatchType;
	DWORD							FeatFlags;
	ESortingType					DistanceSorting;
	bool							EnableLighting;

	PVertexBuffer					InstanceBuffer;
	DWORD							MaxInstanceCount;

	PShader							SharedShader;
	CShader::HVar					hLightType;
	CShader::HVar					hLightPos;
	CShader::HVar					hLightDir;
	CShader::HVar					hLightColor;
	CShader::HVar					hLightParams;

	DWORD							LightFeatFlags[MaxLightsPerObject];

	CArray<CModelRecord>			Models;
	const CArray<Scene::CLight*>*	pLights; //???!!!not ptr?!

	//???both to light?
	bool			IsModelLitByLight(Scene::CModel& Model, Scene::CLight& Light);
	float			CalcLightPriority(Scene::CModel& Model, Scene::CLight& Light);

public:

	CModelRenderer(): pLights(NULL), FeatFlags(0), DistanceSorting(Sort_None), EnableLighting(false) {}

	virtual bool	Init(const Data::CParams& Desc);
	virtual void	AddRenderObjects(const CArray<Scene::CRenderObject*>& Objects);
	virtual void	AddLights(const CArray<Scene::CLight*>& Lights);
	virtual void	Render();
};

typedef Ptr<CModelRenderer> PModelRenderer;

}

#endif
