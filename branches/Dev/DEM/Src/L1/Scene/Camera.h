#pragma once
#ifndef __DEM_L1_SCENE_CAMERA_H__
#define __DEM_L1_SCENE_CAMERA_H__

#include <Scene/SceneNodeAttr.h>
#include <Scene/SceneNode.h>
#include <mathlib/line.h>

// Camera is a scene node attribute describing camera properties.
// Note - W and H are necessary for orthogonal projection matrix,
// aspect ratio for a prespective projection is calculated as W / H.

namespace Scene
{

class CCamera: public CSceneNodeAttr
{
	__DeclareClass(CCamera);

protected:

	enum
	{
		ProjDirty	= 0x02,	// Projection params changed, matrix should be recomputed
		Orthogonal	= 0x04	// Projection is orthogonal, not perspective
	};

	float		FOV;
	float		Width;
	float		Height;
	float		NearPlane;
	float		FarPlane;

	matrix44	View; // InvView is node world tfm
	matrix44	Proj;
	matrix44	InvProj;
	matrix44	ViewProj;

public:

	CCamera(): FOV(n_deg2rad(60.0f)), Width(1024.f), Height(768.f), NearPlane(0.1f), FarPlane(5000.f) { Flags.Set(ProjDirty); }

	//background color
	//???clip planes computing? or from any matrix?
	//???bool IsOrtho? bool ProjMatrixWasSetDirectly?
	//???need BBox calculation? projection box, mul view matrix = viewproj box

	virtual bool	LoadDataBlock(nFourCC FourCC, IO::CBinaryReader& DataReader);
	virtual void	Update();

	void			GetRay3D(float RelX, float RelY, float Length, line3& OutRay) const;
	void			GetPoint2D(const vector3& Point3D, float& OutRelX, float& OutRelY) const;

	void			SetPerspectiveMode() { if (Flags.Is(Orthogonal)) { Flags.Clear(Orthogonal); Flags.Set(ProjDirty); } }
	void			SetOrthogonalMode() { if (!Flags.Is(Orthogonal)) { Flags.Set(Orthogonal); Flags.Set(ProjDirty); } }
	bool			IsOrthogonal() const { return Flags.Is(Orthogonal); }
	void			SetFOV(float NewFOV) { if (FOV != NewFOV) { FOV = NewFOV; Flags.Set(ProjDirty); } }
	float			GetFOV() const { return FOV; }
	void			SetWidth(float W) { if (Width != W) { Width = W; Flags.Set(ProjDirty); } }
	float			GetWidth() const { return Width; }
	void			SetHeight(float H) { if (Height != H) { Height = H; Flags.Set(ProjDirty); } }
	float			GetHeight() const { return Height; }
	float			GetAspectRatio() const { return Width / Height; }
	void			SetNearPlane(float Near) { if (NearPlane != Near) { NearPlane = Near; Flags.Set(ProjDirty); } }
	float			GetNearPlane() const { return NearPlane; }
	void			SetFarPlane(float Far) { if (FarPlane != Far) { FarPlane = Far; Flags.Set(ProjDirty); } }
	float			GetFarPlane() const { return FarPlane; }
	const vector3&	GetPosition() const { return pNode->GetWorldPosition(); }
	const matrix44&	GetViewMatrix() const { return View; }
	const matrix44&	GetInvViewMatrix() const { return pNode->GetWorldMatrix(); }
	const matrix44&	GetProjMatrix() const { return Proj; }
	const matrix44&	GetInvProjMatrix() const { return InvProj; }
	const matrix44&	GetViewProjMatrix() const { return ViewProj; }
};

__RegisterClassInFactory(CCamera);

typedef Ptr<CCamera> PCamera;

}

#endif
