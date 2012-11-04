#ifndef N_ABSTRACTCAMERANODE_H
#define N_ABSTRACTCAMERANODE_H

#include "scene/ntransformnode.h"
#include "renderpath/nrprendertarget.h"

// A scene camera node can render the current scene from a different camera into a render target.
// (C) 2005 RadonLabs GmbH

class nAbstractCameraNode: public nTransformNode
{
protected:

	matrix44	viewMatrix;
	matrix44	projMatrix;

	void SetViewMatrix(const matrix44& m) { viewMatrix = m; }
	void SetProjectionMatrix(const matrix44& m) { projMatrix = m; }

public:

	nString		RenderPathSection;

	virtual ~nAbstractCameraNode() { UnloadResources(); }

	virtual bool	HasCamera() const { return true; }
	virtual bool	RenderCamera(const matrix44& ModelWorld, const matrix44& View, const matrix44& Proj) { return false; }

	const matrix44&	GetViewMatrix() const { return viewMatrix; }
	const matrix44&	GetProjectionMatrix() const { return projMatrix; }
};

#endif
