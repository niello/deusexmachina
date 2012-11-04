#include "scene/ncameranode.h"
#include "mathlib/polar.h"
#include "kernel/nkernelserver.h"

nNebulaClass(nCameraNode, "nabstractcameranode");

bool nCameraNode::RenderCamera(const matrix44& ModelWorld, const matrix44& /*View*/, const matrix44& Proj)
{
	// projectionmatrix
	projMatrix = Proj;

	// viewmatrix
	polar2 viewerAngles;

	vector3 _cameraDirection = ModelWorld.z_component();
	_cameraDirection.norm();

	viewerAngles.set(_cameraDirection);
	viewerAngles.theta -= N_PI * 0.5f;

	viewMatrix.ident();
	viewMatrix.rotate_x(viewerAngles.theta);
	viewMatrix.rotate_y(viewerAngles.rho);

	// initialize point from camera
	vector3 _cameraPosition;
	viewMatrix.mult(ModelWorld.pos_component(), _cameraPosition);
	viewMatrix.pos_component() = _cameraPosition;

	return true;
}
//---------------------------------------------------------------------
