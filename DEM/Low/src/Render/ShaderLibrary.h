#pragma once
#include <Resources/ResourceObject.h>
#include <Data/FixedArray.h>

// Shader library is a shader storage container that provides an
// ability to create and reference shaders by numeric ID. It and
// its underlying SLB format store shaders sequentially and access
// them fast through a table of contents.

// NB: now it is more an universal numeric-ID-based storage than a _shader_ library

namespace IO
{
	typedef Ptr<class CStream> PStream;
}

namespace Resources
{
	class CShaderLibraryLoaderSLB;
}

namespace Render
{

class CShaderLibrary: public Resources::CResourceObject
{
	__DeclareClass(CShaderLibrary);

protected:

	struct CRecord
	{
		U32		ID;
		U32		Offset;
		U32		Size;
	};

	CFixedArray<CRecord>		TOC;			// Sorted by ID
	IO::PStream					Storage;

	friend class Resources::CShaderLibraryLoaderSLB;

public:

	CShaderLibrary();
	virtual ~CShaderLibrary();

	bool			GetRawDataByID(U32 ID, void*& pOutData, UPTR& OutSize);
	IO::PStream		GetElementStream(U32 ID);

	virtual bool	IsResourceValid() const { return Storage.IsValidPtr(); }
};

typedef Ptr<CShaderLibrary> PShaderLibrary;

}