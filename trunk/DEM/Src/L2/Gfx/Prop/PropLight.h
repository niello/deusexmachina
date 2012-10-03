#pragma once
#ifndef __DEM_L2_PROP_LIGHT_H__
#define __DEM_L2_PROP_LIGHT_H__

#include <Game/Property.h>
#include <Gfx/LightEntity.h>
#include <DB/AttrID.h>

// A light property adds a light source object (Graphics::LightEntity) to a game entity.

namespace Attr
{
	DeclareInt(LightType);
	DeclareVector4(LightColor);
	DeclareVector3(LightPos);
	DeclareFloat(LightRange);
	DeclareVector4(LightRot);    // a quaternion
	DeclareVector4(LightAmbient);
	DeclareBool(LightCastShadows);
}

namespace Properties
{

class CPropLight: public Game::CProperty
{
	DeclareRTTI;
	DeclareFactory(CPropLight);
	DeclarePropertyStorage;
	DeclarePropertyPools(Game::LivePool);

private:

	Ptr<Graphics::CLightEntity>	pLightEntity;

	DECLARE_EVENT_HANDLER(UpdateTransform, OnUpdateTransform);

public:

	virtual void	GetAttributes(nArray<DB::CAttrID>& Attrs);
	virtual void	Activate();
	virtual void	Deactivate();

	void			SetLight(const nLight& Light);
	const nLight&	GetLight() const { n_assert(pLightEntity.isvalid()); return pLightEntity->Light; }
};
//---------------------------------------------------------------------

RegisterFactory(CPropLight);

inline void CPropLight::SetLight(const nLight& Light)
{
	n_assert(pLightEntity.isvalid());
	pLightEntity->Light = Light;
	pLightEntity->UpdateNebulaLight();
}
//---------------------------------------------------------------------

}

#endif
