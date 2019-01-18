#pragma once
#include <Data/Ptr.h>
#include <Data/StringID.h>
#include <Data/HashTable.h>
#include <Data/Singleton.h>
#include <Core/RTTI.h>
#include <vector>

// Resource manager controls resource loading, lifetime, uniquity, and serves as
// a hub for accessing any types of assets in an abstract way.
// Resource ID is either an engine path to the file or an abstract string.

namespace Resources
{
typedef Ptr<class CResource> PResource;
typedef Ptr<class IResourceCreator> PResourceCreator;

#define ResourceMgr Resources::CResourceManager::Instance()

class CResourceManager
{
	__DeclareSingleton(CResourceManager);

protected:

	struct CDefaultCreatorRecord
	{
		CStrID				Extension;
		const Core::CRTTI*	pRsrcType;
		PResourceCreator	Creator;
		bool				ClonePerResource;	// Set to true for loaders with state which can change per-resource
	};

	CHashTable<CStrID, PResource>		Registry;
	std::vector<CDefaultCreatorRecord>	DefaultCreators;

public:

	CResourceManager(UPTR InitialCapacity = 256);
	~CResourceManager();

	bool				RegisterDefaultCreator(const char* pFmtExtension, const Core::CRTTI* pRsrcType, IResourceCreator* pCreator, bool ClonePerResource = false);
	PResourceCreator	GetDefaultCreator(const char* pFmtExtension, const Core::CRTTI* pRsrcType = nullptr);
	template<class TRsrc>
	PResourceCreator	GetDefaultCreatorFor(const char* pFmtExtension) { return CreateDefaultLoader(pFmtExtension, &TRsrc::RTTI); }

	PResource			RegisterResource(CStrID UID, IResourceCreator* pCreator = nullptr);
};

}
