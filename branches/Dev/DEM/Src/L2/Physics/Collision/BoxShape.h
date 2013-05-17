#pragma once
#ifndef __DEM_L2_PHYSICS_BOX_SHAPE_H__ //!!!to L1!
#define __DEM_L2_PHYSICS_BOX_SHAPE_H__

#include "Shape.h"

// Box collision shape

namespace Physics
{

class CBoxShape: public CShape
{
	__DeclareClass(CBoxShape);

private:

	vector3 Size;

public:

	CBoxShape(): CShape(Box), Size(1.0f, 1.0f, 1.0f) {}
	virtual ~CBoxShape() {}

	virtual void	Init(Data::PParams Desc);
	virtual bool	Attach(dSpaceID SpaceID);
	virtual void	RenderDebug(const matrix44& ParentTfm);

	void			SetSize(const vector3& NewSize) { n_assert(!IsAttached()); Size = NewSize; }
	const vector3&	GetSize() const { return Size; }
};

}

#endif
