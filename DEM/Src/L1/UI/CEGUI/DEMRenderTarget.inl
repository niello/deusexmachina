#include "DEMRenderTarget.h"
#include "DEMGeometryBuffer.h"
/*
namespace CEGUI
{

template <typename T>
CDEMRenderTarget<T>::CDEMRenderTarget(Direct3D11Renderer& owner):
	d_owner(owner),
	d_device(d_owner.getDirect3DDevice()),
	d_area(0, 0, 0, 0),
	d_viewDistance(0),
	d_matrixValid(false)
{
}
//---------------------------------------------------------------------

template <typename T>
void CDEMRenderTarget<T>::setArea(const Rectf& area)
{
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

	D3D11_VIEWPORT vp;
	setupViewport(vp);
	d_device.d_context->RSSetViewports(1, &vp);

	d_owner.setProjectionMatrix(d_matrix);
}
//---------------------------------------------------------------------

template <typename T>
void CDEMRenderTarget<T>::unprojectPoint(const GeometryBuffer& buff, const Vector2f& p_in, Vector2f& p_out) const
{
    if (!d_matrixValid) updateMatrix();

    const Direct3D11GeometryBuffer& gb = static_cast<const Direct3D11GeometryBuffer&>(buff);

    D3D11_VIEWPORT vp_;
    setupViewport(vp_);

	//ToDo
	//ms didn't update their math for 11, so use 10
	D3D10_VIEWPORT vp;
	vp.Width=static_cast<unsigned int>(vp_.Width);
	vp.Height=static_cast<unsigned int>(vp_.Height);
	vp.TopLeftX=static_cast<int>(vp_.TopLeftX);
	vp.TopLeftY=static_cast<int>(vp_.TopLeftY);
	vp.MinDepth=vp_.MinDepth;
	vp.MaxDepth=vp_.MaxDepth;

    D3DXVECTOR3 in_vec;
    in_vec.z = 0.0f;

    // project points to create a plane orientated with GeometryBuffer's data
    D3DXVECTOR3 p1;
    D3DXVECTOR3 p2;
    D3DXVECTOR3 p3;
    in_vec.x = 0;
    in_vec.y = 0;
    D3DXVec3Project(&p1, &in_vec, &vp, &d_matrix, 0, gb.getMatrix()); 

    in_vec.x = 1;
    in_vec.y = 0;
    D3DXVec3Project(&p2, &in_vec, &vp, &d_matrix, 0, gb.getMatrix()); 

    in_vec.x = 0;
    in_vec.y = 1;
    D3DXVec3Project(&p3, &in_vec, &vp, &d_matrix, 0, gb.getMatrix()); 

    // create plane from projected points
    D3DXPLANE surface_plane;
    D3DXPlaneFromPoints(&surface_plane, &p1, &p2, &p3);

    // unproject ends of ray
    in_vec.x = vp.Width * 0.5f;
    in_vec.y = vp.Height * 0.5f;
    in_vec.z = -d_viewDistance;
    D3DXVECTOR3 t1;
    D3DXVec3Unproject(&t1, &in_vec, &vp, &d_matrix, 0, gb.getMatrix()); 

    in_vec.x = p_in.d_x;
    in_vec.y = p_in.d_y;
    in_vec.z = 0.0f;
    D3DXVECTOR3 t2;
    D3DXVec3Unproject(&t2, &in_vec, &vp, &d_matrix, 0, gb.getMatrix()); 

    // get intersection of ray and plane
    D3DXVECTOR3 intersect;
    D3DXPlaneIntersectLine(&intersect, &surface_plane, &t1, &t2);

    p_out.d_x = intersect.x;
    p_out.d_y = intersect.y;
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

    D3DXVECTOR3 eye(midx, midy, -d_viewDistance);
    D3DXVECTOR3 at(midx, midy, 1);
    D3DXVECTOR3 up(0, -1, 0);

    D3DXMATRIX tmp;
    D3DXMatrixMultiply(&d_matrix,
        D3DXMatrixLookAtRH(&d_matrix, &eye, &at, &up),
        D3DXMatrixPerspectiveFovRH(&tmp, fov, aspect,
                                   d_viewDistance * 0.5f,
                                   d_viewDistance * 2.0f));

    d_matrixValid = false;
}
//---------------------------------------------------------------------

template <typename T>
void CDEMRenderTarget<T>::setupViewport(D3D11_VIEWPORT& vp) const
{
	vp.TopLeftX = static_cast<FLOAT>(d_area.left());
	vp.TopLeftY = static_cast<FLOAT>(d_area.top());
	vp.Width = static_cast<FLOAT>(d_area.getWidth());
	vp.Height = static_cast<FLOAT>(d_area.getHeight());
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
}
//---------------------------------------------------------------------

}
*/