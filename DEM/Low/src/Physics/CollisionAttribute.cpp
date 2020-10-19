#include "CollisionAttribute.h"
#include <Physics/StaticCollider.h>
#include <Physics/MovableCollider.h>
#include <Physics/CollisionShape.h>
#include <Physics/PhysicsLevel.h>
#include <Scene/SceneNode.h>
#include <Resources/ResourceManager.h>
#include <Resources/Resource.h>
#include <IO/BinaryReader.h>
#include <Core/Factory.h>

namespace Physics
{
FACTORY_CLASS_IMPL(Physics::CCollisionAttribute, 'COLA', Scene::CNodeAttribute);

bool CCollisionAttribute::LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count)
{
	for (UPTR j = 0; j < Count; ++j)
	{
		const uint32_t Code = DataReader.Read<uint32_t>();
		switch (Code)
		{
			case 'SHAP':
			{
				_ShapeUID = DataReader.Read<CStrID>();
				break;
			}
			case 'STAT':
			{
				_Static = DataReader.Read<bool>();
				break;
			}
			case 'COGR':
			{
				_CollisionGroupID = DataReader.Read<CStrID>();
				break;
			}
			case 'COMA':
			{
				_CollisionMaskID = DataReader.Read<CStrID>();
				break;
			}
			default: FAIL;
		}
	}

	OK;
}
//---------------------------------------------------------------------

Scene::PNodeAttribute CCollisionAttribute::Clone()
{
	PCollisionAttribute ClonedAttr = n_new(CCollisionAttribute());
	ClonedAttr->_ShapeUID = _ShapeUID;
	ClonedAttr->_CollisionGroupID = _CollisionGroupID;
	ClonedAttr->_CollisionMaskID = _CollisionMaskID;
	return ClonedAttr;
}
//---------------------------------------------------------------------

void CCollisionAttribute::UpdateBeforeChildren(const vector3* pCOIArray, UPTR COICount)
{
	if (!_Collider) return;

	if (_pNode->GetTransformVersion() != _LastTransformVersion)
	{
		_Collider->SetTransform(_pNode->GetWorldMatrix());
		_LastTransformVersion = _pNode->GetTransformVersion();
	}
	else
	{
		_Collider->SetActive(false);
	}
}
//---------------------------------------------------------------------

bool CCollisionAttribute::ValidateResources(Resources::CResourceManager& ResMgr)
{
	if (!_pNode) FAIL;

	if (!_Collider)
	{
		Resources::PResource RShape = ResMgr.RegisterResource<Physics::CCollisionShape>(_ShapeUID.CStr());
		if (!RShape) FAIL;

		auto Shape = RShape->ValidateObject<Physics::CCollisionShape>();
		if (!Shape) FAIL;

		const matrix44& Tfm = _pNode->IsWorldTransformDirty() ? matrix44::Identity : _pNode->GetWorldMatrix();
		if (_Static)
			_Collider = n_new(Physics::CStaticCollider(*Shape, _CollisionGroupID, _CollisionMaskID, Tfm));
		else
			_Collider = new Physics::CMovableCollider(*Shape, _CollisionGroupID, _CollisionMaskID, Tfm);

		// Just updated, save redundant update
		if (!_pNode->IsWorldTransformDirty())
			_LastTransformVersion = _pNode->GetTransformVersion();
	}

	if (IsActive() && _Level) _Collider->AttachToLevel(*_Level);
	else _Collider->RemoveFromLevel();

	OK;
}
//---------------------------------------------------------------------

void CCollisionAttribute::SetPhysicsLevel(PPhysicsLevel Level)
{
	_Level = Level;
	if (IsActive() && _Collider)
	{
		if (_Level) _Collider->AttachToLevel(*_Level);
		else _Collider->RemoveFromLevel();
	}
}
//---------------------------------------------------------------------

void CCollisionAttribute::OnActivityChanged(bool Active)
{
	if (!_Collider) return;
	if (Active && _Level) _Collider->AttachToLevel(*_Level);
	else _Collider->RemoveFromLevel();
}
//---------------------------------------------------------------------

bool CCollisionAttribute::GetGlobalAABB(CAABB& OutBox) const
{
	if (!_Collider) FAIL;
	_Collider->GetGlobalAABB(OutBox);
	OK;
}
//---------------------------------------------------------------------

}
