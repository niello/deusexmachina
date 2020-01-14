#pragma once
#include <Scene/NodeAttribute.h>
#include <Scene/SceneNode.h>
#include <Math/Line.h>

// Camera is a scene node attribute describing camera properties.
// Note - W and H are necessary for orthogonal projection matrix,
// aspect ratio for a prespective projection is calculated as W / H.

namespace Frame
{
typedef Ptr<class CCameraAttribute> PCameraAttribute;

class CCameraAttribute: public Scene::CNodeAttribute
{
	FACTORY_CLASS_DECL;

protected:

	enum // extends Scene::CNodeAttribute enum
	{
		// Active
		// WorldMatrixChanged
		ProjDirty	= 0x04,	// Projection params changed, matrix should be recomputed
		Orthogonal	= 0x08	// Projection is orthogonal, not perspective
	};

	float    _FOV = n_deg2rad(60.0f);
	float    _Width = 1024.f;
	float    _Height = 768.f;
	float    _NearPlane = 0.1f;
	float    _FarPlane = 5000.f;

	matrix44 _View; // InvView is node world tfm
	matrix44 _Proj;
	matrix44 _InvProj;
	matrix44 _ViewProj;

	U32      _LastTransformVersion = 0;

public:

	static PCameraAttribute CreatePerspective(float Aspect = (16.f / 9.f), float FOV = n_deg2rad(60.0f), float NearPlane = 0.1f, float FarPlane = 5000.f);

	CCameraAttribute() { Flags.Set(ProjDirty); }

	//background color
	//???clip planes computing? or from any matrix?
	//???flag ManualProjMatrix? Perspective, Orthogonal, Manual
	//???need BBox calculation? projection box, mul view matrix = viewproj box

	virtual bool					LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count);
	virtual Scene::PNodeAttribute	Clone();
	virtual void					Update(const vector3* pCOIArray, UPTR COICount);

	void							GetRay3D(float RelX, float RelY, float Length, line3& OutRay) const;
	void							GetPoint2D(const vector3& Point3D, float& OutRelX, float& OutRelY) const;

	void							SetPerspectiveMode() { if (Flags.Is(Orthogonal)) { Flags.Clear(Orthogonal); Flags.Set(ProjDirty); } }
	void							SetOrthogonalMode() { if (!Flags.Is(Orthogonal)) { Flags.Set(Orthogonal); Flags.Set(ProjDirty); } }
	bool							IsOrthogonal() const { return Flags.Is(Orthogonal); }
	void							SetFOV(float NewFOV) { if (_FOV != NewFOV) { _FOV = NewFOV; Flags.Set(ProjDirty); } }
	float							GetFOV() const { return _FOV; }
	void							SetWidth(float W) { if (_Width != W) { _Width = W; Flags.Set(ProjDirty); } }
	float							GetWidth() const { return _Width; }
	void							SetHeight(float H) { if (_Height != H) { _Height = H; Flags.Set(ProjDirty); } }
	float							GetHeight() const { return _Height; }
	float							GetAspectRatio() const { return _Width / _Height; }
	void							SetNearPlane(float Near) { if (_NearPlane != Near) { _NearPlane = Near; Flags.Set(ProjDirty); } }
	float							GetNearPlane() const { return _NearPlane; }
	void							SetFarPlane(float Far) { if (_FarPlane != Far) { _FarPlane = Far; Flags.Set(ProjDirty); } }
	float							GetFarPlane() const { return _FarPlane; }
	const vector3&					GetPosition() const { return pNode->GetWorldPosition(); }
	const matrix44&					GetViewMatrix() const { return _View; }
	const matrix44&					GetInvViewMatrix() const { return pNode->GetWorldMatrix(); }
	const matrix44&					GetProjMatrix() const { return _Proj; }
	const matrix44&					GetInvProjMatrix() const { return _InvProj; }
	const matrix44&					GetViewProjMatrix() const { return _ViewProj; }
};

}
