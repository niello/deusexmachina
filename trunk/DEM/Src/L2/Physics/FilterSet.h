#pragma once
#ifndef __DEM_L2_PHYSICS_FILTERSET_H__ //!!!to L1!
#define __DEM_L2_PHYSICS_FILTERSET_H__

#include <util/narray.h>

// A filter set object allows to define conditions which define a set of
// physics entities. It is used to include or exclude physics entities
// from various tests (like stabbing checks).

namespace Physics
{
class CShape;

class CFilterSet
{
private:

	nArray<uint> EntityIDs;
	nArray<uint> RigidBodyIDs;
	nArray<uint> MaterialTypes;

public:

	void AddEntityID(uint ID) { EntityIDs.Append(ID); }
	void AddRigidBodyID(uint ID) { RigidBodyIDs.Append(ID); }
	void AddMaterialType(uint Type) { MaterialTypes.Append(Type); }
	void Clear();
	
	bool CheckEntityID(uint ID) const { return EntityIDs.Find(ID) != NULL; }
	bool CheckRigidBodyID(uint ID) const { return RigidBodyIDs.Find(ID) != NULL; }
	bool CheckMaterialType(uint Type) const { return MaterialTypes.Find(Type) != NULL; }
	bool CheckShape(CShape* pShape) const;
	
	const nArray<uint>& GetEntityIDs() const { return EntityIDs; }
	const nArray<uint>& GetRigidBodyIDs() const { return RigidBodyIDs; }
	const nArray<uint>& GetMaterialTypes() const { return MaterialTypes; }
};
//---------------------------------------------------------------------

inline void CFilterSet::Clear()
{
	EntityIDs.Clear();
	RigidBodyIDs.Clear();
	MaterialTypes.Clear();
}
//---------------------------------------------------------------------

}

#endif
