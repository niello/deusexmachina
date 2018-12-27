#pragma once
#ifndef __DEM_L1_PHYSICS_DEBUG_DRAW_H__
#define __DEM_L1_PHYSICS_DEBUG_DRAW_H__

#include <LinearMath/btIDebugDraw.h>

// DEM implementation of the Bullet debug draw interface

namespace Physics
{

class CPhysicsDebugDraw: public btIDebugDraw
{
protected:

	int Mode;

public:

	virtual void	drawLine(const btVector3& from,const btVector3& to,const btVector3& color);
	virtual void	drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color);
	virtual void	reportErrorWarning(const char* warningString);
	virtual void	draw3dText(const btVector3& location,const char* textString);	
	virtual void	setDebugMode(int debugMode);
	virtual int		getDebugMode() const;
};

}

#endif
