#pragma once
#ifndef __DEM_L2_PHYSICS_MTL_TABLE_H__ //!!!to L1!
#define __DEM_L2_PHYSICS_MTL_TABLE_H__

#include <kernel/ntypes.h>
#include <util/nstring.h>
#include <util/narray.h>

// Contains CMaterial properties and Friction coefficients.

namespace Physics
{
typedef int CMaterialType;
const CMaterialType InvalidMaterial = -1;

class CMaterialTable
{
private:

	struct CMaterial
	{
		nString	Name;
		float	Density;
		CMaterial(): Name(), Density(0.0f) {}
	};

	struct CInteraction
	{
		float	Friction;
		float	Bouncyness;
		nString	CollisionSound;
		CInteraction(): Friction(0.0f), Bouncyness(0.0f), CollisionSound() {}
	};

	static int							MtlCount;
	static nArray<struct CMaterial>		Materials;
	static nArray<struct CInteraction>	Interactions;

	CMaterialTable();
	~CMaterialTable();

public:

	static void Setup();

    static nString			MaterialTypeToString(CMaterialType Type);
    static CMaterialType	StringToMaterialType(const nString& MtlName);

	static float			GetDensity(CMaterialType Type);
    static float			GetFriction(CMaterialType Type1, CMaterialType Type2);
    static float			GetBounce(CMaterialType Type1, CMaterialType Type2);
    static const nString&	GetCollisionSound(CMaterialType Type1, CMaterialType Type2);
};
//---------------------------------------------------------------------

inline nString CMaterialTable::MaterialTypeToString(CMaterialType Type)
{
	return (Type == InvalidMaterial) ? "InvalidMaterial" : Materials[Type].Name;
}
//---------------------------------------------------------------------

inline float CMaterialTable::GetDensity(CMaterialType Type)
{
	return Materials[Type].Density;
}
//---------------------------------------------------------------------

inline float CMaterialTable::GetFriction(CMaterialType Type1, CMaterialType Type2)
{
	n_assert(Type1 >= 0 && Type1 < MtlCount);
	n_assert(Type2 >= 0 && Type2 < MtlCount);
	return Interactions[Type1 * MtlCount + Type2].Friction;
}
//---------------------------------------------------------------------

inline float CMaterialTable::GetBounce(CMaterialType Type1, CMaterialType Type2)
{
	n_assert(Type1 >= 0 && Type1 < MtlCount);
	n_assert(Type2 >= 0 && Type2 < MtlCount);
	return Interactions[Type1 * MtlCount + Type2].Bouncyness;
}
//---------------------------------------------------------------------

inline const nString& CMaterialTable::GetCollisionSound(CMaterialType Type1, CMaterialType Type2)
{
	n_assert(Type1 >= 0 && Type1 < MtlCount);
	n_assert(Type2 >= 0 && Type2 < MtlCount);
	return Interactions[Type1 * MtlCount + Type2].CollisionSound;
}
//---------------------------------------------------------------------

}

#endif
