#include "scene/noverlookcameranode.h"
#include "kernel/nkernelserver.h"

nNebulaClass(nOverlookCameraNode, "nabstractcameranode");

bool nOverlookCameraNode::RenderCamera(const matrix44& modelWorldMatrix, const matrix44& /*viewMatrix*/, const matrix44& projectionMatrix)
{
    // projectionmatrix
    projMatrix = projectionMatrix;

    // viewmatrix
	viewMatrix.ident();
	viewMatrix.rotate_x(N_PI * 0.5f);

    vector3 _cameraPosition;
    viewMatrix.mult(-modelWorldMatrix.pos_component(), _cameraPosition);
    viewMatrix.pos_component() = _cameraPosition;

    return true;
}
//---------------------------------------------------------------------
