#pragma once
#include <Core/RTTIBaseClass.h>
#include <Math/Matrix44.h>

// An interface class for any renderable objects, like regular models, particle systems, terrain patches etc.

namespace Render
{
using PRenderable = std::unique_ptr<class IRenderable>;
class IRenderer;

// FIXME: with fields it is a base class and not an interface!
class IRenderable: public Core::CRTTIBaseClass
{
	RTTI_CLASS_DECL(Render::IRenderable, Core::CRTTIBaseClass);

protected:

	// TODO: bitflags ShadowCaster, ShadowReceiver, DoOcclusionCulling

public:

	//???move material here? everything needs a material - texture or customization params. Or not everything?

	float DistanceToCamera = 0.f; // For LOD, distance culling, render queue sorting and custom use
	float RelScreenRadius = 0.f;  // For LOD and distance culling
	U32   RenderQueueMask = 0;    // Cached mask for fast access. Calculated from an effect type in a material.
	U32   BoundsVersion = 0;
	U16   ObjectLightIntersectionsVersion = 0;
	U16   GeometryKey = 0;        // For render queue sorting. Optimal as long as groups aren't added to existing meshes
	U16   MaterialKey = 0;        // For render queue sorting
	U8    ShaderTechKey = 0;      // For render queue sorting //???or obtain from view using ShaderTechIndex?
	U8    RendererIndex = 0;
	bool  IsVisible = false;
	bool  TrackObjectLightIntersections = false;

	//!!!DBG TMP! Check transform version before rewriting, to save unnecessary ops? Also need better representation!
	matrix44 Transform;

	//!!!DBG TMP! clean! what is renderer specific? what is renderable specific?
	U16 LightIndexBase = 0;		// Memory is actually allocated inside a CView, we store index, not ptr, to handle reallocations
	U8 LightCount = 0;			// If zero, LightIndexBase is undefined
};

}
