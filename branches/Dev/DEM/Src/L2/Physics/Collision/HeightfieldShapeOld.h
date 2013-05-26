#pragma once
#ifndef __DEM_L2_PHYS_HEIGHTFIELD_SHAPE_H__
#define __DEM_L2_PHYS_HEIGHTFIELD_SHAPE_H__

#include "Shape.h"
#include <StdDEM.h>

// Shape described by regular grid of heights

namespace Physics
{

class CHeightfieldShapeOld: public CShape
{
	__DeclareClass(CHeightfieldShapeOld);

protected:

	nString				FileName;
	float*				pHeights;
	DWORD				Width;
	DWORD				Height;
	dHeightfieldDataID	ODEHeightfieldDataID;
	dGeomID				ODEHeightfieldID;

public:

	CHeightfieldShapeOld();
	virtual ~CHeightfieldShapeOld();

	virtual void	Init(Data::PParams Desc);
	virtual bool	Attach(dSpaceID SpaceID);
	virtual void	Detach();
	virtual void	RenderDebug(const matrix44& ParentTfm);

	void			SetFileName(const nString& Name) { FileName = Name; }
	const nString&	GetFileName() const { return FileName; }
	const float*	GetHeights() const { return pHeights; }
	DWORD			GetHFWidth() const { return Width; }
	DWORD			GetHFHeight() const { return Height; }
};

}

#endif
