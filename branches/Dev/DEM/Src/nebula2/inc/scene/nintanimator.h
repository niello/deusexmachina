#ifndef N_INTANIMATOR_H
#define N_INTANIMATOR_H
//------------------------------------------------------------------------------
/**
    @class nIntAnimator
    @ingroup Scene

    @brief Animates a int attribute of a nAbstractShaderNode.

    (C) 2004 RadonLabs GmbH
*/
#include "scene/nshaderanimator.h"
#include "util/nanimkeyarray.h"

//------------------------------------------------------------------------------
class nIntAnimator : public nShaderAnimator
{
public:
    /// constructor
    nIntAnimator();
    /// destructor
    virtual ~nIntAnimator();
    /// save object to persistent stream
    //virtual bool SaveCmds(nPersistServer* ps);
    /// called by scene node objects which wish to be animated by this object
    virtual void Animate(nSceneNode* sceneNode, nRenderContext* renderContext);
    /// add a key
    void AddKey(float time, int key);
    /// get number of keys
    int GetNumKeys() const;
    /// get key at
    void GetKeyAt(int index, float& time, int& key) const;

private:
    nAnimKeyArray<nAnimKey<int> > keyArray;
};
//------------------------------------------------------------------------------
#endif
