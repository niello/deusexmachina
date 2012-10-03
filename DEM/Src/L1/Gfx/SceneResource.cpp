#include "SceneResource.h"

#include <Gfx/GfxServer.h>
#include <Data/Streams/FileStream.h>
#include <Data/BinaryReader.h>
#include <kernel/nkernelserver.h>
#include <scene/ntransformnode.h>

namespace Load
{
	bool SceneFromSCN(const nString& FileName, nSceneNode*& Out, LPCSTR RootName, int DataBlockCount = -1);
}

namespace Graphics
{
ImplementRTTI(Graphics::CSceneResource, Core::CRefCounted);
ImplementFactory(Graphics::CSceneResource);

void CSceneResource::Load()
{
	n_assert(Name.IsValid());
	n_assert(!IsLoaded());

	nArray<nString> Tokens;
	Name.Tokenize("/", Tokens);
	n_assert(Tokens.Size() >= 2);

	nString CatName = Tokens[0];
	nString ObjName = Tokens[1];

	// ----------------------------------------------------------------------------
	// FIXME : special case handling for characters
	//         should be generalized to support more than 1 subdirectory
	nString CatDir = CatName;
	nString NebulaPath = Name;
	bool Char3Mode = false;
	if ((Tokens.Size() == 3) && (CatName == "characters") && (Tokens[2] == "skeleton"))
	{
		// this seems to be a character
		CatDir = Tokens[0] + "/" + Tokens[1];
		ObjName = Tokens[2];
		CatName = Tokens[1] + "_" + Tokens[2];
		Tokens.Clear();
		Tokens.Append(CatName);
		Tokens.Append(ObjName);
		NebulaPath = CatName + "/" + ObjName;
		Char3Mode = true;
	}
	// ----------------------------------------------------------------------------

	// First check if the resource is already in memory
	nRoot* RsrcPool = GfxSrv->GetGfxRoot();
	nKernelServer::Instance()->PushCwd(RsrcPool);
	refNode = (nTransformNode*)nKernelServer::Instance()->Lookup(NebulaPath.Get());
	nKernelServer::Instance()->PopCwd();
	if (refNode.isvalid())
	{
		n_assert(!refRootNode.isvalid());
		refNode->AddRef();
		if (Char3Mode) refRootNode = refNode;
		else
		{
			nString NodeName = Tokens[0] + "/" + Tokens[1];
			nKernelServer::Instance()->PushCwd(RsrcPool);
			refRootNode = nKernelServer::Instance()->Lookup(NodeName.Get());
			nKernelServer::Instance()->PopCwd();
		}
		n_assert(refRootNode.isvalid());
		refRootNode->AddRef();
	}
    else
    {
		// Create category node if necessary
		nRoot* pCat = NULL;
		if (CatName.IsValid())
		{
			pCat = RsrcPool->Find(CatName.Get());
			if (!pCat)
			{
				nKernelServer::Instance()->PushCwd(RsrcPool);
				pCat = nKernelServer::Instance()->New("nroot", CatName.Get());
				nKernelServer::Instance()->PopCwd();
			}
		}

		// Make sure root node doesn't exist, this may happen if the
		// same graphics object is loaded without using sub-nodes
		nString NodeName = Tokens[0] + "/" + Tokens[1];
		while (true)
		{
			nKernelServer::Instance()->PushCwd(RsrcPool);
			nRoot* pOldRoot = nKernelServer::Instance()->Lookup(NodeName.Get());
			nKernelServer::Instance()->PopCwd();
			if (pOldRoot) pOldRoot->Release();
			else break;
		}

        // Try to load Nebula object
        char FName[N_MAXPATH];
        if (pCat)
        {
            snprintf(FName, sizeof(FName), "gfxlib:%s/%s.scn", CatDir.Get(), ObjName.Get());
            nKernelServer::Instance()->PushCwd(pCat);
        }
        else snprintf(FName, sizeof(FName), "gfxlib:%s.scn", ObjName.Get());

		nSceneNode* pObj;
		if (!Load::SceneFromSCN(FName, pObj, ObjName.Get())) pObj = NULL;

		if (pCat) nKernelServer::Instance()->PopCwd();

		if (pObj)
		{
			refRootNode = pObj;
			refNode = (nTransformNode*)pObj;

			// Resolve deep hierarchy node if needed
			for (int i = 2; refNode.isvalid() && i < Tokens.Size(); ++i)
				refNode = (nTransformNode*)refNode->Find(Tokens[i].Get());
		}

		if (!refNode.isvalid()) n_error("CSceneResource: Failed to load resource '%s'\n", Name.Get());
		refNode->PreloadResources();
	}
}
//---------------------------------------------------------------------

void CSceneResource::Unload()
{
	n_assert(refNode.isvalid());
	refNode->Release();
	refNode.invalidate();
	if (refRootNode.isvalid())
	{
		refRootNode->Release();
		refRootNode.invalidate();
	}
}
//---------------------------------------------------------------------

} // namespace Graphics
