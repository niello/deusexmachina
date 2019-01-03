#include "DEMRenderTarget.h"

#include <UI/CEGUI/DEMGeometryBuffer.h>
#include <Math/Plane.h>

namespace CEGUI
{

template <typename T>
CDEMRenderTarget<T>::CDEMRenderTarget(CDEMRenderer& owner):
	d_owner(owner),
	d_area(0.f, 0.f, 0.f, 0.f),
	d_viewDistance(0.f),
	d_matrixValid(false)
{
}
//---------------------------------------------------------------------

template <typename T>
void CDEMRenderTarget<T>::draw(const GeometryBuffer& buffer, uint32 drawModeMask)
{
	buffer.draw(drawModeMask);
}
//---------------------------------------------------------------------

template <typename T>
void CDEMRenderTarget<T>::setArea(const Rectf& area)
{
	if (d_area == area) return;

	d_area = area;
	d_matrixValid = false;

	RenderTargetEventArgs args(this);
	T::fireEvent(RenderTarget::EventAreaChanged, args);
}
//---------------------------------------------------------------------

template <typename T>
void CDEMRenderTarget<T>::activate()
{
	if (!d_matrixValid) updateMatrix();

	Render::CViewport VP;
	VP.Left = static_cast<float>(d_area.left());
	VP.Top = static_cast<float>(d_area.top());
	VP.Width = static_cast<float>(d_area.getWidth());
	VP.Height = static_cast<float>(d_area.getHeight());
	VP.MinDepth = 0.0f;
	VP.MaxDepth = 1.0f;
	d_owner.getGPUDriver()->SetViewport(0, &VP);
	d_owner.setProjMatrix(d_matrix);
}
//---------------------------------------------------------------------

template <typename T>
void CDEMRenderTarget<T>::unprojectPoint(const GeometryBuffer& buff, const Vector2f& p_in, Vector2f& p_out) const
{
	if (!d_matrixValid) updateMatrix();

	const CDEMGeometryBuffer& gb = static_cast<const CDEMGeometryBuffer&>(buff);

	vector3 p1(0.f, 0.f, 0.f), p2(1.f, 0.f, 0.f), p3(0.f, 1.f, 0.f);

	float HalfW = d_area.getWidth() * 0.5f;
	float HalfH = d_area.getHeight() * 0.5f;
	matrix44 m2 = d_matrix * (*gb.getMatrix());

	p1 = m2.transform_coord(p1);
	p1.x = d_area.left() + (1.0f + p1.x) * HalfW; 
	p1.y = d_area.top() + (1.0f - p1.y) * HalfH;

	p2 = m2.transform_coord(p2);
	p2.x = d_area.left() + (1.0f + p2.x) * HalfW; 
	p2.y = d_area.top() + (1.0f - p2.y) * HalfH;

	p3 = m2.transform_coord(p3);
	p3.x = d_area.left() + (1.0f + p3.x) * HalfW; 
	p3.y = d_area.top() + (1.0f - p3.y) * HalfH;

	plane Plane(p1, p2, p3);

	m2.invert();

	vector3 ray1(-d_area.left() / HalfW, d_area.top() / HalfH, -d_viewDistance);
	ray1 = m2.transform_coord(ray1);

	vector3 ray2((p_in.d_x - d_area.left()) / HalfW - 1.0f, 1.0f - (p_in.d_y - d_area.top()) / HalfH, 0.0f);
	ray2 = m2.transform_coord(ray1);

	float Factor = 0.f;
	line3 Ray(ray1, ray2);
	Plane.intersect(Ray, Factor);
	vector3 intersection = Ray.Start + Ray.Vector * Factor;

	p_out.d_x = intersection.x;
	p_out.d_y = intersection.y;
}
//---------------------------------------------------------------------

template <typename T>
void CDEMRenderTarget<T>::updateMatrix() const
{
	const float fov = 0.523598776f;
	const float w = d_area.getWidth();
	const float h = d_area.getHeight();
	const float aspect = w / h;
	const float midx = w * 0.5f;
	const float midy = h * 0.5f;
	d_viewDistance = midx / (aspect * 0.267949192431123f);

	// Set eye vector
	d_matrix.M41 = midx;
	d_matrix.M42 = midy;
	d_matrix.M43 = -d_viewDistance;

	vector3 at(midx, midy, 1);
	vector3 up(0, -1, 0);
	d_matrix.lookatRh(at, up);

	matrix44 Perspective;
	Perspective.perspFovRh(fov, aspect, d_viewDistance * 0.5f, d_viewDistance * 2.0f);

	d_matrix *= Perspective;

	d_matrixValid = true;
}
//---------------------------------------------------------------------

}