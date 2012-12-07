#pragma once
#ifndef __DEM_L1_SCENE_MODEL_H__
#define __DEM_L1_SCENE_MODEL_H__

#include <Scene/SceneNodeAttr.h>
#include <Render/Materials/Material.h>
#include <Render/Geometry/Mesh.h>

//!!!OLD!
#include "gfx2/nmesh2.h"
#include "gfx2/nshader2.h"

// Mesh is a scene node attribute representing a visible shape.
// Mesh attribute references VB & IB resources, stores vertex and index range,
// material, its parameters and texture refs.

//!!!it is good to have attr for each mesh group, so separate visibility test
// is performed and shader sorting simplifies

class bbox3;

namespace Scene
{
struct CSPSRecord;

class CModel: public CSceneNodeAttr
{
	DeclareRTTI;
	DeclareFactory(CModel);

public:

//!!!OLD!
    struct CTextureNode
    {
        nShaderState::Param shaderParameter;
        nString texName;
        nRef<nTexture2> refTexture;

		CTextureNode(): shaderParameter(nShaderState::InvalidParameter) {}
		CTextureNode(nShaderState::Param shaderParam, const char* name): shaderParameter(shaderParam), texName(name) {}
    };

    nString shaderName;
    int shaderIndex;
    nRef<nShader2> refShader;

	nArray<CTextureNode> texNodeArray;
    nShaderParams shaderParams;

    nRef<nMesh2> refMesh;
    nString meshName;
	int meshUsage;
    int groupIndex;

	bool resourcesValid;

    virtual bool LoadResources();
    virtual void UnloadResources();
	bool AreResourcesValid() const { return resourcesValid; }

	bool LoadShader();
    void UnloadShader();
	bool LoadTexture(int index);
    void UnloadTexture(int index);
	bool LoadMesh();
    void UnloadMesh();

	void SetShader(const nString& name) { n_assert(name.IsValid()); shaderName = name; }
	const nString& GetShader() const { return shaderName; }
    int GetShaderIndex();
    nShader2* GetShaderObject();

    void SetInt(nShaderState::Param param, int val);
	int GetInt(nShaderState::Param param) const { return shaderParams.GetArg(param).GetInt(); }
    void SetBool(nShaderState::Param param, bool val);
	bool GetBool(nShaderState::Param param) const { return shaderParams.GetArg(param).GetBool(); }
    void SetFloat(nShaderState::Param param, float val);
	float GetFloat(nShaderState::Param param) const { return shaderParams.GetArg(param).GetFloat(); }
    void SetVector(nShaderState::Param param, const vector4& val);
	const vector4& GetVector(nShaderState::Param param) const { return shaderParams.GetArg(param).GetVector4(); }

	void SetTexture(nShaderState::Param param, const char* texName);
    const char* GetTexture(nShaderState::Param param) const;

	int GetNumParams() const { return shaderParams.GetNumValidParams(); }
	nShaderParams& GetShaderParams() { return shaderParams; }
	bool HasParam(nShaderState::Param param) { return shaderParams.IsParameterValid(param); }

	int GetNumTextures() const { return texNodeArray.Size(); }
	const char* GetTextureAt(int index) const { return texNodeArray[index].texName.Get(); }
	bool IsTextureUsed(nShaderState::Param param) { return refShader.isvalid() && refShader->IsParameterUsed(param); }

	void SetMesh(const nString& name) { n_assert(name.IsValid()); UnloadMesh(); meshName = name; }
	const nString& GetMesh() const { return meshName; }
	nMesh2* GetMeshObject() { if (!refMesh.isvalid()) { n_assert(LoadMesh()); } return refMesh.get(); }
//!!!OLD!

	Render::PMesh			Mesh;
	DWORD					MeshGroupIndex;
	Render::PMaterial		Material;
	Render::CShaderVarMap	ShaderVars;	// Animable per-object vars, also can store geom. vars like CullMode

	// ERenderFlag: ShadowCaster, ShadowReceiver, DoOcclusionCulling

	CSPSRecord*	pSPSRecord;

	CModel(): pSPSRecord(NULL), resourcesValid(false), groupIndex(0), meshUsage(nMesh2::WriteOnce) {}

	virtual bool	LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader);
	virtual void	OnRemove();
	virtual void	Update();
	void			GetBox(bbox3& OutBox) const;
};

RegisterFactory(CModel);

typedef Ptr<CModel> PModel;

//!!!OLD!
inline nShader2* CModel::GetShaderObject()
{
	if (!AreResourcesValid()) LoadResources();
	return refShader.get_unsafe();
}
//---------------------------------------------------------------------

inline int CModel::GetShaderIndex()
{
	if (!AreResourcesValid()) LoadResources();
	return shaderIndex;
}
//---------------------------------------------------------------------

inline void CModel::SetInt(nShaderState::Param param, int val)
{
	if (nShaderState::InvalidParameter != param)
		shaderParams.SetArg(param, nShaderArg(val));
}
//---------------------------------------------------------------------

inline void CModel::SetBool(nShaderState::Param param, bool val)
{
	if (nShaderState::InvalidParameter != param)
		shaderParams.SetArg(param, nShaderArg(val));
}
//---------------------------------------------------------------------

inline void CModel::SetFloat(nShaderState::Param param, float val)
{
	if (nShaderState::InvalidParameter != param)
		shaderParams.SetArg(param, nShaderArg(val));
}
//---------------------------------------------------------------------

inline void CModel::SetVector(nShaderState::Param param, const vector4& val)
{
	if (nShaderState::InvalidParameter != param)
		shaderParams.SetArg(param, nShaderArg(val));
}
//---------------------------------------------------------------------

}

#endif
