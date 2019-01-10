#pragma once
#ifndef __DEM_L1_RENDER_RENDERABLE_H__
#define __DEM_L1_RENDER_RENDERABLE_H__

#include <Core/RTTIBaseClass.h>
#include <Data/Ptr.h>

// An interface class for any renderable objects, like regular models, particle systems, terrain patches etc.

class CAABB;

namespace IO
{
	class CBinaryReader;
}

namespace Render
{
typedef Ptr<class CGPUDriver> PGPUDriver;

class IRenderable: public Core::CRTTIBaseClass //???or CObject?
{
	__DeclareClassNoFactory;

protected:

	//enum // extends Scene::CNodeAttribute enum
	//{
	//	// Active
	//	// WorldMatrixChanged
	//	//AddedAsAlwaysVisible	= 0x04,	// To avoid searching in SPS AlwaysVisible array at each UpdateInSPS() call
	//	DoOcclusionCulling		= 0x08,
	//	CastShadow				= 0x10,
	//	ReceiveShadow			= 0x20 //???needed for some particle systems?
	//};

	//Data::CFlags Flags;

public:

	virtual ~IRenderable() {}

	virtual bool			LoadDataBlock(Data::CFourCC FourCC, IO::CBinaryReader& DataReader) = 0;
	virtual IRenderable*	Clone() = 0;
	virtual bool			GetLocalAABB(CAABB& OutBox, UPTR LOD = 0) const = 0;
	virtual bool			ValidateResources(CGPUDriver* pGPU) = 0;
};

//typedef Ptr<IRenderable> PRenderable;

}

#endif
