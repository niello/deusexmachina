#ifndef N_MATERIALNODE_H
#define N_MATERIALNODE_H
//------------------------------------------------------------------------------
/**
    @class nMaterialNode
    @ingroup Scene

    @brief A material node defines a shader resource and associated shader
    variables. A shader resource is an external file (usually a text file)
    which defines a surface shader.

    Material nodes themselves cannot render anything useful, they can just
    adjust render states as preparation of an actual rendering process.
    Thus, subclasses should be derived which implement some sort of
    shape rendering.

    - 27-Jan-05 floh    removed shader FourCC code stuff

    (C) 2002 RadonLabs GmbH
*/
#include "scene/ntransformnode.h"
#include "kernel/nref.h"
#include "gfx2/nshader2.h"

class nShader2;
class nGfxServer2;

class nMaterialNode: public nTransformNode
{
private:

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

	bool LoadShader();
    void UnloadShader();
	bool LoadTexture(int index);
    void UnloadTexture(int index);
	bool IsTextureUsed(nShaderState::Param param) { return refShader.isvalid() && refShader->IsParameterUsed(param); }

public:

	nMaterialNode(): shaderIndex(-1) {}

	virtual bool LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader);

	virtual bool LoadResources();
    virtual void UnloadResources();

	virtual bool HasShader() const { return true; }

	virtual bool ApplyShader(nSceneServer* sceneServer);
    virtual bool RenderShader(nSceneServer* sceneServer, nRenderContext* renderContext);

	virtual bool ApplyGeometry(nSceneServer* sceneServer) { return false; }
    virtual bool RenderGeometry(nSceneServer* sceneServer, nRenderContext* renderContext) { return false; }

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
};

inline nShader2* nMaterialNode::GetShaderObject()
{
	if (!AreResourcesValid()) LoadResources();
	return refShader.get_unsafe();
}
//---------------------------------------------------------------------

inline int nMaterialNode::GetShaderIndex()
{
	if (!AreResourcesValid()) LoadResources();
	return shaderIndex;
}
//---------------------------------------------------------------------

inline void nMaterialNode::SetInt(nShaderState::Param param, int val)
{
	if (nShaderState::InvalidParameter != param)
		shaderParams.SetArg(param, nShaderArg(val));
}
//---------------------------------------------------------------------

inline void nMaterialNode::SetBool(nShaderState::Param param, bool val)
{
	if (nShaderState::InvalidParameter != param)
		shaderParams.SetArg(param, nShaderArg(val));
}
//---------------------------------------------------------------------

inline void nMaterialNode::SetFloat(nShaderState::Param param, float val)
{
	if (nShaderState::InvalidParameter != param)
		shaderParams.SetArg(param, nShaderArg(val));
}
//---------------------------------------------------------------------

inline void nMaterialNode::SetVector(nShaderState::Param param, const vector4& val)
{
	if (nShaderState::InvalidParameter != param)
		shaderParams.SetArg(param, nShaderArg(val));
}
//---------------------------------------------------------------------

#endif

