#pragma once
#include <Data/Ptr.h>
#include <Data/StringID.h>
#include <IO/IOFwd.h>
#include <vector>

// Resource manager controls resource loading, lifetime, uniquity, and serves as
// a hub for accessing any types of assets in an abstract way.
// Resource ID is either an engine path to the file or an abstract string.

namespace Core
{
	class CRTTI;
}

namespace IO
{
	class CIOServer;
	typedef Ptr<class IStream> PStream;
}

namespace Resources
{
typedef Ptr<class CResource> PResource;
typedef Ptr<class IResourceCreator> PResourceCreator;

class CResourceManager final
{
protected:

	struct CDefaultCreatorRecord
	{
		CStrID				Extension;
		const Core::CRTTI*	pRsrcType;
		PResourceCreator	Creator;
	};

	IO::CIOServer*                        pIO = nullptr;
	std::unordered_map<CStrID, PResource> Registry;
	std::vector<CDefaultCreatorRecord>    DefaultCreators;

	PResourceCreator GetDefaultCreator(CStrID UID, const Core::CRTTI& RsrcType) const;

public:

	CResourceManager(IO::CIOServer* pIOServer, UPTR InitialCapacity = 256);
	CResourceManager(const CResourceManager&) = delete;
	~CResourceManager();

	CResourceManager& operator =(const CResourceManager&) = delete;

	bool				RegisterDefaultCreator(const char* pFmtExtension, const Core::CRTTI* pRsrcType, IResourceCreator* pCreator);
	PResourceCreator	GetDefaultCreator(const char* pFmtExtension, const Core::CRTTI* pRsrcType = nullptr) const;
	template<class TRsrc>
	PResourceCreator	GetDefaultCreatorFor(const char* pFmtExtension) const { return GetDefaultCreator(pFmtExtension, &TRsrc::RTTI); }

	template<class TRsrc>
	PResource			RegisterResource(const char* pUID) { return RegisterResource(pUID, TRsrc::RTTI); }
	template<class TRsrc>
	void                RegisterResource(PResource& Resource) { return RegisterResource(Resource, TRsrc::RTTI); }
	PResource			RegisterResource(const char* pUID, const Core::CRTTI& RsrcType);
	void                RegisterResource(PResource& Resource, const Core::CRTTI& RsrcType);
	PResource			RegisterResource(const char* pUID, IResourceCreator* pCreator);
	CResource*			FindResource(const char* pUID) const;
	CResource*			FindResource(CStrID UID) const;
	void				UnregisterResource(const char* pUID);
	void				UnregisterResource(CStrID UID);

	IO::PStream			CreateResourceStream(const char* pUID, const char*& pOutSubId, IO::EStreamAccessPattern Pattern = IO::SAP_DEFAULT);
};

}
