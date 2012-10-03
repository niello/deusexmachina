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
void
nLodNode::Attach(nSceneServer* sceneServer, nRenderContext* renderContext)
{
    n_assert(sceneServer);
    n_assert(renderContext);

    if (this->CheckFlags(Active))
    {
        // get camera distance
        const matrix44& viewer = nGfxServer2::Instance()->GetTransform(nGfxServer2::InvView);

        // get global render context position
        matrix44 transform = renderContext->GetTransform();

        // compute position by stepping up node hierarchy (Argh)
        /*
        nTransformNode* tNode = this;
        while ((tNode = (nTransformNode*) tNode->GetParent()) && tNode->IsA(this->transformNodeClass))
        {
            transform = transform * tNode->GetTransform();
        }
        */

        vector3 lodViewer = viewer.pos_component() - transform.pos_component();
        float distance = lodViewer.len();

        if ((distance > this->minDistance) && (distance < this->maxDistance))
        {

            // get number of child nodes
            int num = 0;
            nSceneNode* curChild;
            for (curChild = (nSceneNode*)this->GetHead();
                curChild;
                curChild = (nSceneNode*)curChild->GetSucc())
            {
                if (curChild->IsA(this->transformNodeClass))
                {
                    num++;
                }
            }

            // if there are not enough thresholds, set some default values
            if (!this->thresholds.Size())
            {
                thresholds.Append(100.0f);
            }
            while (this->thresholds.Size() < (num - 1))
            {
                this->thresholds.Append(thresholds[this->thresholds.Size()-1] * 2.0f);
            }

            // find the proper child to attach
            int index = 0;
            if (this->GetHead())
            {
                nSceneNode* childToAttach = (nSceneNode*)this->GetHead();
                for (curChild = (nSceneNode*)childToAttach->GetSucc();
                    curChild;
                    curChild = (nSceneNode*)curChild->GetSucc(), index++)
                {
                    if (curChild->IsA(this->transformNodeClass))
                    {
                        if (distance >= this->thresholds[index])
                        {
                            childToAttach = curChild;
                        }
                    }
                }
                childToAttach->Attach(sceneServer, renderContext);
            }
        }
    }
}
