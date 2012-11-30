#ifndef N_LIGHTNODE_H
#define N_LIGHTNODE_H

#include "scene/ntransformnode.h"
#include "gfx2/nlight.h"

// Scene node which provides lighting information.
// (C) 2003 RadonLabs GmbH

class nLightNode: public nTransformNode
{
public:

	nLight Light;

	virtual bool	LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader);
	virtual bool	HasLight() const { return true; }
};

#endif



