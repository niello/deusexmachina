#pragma once
#include <Core/RTTIBaseClass.h>

// An interface class for any renderable objects, like regular models, particle systems, terrain patches etc.

namespace Render
{
typedef std::unique_ptr<class IRenderable> PRenderable;

class IRenderable: public Core::CRTTIBaseClass
{
	RTTI_CLASS_DECL(Render::IRenderable, Core::CRTTIBaseClass);

protected:

	//enum //???not all flags applicable to all renderables?
	//{
	//	//AddedAsAlwaysVisible	= 0x04,	// To avoid searching in SPS AlwaysVisible array at each UpdateInSPS() call
	//	DoOcclusionCulling		= 0x08,
	//	CastShadow				= 0x10,
	//	ReceiveShadow			= 0x20 //???needed for some particle systems?
	//};

	//Data::CFlags Flags;

public:

};

}
