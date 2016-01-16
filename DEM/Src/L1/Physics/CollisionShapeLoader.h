#pragma once
#ifndef __DEM_L1_PHYSICS_COLLISION_SHAPE_LOADER_H__
#define __DEM_L1_PHYSICS_COLLISION_SHAPE_LOADER_H__

#include <Resources/ResourceLoader.h>

// Loads a collision shape from HRD or PRM

namespace Resources
{

class CCollisionShapeLoader: public CResourceLoader
{
	__DeclareClass(CCollisionShapeLoader);

public:

	virtual ~CCollisionShapeLoader() {}

	virtual const Core::CRTTI&	GetResultType() const;
	virtual bool				IsProvidedDataValid() const { OK; } //!!!implement properly!
	virtual bool				Load(CResource& Resource);
};

typedef Ptr<CCollisionShapeLoader> PCollisionShapeLoader;

}

#endif
