#pragma once
#include <Scene/NodeAttribute.h>
#include <Math/SIMDMath.h>

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
		ProjDirty	= 0x10,	// Projection params changed, matrix should be recomputed
		Orthogonal	= 0x20	// Projection is orthogonal, not perspective
	};

	rtm::matrix3x4f _View; // InvView is node world tfm
	rtm::matrix4x4f _Proj;
	rtm::matrix4x4f _InvProj;
	rtm::matrix4x4f _ViewProj;

	float _FOV = n_deg2rad(60.0f);
	float _Width = 1024.f;
	float _Height = 768.f;
	float _NearPlane = 0.1f;
	float _FarPlane = 5000.f;

	U32 _LastTransformVersion = 0;

public:

	static PCameraAttribute CreatePerspective(float Aspect = (16.f / 9.f), float FOV = n_deg2rad(60.0f), float NearPlane = 0.1f, float FarPlane = 5000.f);

	CCameraAttribute() { _Flags.Set(ProjDirty); }

	//background color
	//???clip planes computing? or from any matrix?
	//???flag ManualProjMatrix? Perspective, Orthogonal, Manual
	//???need BBox calculation? projection box, mul view matrix = viewproj box

	virtual bool					LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count) override;
	virtual void                    RenderDebug(Debug::CDebugDraw& DebugDraw) const override;
	virtual Scene::PNodeAttribute	Clone() override;
	virtual void					UpdateBeforeChildren(const rtm::vector4f* pCOIArray, UPTR COICount) override;

	void							GetRay3D(float RelX, float RelY, float Length, Math::CLine& OutRay) const;
	void							GetPoint2D(const rtm::vector4f& Point3D, float& OutRelX, float& OutRelY) const;

	void							SetPerspectiveMode() { if (_Flags.Is(Orthogonal)) { _Flags.Clear(Orthogonal); _Flags.Set(ProjDirty); } }
	void							SetOrthogonalMode() { if (!_Flags.Is(Orthogonal)) { _Flags.Set(Orthogonal); _Flags.Set(ProjDirty); } }
	bool							IsOrthogonal() const { return _Flags.Is(Orthogonal); }
	void							SetFOV(float NewFOV) { if (_FOV != NewFOV) { _FOV = NewFOV; _Flags.Set(ProjDirty); } }
	float							GetFOV() const { return _FOV; }
	void							SetWidth(float W) { if (_Width != W) { _Width = W; _Flags.Set(ProjDirty); } }
	float							GetWidth() const { return _Width; }
	void							SetHeight(float H) { if (_Height != H) { _Height = H; _Flags.Set(ProjDirty); } }
	float							GetHeight() const { return _Height; }
	float							GetAspectRatio() const { return _Width / _Height; }
	void							SetNearPlane(float Near) { if (_NearPlane != Near) { _NearPlane = Near; _Flags.Set(ProjDirty); } }
	float							GetNearPlane() const { return _NearPlane; }
	void							SetFarPlane(float Far) { if (_FarPlane != Far) { _FarPlane = Far; _Flags.Set(ProjDirty); } }
	float							GetFarPlane() const { return _FarPlane; }
	const rtm::vector4f&			GetPosition() const;
	const rtm::matrix3x4f&			GetViewMatrix() const { return _View; }
	const rtm::matrix3x4f&			GetInvViewMatrix() const;
	const rtm::matrix4x4f&			GetProjMatrix() const { return _Proj; }
	const rtm::matrix4x4f&			GetInvProjMatrix() const { return _InvProj; }
	const rtm::matrix4x4f&			GetViewProjMatrix() const { return _ViewProj; }
};

}
