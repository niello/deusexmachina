#pragma once
#include <Core/RTTIBaseClass.h>

// An interface class for any renderable objects, like regular models, particle systems, terrain patches etc.

namespace Render
{
using PRenderable = std::unique_ptr<class IRenderable>;

// FIXME: with fields it is a base class and not an interface!
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

	U32  RenderQueueMask = 0; // Cached mask for fast access. Calculated from an effect type in a material.
	U32  BoundsVersion = 0;
	bool IsVisible = false;
};

}
