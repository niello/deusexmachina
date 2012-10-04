#ifndef N_LIGHTNODE_H
#define N_LIGHTNODE_H
//------------------------------------------------------------------------------
/**
    @class nLightNode
    @ingroup Scene
    @brief Scene node which provides lighting information.

    NOTE: nLightNode is derived from nAbstractShaderNode, and holds most
    light parameters inside shader params. This is in order to enable
    simple animation of light parameters using existing animators.

    (C) 2003 RadonLabs GmbH
*/
#include "scene/nabstractshadernode.h"
#include "gfx2/nlight.h"

namespace Data
{
	class CBinaryReader;
}

//------------------------------------------------------------------------------
class nLightNode : public nAbstractShaderNode
{
public:

	virtual bool LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader);

	/// return true if node provides lighting information
    virtual bool HasLight() const;
    /// set per-light states
    virtual const nLight& ApplyLight(nSceneServer* sceneServer, nRenderContext* renderContext, const matrix44& lightTransform, const vector4& shadowLightMask);
    /// set per-instance light states
    virtual const nLight& RenderLight(nSceneServer* sceneServer, nRenderContext* renderContext, const matrix44& lightTransform);
    /// set light type
    void SetType(nLight::Type t);
    /// get light type
    nLight::Type GetType() const;
    /// set cast shadows
    void SetCastShadows(bool b);
    /// set cast shadows
    bool GetCastShadows() const;
    /// get embedded light object
    const nLight& GetLight() const;

private:
    nLight light;
};

//------------------------------------------------------------------------------
/**
*/
inline
void
nLightNode::SetType(nLight::Type t)
{
    this->light.SetType(t);
}

//------------------------------------------------------------------------------
/**
*/
inline
nLight::Type
nLightNode::GetType() const
{
    return this->light.GetType();
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nLightNode::SetCastShadows(bool b)
{
    this->light.SetCastShadows(b);
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nLightNode::GetCastShadows() const
{
    return this->light.GetCastShadows();
}

//------------------------------------------------------------------------------
/**
*/
inline
const nLight&
nLightNode::GetLight() const
{
    return this->light;
}

//------------------------------------------------------------------------------
#endif



