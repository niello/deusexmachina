#ifndef N_ANIMATOR_H
#define N_ANIMATOR_H

#include "scene/nscenenode.h"
#include "variable/nvariable.h"
#include "util/nanimlooptype.h"

// Animator nodes manipulate properties of other scene objects.
// They are not attached to the scene, instead they are called back by scene
// objects which wish to be manipulated.
// (C) 2003 RadonLabs GmbH

class nVariableServer;

namespace Data
{
	class CBinaryReader;
}

class nAnimator : public nSceneNode
{
protected:

	nVariable::Handle	HChannel;
	nVariable::Handle	HChannelOffset;

public:

	enum Type
	{
		InvalidType,    // an invalid type
		Transform,      // a transform animator
		Shader,         // a shader animator
		BlendShape,     // a blend shape animator
	};

	nAnimLoopType::Type	LoopType;

	nAnimator();

	virtual bool		LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader);

	virtual void		Animate(nSceneNode* sceneNode, nRenderContext* renderContext) {}

	void				SetChannel(const char* name);
	const char*			GetChannel();
	virtual Type		GetAnimatorType() const { return InvalidType; }
};

#endif
