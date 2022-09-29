#include "DEMRenderTarget.h"

#include <Render/GPUDriver.h>
#include <UI/CEGUI/DEMGeometryBuffer.h>
#include <glm/gtc/matrix_transform.hpp>

namespace CEGUI
{

CDEMRenderTarget::CDEMRenderTarget(CDEMRenderer& owner):
	d_owner(owner)
{
}
//---------------------------------------------------------------------

void CDEMRenderTarget::activate()
{
	if (!d_matrixValid)
		updateMatrix();

	Render::CViewport VP;
	VP.Left = static_cast<float>(d_area.left());
	VP.Top = static_cast<float>(d_area.top());
	VP.Width = static_cast<float>(d_area.getWidth());
	VP.Height = static_cast<float>(d_area.getHeight());
	VP.MinDepth = 0.0f;
	VP.MaxDepth = 1.0f;
	d_owner.getGPUDriver()->SetViewport(0, &VP);

	d_owner.setViewProjectionMatrix(RenderTarget::d_matrix);

	RenderTarget::activate();
}
//---------------------------------------------------------------------

void CDEMRenderTarget::updateMatrix() const
{
	RenderTarget::updateMatrix(RenderTarget::createViewProjMatrixForDirect3D()); // FIXME: get handedness from GPU
}
//---------------------------------------------------------------------

}
