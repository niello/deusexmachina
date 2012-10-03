#pragma once
#ifndef __DEM_L2_PHYSICS_GFX_UTIL_H__
#define __DEM_L2_PHYSICS_GFX_UTIL_H__

#include <Core/Ptr.h>
#include <util/narray.h>
#include <util/nstring.h>

// Helper class which knows how to connect complex physics composites to graphics entities.
// Based on PhysicsGfxUtil_(C) 2005 Radon Labs GmbH

//!!!move methods to GfxServer or smth!

namespace Physics
{
	class CEntity;
	class CComposite;
	class CRagdoll;
}

namespace Graphics
{
class CCharEntity;
typedef Ptr<class CShapeEntity> PShapeEntity;

typedef nArray<Graphics::PShapeEntity> CGfxShapeArray;

class CPhysicsGfxUtil
{
public:

	static void CreateGraphics(Physics::CEntity* pPhysEntity, CGfxShapeArray& OutEntities);
	static bool SetupGraphics(const nString& resourceName, Physics::CEntity* pPhysEntity, const CGfxShapeArray& GfxEntities);
	static bool UpdateGraphicsTransforms(Physics::CEntity* pPhysEntity, const CGfxShapeArray& GfxEntities);

private:

	static void CreateCompositeGraphics(Physics::CComposite* pComp, CGfxShapeArray& OutEntities);
	static bool SetupCompositeGraphics(const nString& resourceName, Physics::CComposite* pComp, const CGfxShapeArray& GfxEntities);
	static bool TransferCompositeTransforms(Physics::CComposite* pComp, const CGfxShapeArray& GfxEntities);

	static bool SetupRagdollGraphics(const nString& resourceName, Physics::CEntity* pPhysEntity, Graphics::CCharEntity* pGfxEntity);
	static bool TransferRagdollTransforms(Physics::CEntity* pPhysEntity, Graphics::CCharEntity* pGfxEntity);
};

}

#endif
