#pragma once
#include <Scene/NodeAttribute.h>
#include <Math/SIMDMath.h>

// Camera is a scene node attribute describing camera properties.
// Note - W and H are necessary for Orthographic projection matrix,
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
		Orthographic	= 0x20	// Projection is Orthographic, not perspective
	};

	rtm::matrix3x4f _View = rtm::matrix_identity(); // InvView is node world tfm
	rtm::matrix4x4f _Proj = rtm::matrix_identity();
	rtm::matrix4x4f _InvProj = rtm::matrix_identity();
	rtm::matrix4x4f _ViewProj = rtm::matrix_identity();

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
	//???flag ManualProjMatrix? Perspective, Orthographic, Manual
	//???need BBox calculation? projection box, mul view matrix = viewproj box

	virtual bool					LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count) override;
	virtual void                    RenderDebug(Debug::CDebugDraw& DebugDraw) const override;
	virtual Scene::PNodeAttribute	Clone() override;
	virtual void					UpdateBeforeChildren(const rtm::vector4f* pCOIArray, UPTR COICount) override;

	void							GetRay3D(float RelX, float RelY, float Length, Math::CLine& OutRay) const;
	void							GetPoint2D(const rtm::vector4f& Point3D, float& OutRelX, float& OutRelY) const;

	void							SetPerspectiveMode() { if (_Flags.Is(Orthographic)) { _Flags.Clear(Orthographic); _Flags.Set(ProjDirty); } }
	void							SetOrthographicMode() { if (!_Flags.Is(Orthographic)) { _Flags.Set(Orthographic); _Flags.Set(ProjDirty); } }
	bool							IsOrthographic() const { return _Flags.Is(Orthographic); }
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
