#pragma once
#ifndef __DEM_L1_RESOURCE_SERVER_H__
#define __DEM_L1_RESOURCE_SERVER_H__

#include <Resources/ResourceLoader.h>
#include <util/ndictionary.h>
#include <util/nhashmap2.h>
#include <util/HashTable.h>

// Resource wraps a piece of abstract data, used by other systems. It can be loaded and managed by
// resource system, so clients needn't to care about loading, unloading and LOD of resources.

namespace Resources
{
#define RsrcSrv Resources::CResourceServer::Instance()

class CResourceServer: public Core::CRefCounted
{
	DeclareRTTI;
	__DeclareSingleton(CResourceServer);

protected:

	static DWORD UIDCounter;

	//???force ID = Filename StrID for loaded (not created) resources
	nHashMap2<CResource*>								FileNameToResource;
	HashTable<CStrID, CResource*>						IDToResource;
	nDictionary<const Core::CRTTI*, const Core::CRTTI*> Loaders;

public:

	CResourceServer();
	~CResourceServer();

	PResource	CreateResource(const Core::CRTTI& RsrcClass, CStrID UID = CStrID::Empty);
	void		ReleaseResource(CResource* pResource);
	PResource	GetResource(const nString& FileName, const Core::CRTTI& RsrcClass, CStrID UID = CStrID::Empty, const Core::CRTTI* pLoaderClass = NULL);
	PResource	GetResource(CStrID UID);
	void		ReloadResource(PResource Resource);
	void		SetLoader(const Core::CRTTI& RsrcClass, LPCSTR Extension, const Core::CRTTI& LoaderClass);
};

typedef Ptr<CResource> PResource;

inline PResource CResourceServer::GetResource(CStrID UID)
{
	CResource** pRsrc = IDToResource.Get(UID);
	return pRsrc ? *pRsrc : NULL;
}
//---------------------------------------------------------------------

}

#endif
