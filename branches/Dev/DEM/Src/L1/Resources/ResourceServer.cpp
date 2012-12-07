#include "ResourceServer.h"

	// Lifecycle:
	// Client requests resource by FileName or some ID that can be resolved into a filename
	// RsrcMgr checks cache, and if there is this resource, loads it
	// RsrcMgr detects appropriate loader for the source provided, or client provides explicit loader class RTTI/instance
	// Created loader performs sync/async loading, changing the resource state appropriately
	// When all data is loaded, resource is initialized from it and is usable from now
	// RsrcMgr provides new resource with an unique ID, so it can be requested by it. ID is CStrID.
	//   Client can explicitly specify ID for a new resource.
	// RsrcMgr returns new PResource synchronously and client should check its state before using it.

namespace Resources
{
ImplementRTTI(Resources::CResourceServer, Core::CRefCounted);
__ImplementSingleton(CResourceServer);

DWORD CResourceServer::UIDCounter = 0;

CResourceServer::CResourceServer()
{
	__ConstructSingleton;
}
//---------------------------------------------------------------------

CResourceServer::~CResourceServer()
{
	n_assert(!IDToResource.Size());
	__DestructSingleton;
}
//---------------------------------------------------------------------

PResource CResourceServer::CreateResource(const Core::CRTTI& RsrcClass, CStrID UID)
{
	n_assert(RsrcClass.IsDerivedFrom(CResource::RTTI));

	if (!UID.IsValid())
	{
		char ID[32];
		sprintf(ID, "Rsrc%d", UIDCounter++);
		UID = CStrID(ID);
		n_assert_dbg(!IDToResource.Contains(UID));
	}

	if (IDToResource.Contains(UID)) return NULL;

	PResource New = (CResource*)CoreFct->Create(RsrcClass); //!!!can set creator function to RTTI, as in N3!
	New->UID = UID;

	IDToResource.Add(UID, New);

	return New;
}
//---------------------------------------------------------------------

void CResourceServer::ReleaseResource(CResource* pResource)
{
	n_assert2(!pResource->GetRefCount(), "CResourceServer::ReleaseResource -> Do not call manually!");
	IDToResource.Erase(pResource->GetUID());
}
//---------------------------------------------------------------------

PResource CResourceServer::GetResource(const nString& FileName, const Core::CRTTI& RsrcClass, CStrID UID, const Core::CRTTI* pLoaderClass)
{
	n_assert(FileName.IsValid());

	if (!pLoaderClass)
	{
		// try to find loader class by file extension
	}

	if (!pLoaderClass) return NULL;

	PResource New = CreateResource(RsrcClass, UID);

	if (New.isvalid())
	{
		// create loader
		// ResourceLoader->LoadResource(New, FileName);

		// or New->Load(FileName, LoaderClass);
	}

	return New;
}
//---------------------------------------------------------------------

}
