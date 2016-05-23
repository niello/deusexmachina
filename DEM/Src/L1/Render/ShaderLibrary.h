#pragma once
#ifndef __DEM_L1_RENDER_SHADER_LIBRARY_H__
#define __DEM_L1_RENDER_SHADER_LIBRARY_H__

#include <Resources/ResourceObject.h>
#include <Data/FixedArray.h>

// Shader library is a shader storage container that provides an
// ability to create and reference shaders by numeric ID. It and
// its underlying SLB format store shaders sequentially and access
// them fast through a table of contents.

namespace IO
{
	typedef Ptr<class CStream> PStream;
}

namespace Resources
{
	class CShaderLibraryLoaderSLB;
	typedef Ptr<class CShaderLoader> PShaderLoader;
}

namespace Render
{
typedef Ptr<class CGPUDriver> PGPUDriver;
typedef Ptr<class CShader> PShader;

class CShaderLibrary: public Resources::CResourceObject
{
	__DeclareClass(CShaderLibrary);

protected:

	struct CRecord
	{
		U32		ID;
		U32		Offset;
		U32		Size;
		PShader	LoadedShader;
	};

	CFixedArray<CRecord>		TOC;			// Sorted by ID
	IO::PStream					Storage;
	Resources::PShaderLoader	ShaderLoader;

	friend class Resources::CShaderLibraryLoaderSLB;

public:

	virtual ~CShaderLibrary();

	void			SetLoader(Resources::PShaderLoader Loader);
	PShader			GetShaderByID(U32 ID);
	bool			GetRawDataByID(U32 ID, void*& pOutData, UPTR& OutSize);

	virtual bool	IsResourceValid() const { return Storage.IsValidPtr(); }
};

typedef Ptr<CShaderLibrary> PShaderLibrary;

}

#endif
