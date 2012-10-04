#ifndef N_VECTORANIMATOR_H
#define N_VECTORANIMATOR_H
//------------------------------------------------------------------------------
/**
    @class nVectorAnimator
    @ingroup Scene

    @brief Animates a vector attribute of an nAbstractShaderNode.

    See also @ref N2ScriptInterface_nvectoranimator

    (C) 2003 RadonLabs GmbH
*/
#include "scene/nshaderanimator.h"
#include "util/nanimkeyarray.h"

//------------------------------------------------------------------------------
class nVectorAnimator : public nShaderAnimator
{
public:
    /// constructor
    nVectorAnimator();
    /// destructor
    virtual ~nVectorAnimator();
    /// save object to persistent stream
    //virtual bool SaveCmds(nPersistServer* ps);

    /// called by scene node objects which wish to be animated by this object
    virtual void Animate(nSceneNode* sceneNode, nRenderContext* renderContext);
    /// add a key
    void AddKey(float time, const vector4& key);
    /// get number of keys
    int GetNumKeys() const;
    /// get key at
    void GetKeyAt(int index, float& time, vector4& key) const;

private:
    nAnimKeyArray<nAnimKey<vector4> > keyArray;
};

//------------------------------------------------------------------------------
#endif

