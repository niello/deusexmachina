#ifndef N_CAMERA2_H
#define N_CAMERA2_H
//------------------------------------------------------------------------------
/**
    @class nCamera2
    @ingroup NebulaGraphicsSystem

    Hold camera attributes for nGfxServer2.

    Technically this sets the projection matrix. The camera's position and
    orientation have to be set by the View matrix which can be done using
    nGfxServer2::SetTransform().

    (C) 2002 RadonLabs GmbH
*/

#include "kernel/ntypes.h"
#include "mathlib/matrix.h"
#include "mathlib/bbox.h"
#include "mathlib/vector.h"

//------------------------------------------------------------------------------
class nCamera2
{
public:

    /// type
    enum Type
    {
        Perspective,
        Orthogonal,
        ProjMatrixSet,
    };

    /// default constructor
    nCamera2();
    /// arg constructor
    nCamera2(float aov, float aspect, float nearp, float farp);
    /// set perspective projection parameters
    void SetPerspective(float aov, float aspect, float nearp, float farp);
    /// set orthogonal projection parameters
    void SetOrthogonal(float w, float h, float nearp, float farp);
    /// set the maya projection matrix
    void SetProjectionMatrix(const matrix44& projMatrix);
    /// set projection type
    void SetType(Type t);
    /// get projection type
    Type GetType() const;
    /// set view volume width (ortho only)
    void SetWidth(float w);
    /// get view volume width (ortho only)
    float GetWidth() const;
    /// set view volume height (ortho only)
    void SetHeight(float h);
    /// get view volume height (ortho only)
    float GetHeight() const;
    /// set angle of view in degree
    void SetAngleOfView(float a);
    /// get angle of view
    float GetAngleOfView() const;
    /// set horizontal/vertical aspect ratio (i.e. (4.0 / 3.0))
    void SetAspectRatio(float r);
    /// get aspect ratio
    float GetAspectRatio() const;
    /// set near clip plane
    void SetNearPlane(float v);
    /// get near clip plane
    float GetNearPlane() const;
    /// set far clip plane
    void SetFarPlane(float v);
    /// set far clip plane
    float GetFarPlane() const;
    /// set shadow offset, used by shadow projection matrix to move clip planes
    void SetShadowOffset(float v);
    /// get shadow offset, used by shadow projection matrix to move clip planes
    float GetShadowOffset() const;
    /// get a projection matrix representing the camera
    const matrix44& GetProjection();
    /// get the inverse of the projection
    const matrix44& GetInvProjection();
    /// get the shadow projection matrix
    const matrix44& GetShadowProjection();
    /// get a bounding box enclosing the camera
    const bbox3& GetBox();
    // get the view volume
    void GetViewVolume(bbox3& Vol) const;
    /// check if 2 view volumes intersect
    EClipStatus GetClipStatus(const matrix44& myTransform, const matrix44& otherViewProjection);

private:
    /// update the internal projection and inverse projection matrices
    void UpdateProjInvProj();

    Type type;
    float width;
    float height;
    float angleOfView;
    float vertAngleOfView;
    float aspectRatio;
    float nearPlane;
    float farPlane;
    bool projDirty;
    bool boxDirty;
    matrix44 proj;
    matrix44 invProj;
    matrix44 shadowProj;
    float shadowOffset;
    bbox3 box;
};

//------------------------------------------------------------------------------
/**
*/
inline
nCamera2::nCamera2() :
    type(Perspective),
    width(0),
    height(0),
    angleOfView(n_deg2rad(60.0f)),
    aspectRatio(4.0f / 3.0f),
    nearPlane(0.1f),
    farPlane(5000.0f),
    projDirty(true),
    boxDirty(true),
    shadowOffset(0.00007f)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
nCamera2::nCamera2(float aov, float aspect, float nearp, float farp) :
    type(Perspective),
    width(0),
    height(0),
    angleOfView(n_deg2rad(aov)),
    aspectRatio(aspect),
    nearPlane(nearp),
    farPlane(farp),
    projDirty(true),
    boxDirty(true),
    shadowOffset(0.00007f)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nCamera2::SetPerspective(float aov, float aspect, float nearp, float farp)
{
    this->type = Perspective;
    this->angleOfView = n_deg2rad(aov);
    this->aspectRatio = aspect;
    this->nearPlane = nearp;
    this->farPlane = farp;
    this->projDirty = true;
    this->boxDirty = true;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nCamera2::SetOrthogonal(float w, float h, float nearp, float farp)
{
    this->type = Orthogonal;
    this->width = w;
    this->height = h;
    this->nearPlane = nearp;
    this->farPlane = farp;
    this->projDirty = true;
    this->boxDirty = true;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nCamera2::SetProjectionMatrix(const matrix44& projMatrix)
{
    this->type = ProjMatrixSet;
    this->proj = projMatrix;
    this->shadowProj = projMatrix;
    this->projDirty = true;
    this->boxDirty = true;
    this->shadowOffset = 0.00007f;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nCamera2::SetType(Type t)
{
    this->type = t;
    this->projDirty = true;
    this->boxDirty  = true;
}

//------------------------------------------------------------------------------
/**
*/
inline
nCamera2::Type
nCamera2::GetType() const
{
    return this->type;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nCamera2::SetWidth(float w)
{
    this->width = w;
    this->projDirty = true;
    this->boxDirty  = true;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
nCamera2::GetWidth() const
{
    return this->width;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nCamera2::SetHeight(float h)
{
    this->height = h;
    this->projDirty = true;
    this->boxDirty  = true;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
nCamera2::GetHeight() const
{
    return this->height;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nCamera2::SetAngleOfView(float a)
{
    this->angleOfView = n_deg2rad(a);
    this->projDirty = true;
    this->boxDirty  = true;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
nCamera2::GetAngleOfView() const
{
    return n_rad2deg(this->angleOfView);
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nCamera2::SetAspectRatio(float r)
{
    this->aspectRatio = r;
    this->projDirty = true;
    this->boxDirty = true;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
nCamera2::GetAspectRatio() const
{
    return this->aspectRatio;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nCamera2::SetNearPlane(float v)
{
    this->nearPlane = v;
    this->projDirty = true;
    this->boxDirty = true;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
nCamera2::GetNearPlane() const
{
    return this->nearPlane;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nCamera2::SetFarPlane(float v)
{
    this->farPlane = v;
    this->projDirty = true;
    this->boxDirty = true;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
nCamera2::GetFarPlane() const
{
    return this->farPlane;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nCamera2::SetShadowOffset(float v)
{
    this->shadowOffset = v;
    this->projDirty = true;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
nCamera2::GetShadowOffset() const
{
    return this->shadowOffset;
}

//------------------------------------------------------------------------------

// x and y are relative to the near plane
inline void nCamera2::GetViewVolume(bbox3& Vol) const
{
	Vol.vmax.y = nearPlane * n_tan(angleOfView * 0.5f);
	Vol.vmin.y = -Vol.vmax.y;
	Vol.vmax.x = aspectRatio * Vol.vmax.y;
	Vol.vmin.x = -Vol.vmax.x;
	Vol.vmin.z = nearPlane;
	Vol.vmax.z = farPlane;
}

//------------------------------------------------------------------------------
/**
    Update the internal projection and inverse projection matrix
    and clear the projDirty flag.
*/
inline
void
nCamera2::UpdateProjInvProj()
{
    n_assert(this->projDirty);
    this->projDirty = false;
    if (Perspective == this->type)
    {
        this->proj.perspFovRh(this->angleOfView, this->aspectRatio, this->nearPlane, this->farPlane);
        this->shadowProj.perspFovRh(this->angleOfView, this->aspectRatio, this->nearPlane - this->shadowOffset, this->farPlane - this->shadowOffset);
    }
    else if (Orthogonal == this->type)
    {
        this->proj.orthoRh(this->width, this->height, this->nearPlane, this->farPlane);
        this->shadowProj.orthoRh(this->width, this->height, this->nearPlane - this->shadowOffset, this->farPlane - this->shadowOffset);
    }
    this->invProj = proj;
    this->invProj.invert();
}

//------------------------------------------------------------------------------
/**
    Get the projection matrix representing the camera. This will only
    recompute the matrix if it is marked as dirty. The returned matrixed
    is identical to the result of D3DXMatrixPerspectiveFovRH() (see DX9
    docs for details).
*/
inline
const matrix44&
nCamera2::GetProjection()
{
    if (this->projDirty)
    {
        this->UpdateProjInvProj();
    }
    return this->proj;
}

//------------------------------------------------------------------------------
/**
    Get the inverse of the projection matrix.
*/
inline
const matrix44&
nCamera2::GetInvProjection()
{
    if (this->projDirty)
    {
        this->UpdateProjInvProj();
    }
    return this->invProj;
}

//------------------------------------------------------------------------------
/**
    Get the shadow projection matrix.
*/
inline
const matrix44&
nCamera2::GetShadowProjection()
{
    if (this->projDirty)
    {
        this->UpdateProjInvProj();
    }
    return this->shadowProj;
}

//------------------------------------------------------------------------------
/**
    Check if 2 view volumes intersect.
*/
inline EClipStatus nCamera2::GetClipStatus(const matrix44& myTransform, const matrix44& otherViewProjection)
{
    // compute matrix which transforms my local hull into
    // projection space of the other camera
    matrix44 invProjModelViewProj = this->GetInvProjection() * myTransform * otherViewProjection;

    // compute clip code of hull
    int andFlags = 0xffff;
    int orFlags  = 0;
    int i;
    vector4 v0(0.0f, 0.0f, 0.0f, 1.0f);
    vector4 v1;
    for (i = 0; i < 8; i++)
    {
        if (i & 1) v0.x = -1.0f;
        else       v0.x = +1.0f;
        if (i & 2) v0.y = -1.0f;
        else       v0.y = +1.0f;
        if (i & 3) v0.z = 0.0f;
        else       v0.z = +1.0f;

        v1 = invProjModelViewProj * v0;
        int clip = 0;
        if (v1.x < -v1.w)       clip |= (1<<0);
        else if (v1.x > v1.w)   clip |= (1<<1);
        if (v1.y < -v1.w)       clip |= (1<<2);
        else if (v1.y > v1.w)   clip |= (1<<3);
        if (v1.z < -v1.w)       clip |= (1<<4);
        else if (v1.z > v1.w)   clip |= (1<<5);
        andFlags &= clip;
        orFlags  |= clip;
    }
    if (0 == orFlags)       return Inside;
    else if (0 != andFlags) return Outside;
    else                    return Clipped;
}

//------------------------------------------------------------------------------
/**
    Get the bounding box enclosing the view frustum defined by the camera.
*/
inline
const bbox3&
nCamera2::GetBox()
{
    if (this->boxDirty)
    {
        this->boxDirty = false;
        if (Perspective == this->type || ProjMatrixSet == this->type)
        {
            float tanAov = n_tan(this->angleOfView * 0.5f);
            this->box.vmin.z = this->nearPlane;
            this->box.vmax.z = this->farPlane;
            this->box.vmax.y = tanAov * this->farPlane;     // ??? -> tanAov * (this->farPlane - this->nearPlane);
            this->box.vmin.y = -this->box.vmax.y;
            this->box.vmax.x = this->box.vmax.y * this->aspectRatio;
            this->box.vmin.x = -this->box.vmax.x;
        }
        else
        {
            this->box.vmin.x = -this->width * 0.5f;
            this->box.vmin.y = -this->height * 0.5f;
            this->box.vmin.z = -1000000.0f;
            this->box.vmax.x = this->width * 0.5f;
            this->box.vmax.y = this->height * 0.5f;
            this->box.vmax.z = 1000000.0f;
        }
    }
    return this->box;
}

//------------------------------------------------------------------------------
#endif
