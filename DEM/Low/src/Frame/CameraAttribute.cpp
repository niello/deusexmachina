#include "CameraAttribute.h"
#include <Scene/SceneNode.h>
#include <Debug/DebugDraw.h>
#include <Math/CameraMath.h>
#include <Core/Factory.h>

namespace Frame
{
FACTORY_CLASS_IMPL(Frame::CCameraAttribute, 'CAMR', Scene::CNodeAttribute);

PCameraAttribute CCameraAttribute::CreatePerspective(float Aspect, float FOV, float NearPlane, float FarPlane)
{
	Frame::PCameraAttribute Camera = n_new(Frame::CCameraAttribute());
	Camera->SetWidth(Aspect);
	Camera->SetHeight(1.f);
	Camera->SetNearPlane(NearPlane);
	Camera->SetFarPlane(FarPlane);
	return Camera;
}
//---------------------------------------------------------------------

bool CCameraAttribute::LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count)
{
	/*
	for (UPTR j = 0; j < Count; ++j)
	{
		const uint32_t Code = DataReader.Read<uint32_t>();
		switch (Code)
		{
			case 'XXXX':
			{
				DataReader.Read();
				break;
			}
			default: FAIL;
		}
	}

	OK;
	*/

	return !Count;
}
//---------------------------------------------------------------------

void CCameraAttribute::RenderDebug(Debug::CDebugDraw& DebugDraw) const
{
	DebugDraw.DrawFrustumWireframe(Math::FromSIMD(_ViewProj), Render::ColorRGBA(128, 0, 255, 255), 2.f);
}
//---------------------------------------------------------------------

Scene::PNodeAttribute CCameraAttribute::Clone()
{
	PCameraAttribute ClonedAttr = n_new(CCameraAttribute());
	ClonedAttr->_FOV = _FOV;
	ClonedAttr->_Width = _Width;
	ClonedAttr->_Height = _Height;
	ClonedAttr->_NearPlane = _NearPlane;
	ClonedAttr->_FarPlane = _FarPlane;
	ClonedAttr->_Flags.SetTo(Orthogonal, IsOrthogonal());
	return ClonedAttr;
}
//---------------------------------------------------------------------

void CCameraAttribute::UpdateBeforeChildren(const rtm::vector4f* pCOIArray, UPTR COICount)
{
	bool ViewOrProjChanged = false;

	if (_Flags.Is(ProjDirty))
	{
		if (_Flags.Is(Orthogonal))
			_Proj.orthoRh(_Width, _Height, _NearPlane, _FarPlane);
		else
			_Proj.perspFovRh(_FOV, _Width / _Height, _NearPlane, _FarPlane);

		// Shadow proj was calculated with:
		//nearPlane - shadowOffset, farPlane - shadowOffset, shadowOffset(0.00007f)

		_InvProj = rtm::matrix_inverse(_Proj);

		_Flags.Clear(ProjDirty);
		ViewOrProjChanged = true;
	}

	if (_pNode->GetTransformVersion() != _LastTransformVersion)
	{
		_View = rtm::matrix_inverse(_pNode->GetWorldMatrix());
		_LastTransformVersion = _pNode->GetTransformVersion();
		ViewOrProjChanged = true;
	}

	if (ViewOrProjChanged) _ViewProj = rtm::matrix_mul(rtm::matrix_cast(_View), _Proj);
}
//---------------------------------------------------------------------

void CCameraAttribute::GetRay3D(float RelX, float RelY, float Length, Math::CLine& OutRay) const
{
	constexpr rtm::vector4f SignMask = { 1.f, -1.f, 1.f, 1.f };
	const rtm::vector4f ScreenCoord3D = rtm::vector_set((RelX - 0.5f) * 2.0f, (RelY - 0.5f) * 2.0f, 1.0f);
	const rtm::vector4f ViewLocalPos = rtm::vector_mul(rtm::vector_mul(rtm::matrix_mul_vector(ScreenCoord3D, _InvProj), _NearPlane * 1.1f), SignMask);
	const rtm::matrix3x4f& InvView = GetInvViewMatrix();
	OutRay.Start = rtm::matrix_mul_point3(ViewLocalPos, InvView);
	OutRay.Dir = rtm::vector_sub(OutRay.Start, InvView.w_axis);
	OutRay.Dir = rtm::vector_mul(OutRay.Dir, Length / rtm::vector_length3(OutRay.Dir));
}
//---------------------------------------------------------------------

void CCameraAttribute::GetPoint2D(const rtm::vector4f& Point3D, float& OutRelX, float& OutRelY) const
{
	const rtm::vector4f ViewPos = rtm::matrix_mul_point3(Point3D, _View);
	const rtm::vector4f ProjPos = rtm::matrix_mul_vector(rtm::vector_set_w(ViewPos, 0.f), _Proj);
	const rtm::vector4f HalfNormProjPos = rtm::vector_mul(rtm::vector_div(ProjPos, rtm::vector_dup_w(ProjPos)), 0.5f);
	OutRelX = 0.5f + rtm::vector_get_x(HalfNormProjPos);
	OutRelY = 0.5f - rtm::vector_get_y(HalfNormProjPos);
}
//---------------------------------------------------------------------

const rtm::vector4f& CCameraAttribute::GetPosition() const
{
	return _pNode->GetWorldPosition();
}
//---------------------------------------------------------------------

const rtm::matrix3x4f& CCameraAttribute::GetInvViewMatrix() const
{
	return _pNode->GetWorldMatrix();
}
//---------------------------------------------------------------------

}
