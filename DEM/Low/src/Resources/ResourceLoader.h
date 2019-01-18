#pragma once
#include <Resources/ResourceCreator.h>

// A subclass of resource creators which loads resource data from a stream.
// Path is incorporated into a resource UID.

namespace IO
{
	class CIOServer;
	typedef Ptr<class CStream> PStream;
}

namespace Resources
{

class CResourceLoader: public IResourceCreator
{
protected:

	IO::CIOServer* pIO = nullptr;

	IO::PStream OpenStream(CStrID UID, const char*& pOutSubId);

public:

	CResourceLoader(IO::CIOServer* pIOServer) : pIO(pIOServer) {}

	virtual const Core::CRTTI&	GetResultType() const = 0;
	virtual PResourceObject		CreateResource(CStrID UID) = 0;
};

}
