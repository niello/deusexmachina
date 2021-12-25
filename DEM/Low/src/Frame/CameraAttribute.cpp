#include "CameraAttribute.h"
#include <Scene/SceneNode.h>
#include <Debug/DebugDraw.h>
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
	DebugDraw.DrawFrustumWireframe(_ViewProj, Render::ColorRGBA(128, 0, 255, 255), 2.f);
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

void CCameraAttribute::UpdateBeforeChildren(const vector3* pCOIArray, UPTR COICount)
{
	bool ViewOrProjChanged = false;

	if (_Flags.Is(ProjDirty))
	{
		if (_Flags.Is(Orthogonal)) _Proj.orthoRh(_Width, _Height, _NearPlane, _FarPlane);
		else _Proj.perspFovRh(_FOV, _Width / _Height, _NearPlane, _FarPlane);

		// Shadow proj was calculated with:
		//nearPlane - shadowOffset, farPlane - shadowOffset, shadowOffset(0.00007f)

		//!!!avoid copying!
		_InvProj = _Proj;
		_InvProj.invert();

		_Flags.Clear(ProjDirty);
		ViewOrProjChanged = true;
	}

	if (_pNode->GetTransformVersion() != _LastTransformVersion)
	{
		_pNode->GetWorldMatrix().invert_simple(_View);
		_LastTransformVersion = _pNode->GetTransformVersion();
		ViewOrProjChanged = true;
	}

	if (ViewOrProjChanged) _ViewProj = _View * _Proj;
}
//---------------------------------------------------------------------

void CCameraAttribute::GetRay3D(float RelX, float RelY, float Length, line3& OutRay) const
{
	vector3 ScreenCoord3D((RelX - 0.5f) * 2.0f, (RelY - 0.5f) * 2.0f, 1.0f);
	vector3 ViewLocalPos = (_InvProj * ScreenCoord3D) * _NearPlane * 1.1f;
	ViewLocalPos.y = -ViewLocalPos.y;
	const matrix44&	InvView = GetInvViewMatrix();
	OutRay.Start = InvView * ViewLocalPos;
	OutRay.Vector = OutRay.Start - InvView.Translation();
	OutRay.Vector *= (Length / OutRay.Length());
}
//---------------------------------------------------------------------

void CCameraAttribute::GetPoint2D(const vector3& Point3D, float& OutRelX, float& OutRelY) const
{
	vector4 WorldPos = _View * Point3D;
	WorldPos.w = 0.f;
	WorldPos = _Proj * WorldPos;
	WorldPos /= WorldPos.w;
	OutRelX = 0.5f + WorldPos.x * 0.5f;
	OutRelY = 0.5f - WorldPos.y * 0.5f;
}
//---------------------------------------------------------------------

const vector3& CCameraAttribute::GetPosition() const
{
	return _pNode->GetWorldPosition();
}
//---------------------------------------------------------------------

const matrix44& CCameraAttribute::GetInvViewMatrix() const
{
	return _pNode->GetWorldMatrix();
}
//---------------------------------------------------------------------

}
