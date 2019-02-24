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
		updateMatrix(RenderTarget::createViewProjMatrixForDirect3D()); // FIXME: get handedness from GPU

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

void CDEMRenderTarget::unprojectPoint(const GeometryBuffer& buff, const glm::vec2& p_in, glm::vec2& p_out) const
{
	if (!d_matrixValid)
		updateMatrix(RenderTarget::createViewProjMatrixForDirect3D()); // FIXME: get handedness from GPU

	const CDEMGeometryBuffer& gb = static_cast<const CDEMGeometryBuffer&>(buff);

	const I32 vp[4] = {
		static_cast<I32>(RenderTarget::d_area.left()),
		static_cast<I32>(RenderTarget::d_area.top()),
		static_cast<I32>(RenderTarget::d_area.getWidth()),
		static_cast<I32>(RenderTarget::d_area.getHeight())
	};

	float in_x, in_y = 0.0f, in_z = 0.0f;

	glm::ivec4 viewPort = glm::ivec4(vp[0], vp[1], vp[2], vp[3]);
	const glm::mat4& projMatrix = RenderTarget::d_matrix;
	const glm::mat4& modelMatrix = gb.getModelMatrix();

	// unproject the ends of the ray
	glm::vec3 unprojected1;
	glm::vec3 unprojected2;
	in_x = vp[2] * 0.5f;
	in_y = vp[3] * 0.5f;
	in_z = -RenderTarget::d_viewDistance;
	unprojected1 =  glm::unProject(glm::vec3(in_x, in_y, in_z), modelMatrix, projMatrix, viewPort);
	in_x = p_in.x;
	in_y = vp[3] - p_in.y;
	in_z = 0.0;
	unprojected2 = glm::unProject(glm::vec3(in_x, in_y, in_z), modelMatrix, projMatrix, viewPort);

	// project points to orientate them with GeometryBuffer plane
	glm::vec3 projected1;
	glm::vec3 projected2;
	glm::vec3 projected3;
	in_x = 0.0;
	in_y = 0.0;
	projected1 = glm::project(glm::vec3(in_x, in_y, in_z), modelMatrix, projMatrix, viewPort);
	in_x = 1.0;
	in_y = 0.0;
	projected2 = glm::project(glm::vec3(in_x, in_y, in_z), modelMatrix, projMatrix, viewPort);
	in_x = 0.0;
	in_y = 1.0;
	projected3 = glm::project(glm::vec3(in_x, in_y, in_z), modelMatrix, projMatrix, viewPort);

	// calculate vectors for generating the plane
	const glm::vec3 pv1 = projected2 - projected1;
	const glm::vec3 pv2 = projected3 - projected1;
	// given the vectors, calculate the plane normal
	const glm::vec3 planeNormal = glm::cross(pv1, pv2);
	// calculate plane
	const glm::vec3 planeNormalNormalized = glm::normalize(planeNormal);
	const double pl_d = - glm::dot(projected1, planeNormalNormalized);
	// calculate vector of picking ray
	const glm::vec3 rv = unprojected1 - unprojected2;
	// calculate intersection of ray and plane
	const double pn_dot_r1 = glm::dot(unprojected1, planeNormal);
	const double pn_dot_rv = glm::dot(rv, planeNormal);
	const double tmp1 = pn_dot_rv != 0.0 ? (pn_dot_r1 + pl_d) / pn_dot_rv : 0.0;
	const double is_x = unprojected1.x - rv.x * tmp1;
	const double is_y = unprojected1.y - rv.y * tmp1;

	p_out.x = static_cast<float>(is_x);
	p_out.y = static_cast<float>(is_y);
}
//---------------------------------------------------------------------

}