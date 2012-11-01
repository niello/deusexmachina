//------------------------------------------------------------------------------
//  nlodnode_main.cc
//  (C) 2004 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "scene/nlodnode.h"
#include "scene/nsceneserver.h"
#include "gfx2/ngfxserver2.h"
#include "scene/nrendercontext.h"
#include <Data/BinaryReader.h>

nNebulaClass(nLodNode, "ntransformnode");

//------------------------------------------------------------------------------
/**
*/
nLodNode::nLodNode() :
    minDistance(-5000.0f),
    maxDistance(5000.0f)
{
    this->transformNodeClass = nKernelServer::Instance()->FindClass("ntransformnode");
}

bool nLodNode::LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader)
{
	switch (FourCC)
	{
		case 'HSRT': // TRSH
		{
			short Count;
			if (!DataReader.Read(Count)) FAIL;
			for (short i = 0; i < Count; ++i)
				AppendThreshold(DataReader.Read<float>());
			OK;
		}
		case 'NIMD': // DMIN
		{
			SetMinDistance(DataReader.Read<float>());
			OK;
		}
		case 'XAMD': // DMAX
		{
			SetMaxDistance(DataReader.Read<float>());
			OK;
		}
		default: return nTransformNode::LoadDataBlock(FourCC, DataReader);
	}
}
//---------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
    Attach to the scene server.
    FIXME FLOH: NOTE, this method will only be correct if this node and
    its parent nodes are not animated.
*/
void nLodNode::Attach(nSceneServer* sceneServer, nRenderContext* renderContext)
{
    n_assert(sceneServer);
    n_assert(renderContext);

    if (!CheckFlags(Active) || !GetHead()) return;

	const vector3& viewer = nGfxServer2::Instance()->GetTransform(nGfxServer2::InvView).pos_component();
    vector3 lodViewer = viewer - renderContext->GetTransform().pos_component();
    float distance = lodViewer.len();

    if (distance <= minDistance || distance >= maxDistance) return;

	// get number of child nodes
    int num = 0;
    for (nSceneNode* pChild = (nSceneNode*)GetHead(); pChild; pChild = (nSceneNode*)pChild->GetSucc())
        if (pChild->IsA(transformNodeClass))
            ++num;

    // if there are not enough thresholds, set some default values
    if (!thresholds.Size()) thresholds.Append(100.0f);

	while (thresholds.Size() < (num - 1))
        thresholds.Append(thresholds[thresholds.Size()-1] * 2.0f);

    // find the proper child to attach
    int i = 0;
    nSceneNode* pSelChild = (nSceneNode*)GetHead();
	nSceneNode* pChild = (nSceneNode*)pSelChild->GetSucc();
    for (; pChild; pChild = (nSceneNode*)pChild->GetSucc(), ++i)
        if (pChild->IsA(transformNodeClass) && distance >= thresholds[i])
                pSelChild = pChild;
    pSelChild->Attach(sceneServer, renderContext);
}
