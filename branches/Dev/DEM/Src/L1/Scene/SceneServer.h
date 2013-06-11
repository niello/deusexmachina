#pragma once
#ifndef __DEM_L1_SCENE_SERVER_H__
#define __DEM_L1_SCENE_SERVER_H__

#include <Scene/Scene.h>
#include <Render/FrameShader.h>
#include <Animation/AnimClip.h>
#include <Resources/ResourceManager.h>
#include <Data/Pool.h>
#include <util/ndictionary.h>

// Scene server loads and manages 3D scenes and frame shaders for a scene rendering.
// Also scene server holds animation resource manager, which stores data, used for
// animation of scene nodes (including character bones).

namespace Scene
{
#define SceneSrv Scene::CSceneServer::Instance()

class CSceneServer: public Core::CRefCounted
{
	__DeclareSingleton(CSceneServer);

private:

	//CPool<CSceneNode>								NodePool;
	nDictionary<CStrID, Render::PFrameShader>		FrameShaders;	//???to RenderServer?
	CStrID											ScreenFrameShaderID;

public:

	Resources::CResourceManager<Anim::CAnimClip>	AnimationMgr;

	CSceneServer() { __ConstructSingleton; }
	~CSceneServer() { __DestructSingleton; }

	PSceneNode				CreateSceneNode(CScene& Scene, CStrID Name);

	//???AddFrameShader to RenderServer?
	void					AddFrameShader(CStrID ID, Render::PFrameShader FrameShader); //???or always load internally?
	void					SetScreenFrameShaderID(CStrID ID) { ScreenFrameShaderID = ID; }
	Render::CFrameShader*	GetScreenFrameShader() const;
};

inline PSceneNode CSceneServer::CreateSceneNode(CScene& Scene, CStrID Name)
{
	//return (CSceneNode*)NodePool.Construct();
	return n_new(CSceneNode)(Scene, Name);
}
//---------------------------------------------------------------------

inline void CSceneServer::AddFrameShader(CStrID ID, Render::PFrameShader FrameShader)
{
	n_assert(ID.IsValid() && FrameShader.IsValid());
	FrameShaders.Add(ID, FrameShader);
}
//---------------------------------------------------------------------

inline Render::CFrameShader* CSceneServer::GetScreenFrameShader() const
{
	int Idx = FrameShaders.FindIndex(ScreenFrameShaderID);
	return (Idx != INVALID_INDEX) ? FrameShaders.ValueAt(Idx).GetUnsafe() : NULL;
}
//---------------------------------------------------------------------

}

#endif
