#ifndef N_LODNODE_H
#define N_LODNODE_H

#include "scene/ntransformnode.h"

// A lod node switches its child nodes on and off according to its current camera distance
// to accomplish different representations with different levels of detail.
// (C) 2002 RadonLabs GmbH

namespace Data
{
	class CBinaryReader;
}

class nLodNode: public nTransformNode
{
protected:

    nArray<float> thresholds;

public:

	float maxDistance;
	float minDistance;

	nLodNode(): minDistance(-5000.0f), maxDistance(5000.0f) {}

	virtual bool	LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader);

	virtual void	Attach(nSceneServer* sceneServer, nRenderContext* renderContext);

	void			AppendThreshold(float distance) { thresholds.Append(distance); }
	float			GetThreshold(int index) const { return thresholds[index]; }
};

#endif
