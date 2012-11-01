#include "PropLight.h"

#include <Game/Entity.h>
#include <Gfx/GfxServer.h>
#include <Scene/SceneServer.h>
#include <Loading/EntityFactory.h>
#include <DB/DBServer.h>

namespace Attr
{
	DeclareAttr(Transform);

    DefineInt(LightType);
    DefineVector4(LightColor);
    DefineVector3(LightPos);
    DefineFloat(LightRange);
    DefineVector4(LightRot);    // a quaternion
    DefineVector4(LightAmbient);
    DefineBool(LightCastShadows);
};

BEGIN_ATTRS_REGISTRATION(PropLight)
	RegisterIntWithDefault(LightType, ReadOnly, (int)nLight::Point);
    RegisterVector4WithDefault(LightColor, ReadOnly, vector4(1.f, 1.f, 1.f, 1.f));
    RegisterVector3(LightPos, ReadOnly);
    RegisterFloatWithDefault(LightRange, ReadOnly, 10.f);
    RegisterVector4(LightRot, ReadOnly);    // a quaternion
    RegisterVector4(LightAmbient, ReadOnly);
    RegisterBool(LightCastShadows, ReadOnly);
END_ATTRS_REGISTRATION

namespace Properties
{
ImplementRTTI(Properties::CPropLight, Game::CProperty);
ImplementFactory(Properties::CPropLight);
ImplementPropertyStorage(CPropLight, 32);
RegisterProperty(CPropLight);

using namespace Game;

void CPropLight::GetAttributes(nArray<DB::CAttrID>& Attrs)
{
	CProperty::GetAttributes(Attrs);
	Attrs.Append(Attr::LightType);
	Attrs.Append(Attr::LightColor);
	Attrs.Append(Attr::LightRange);
	Attrs.Append(Attr::LightAmbient);
	Attrs.Append(Attr::LightCastShadows);
}
//---------------------------------------------------------------------

void CPropLight::Activate()
{
	CProperty::Activate();

	// Cache scene node
	//Scene::PSceneNode Node = SceneSrv->GetCurrentScene()->GetNode(Attr::ScenePath);

	pLightEntity = Graphics::CLightEntity::Create();

	pLightEntity->Light.SetType((nLight::Type)GetEntity()->Get<int>(Attr::LightType));
	pLightEntity->Light.SetDiffuse(GetEntity()->Get<vector4>(Attr::LightColor));
	pLightEntity->Light.SetSpecular(pLightEntity->Light.GetDiffuse());
	pLightEntity->Light.SetCastShadows(GetEntity()->Get<bool>(Attr::LightCastShadows));
	if (pLightEntity->Light.GetType() == nLight::Point)
		pLightEntity->Light.SetRange(GetEntity()->Get<float>(Attr::LightRange));
	else
		pLightEntity->Light.SetAmbient(GetEntity()->Get<vector4>(Attr::LightAmbient));

	pLightEntity->UpdateNebulaLight();
	pLightEntity->SetTransform(GetEntity()->Get<matrix44>(Attr::Transform));

	GfxSrv->GetLevel()->AttachEntity(pLightEntity);

	PROP_SUBSCRIBE_PEVENT(UpdateTransform, CPropLight, OnUpdateTransform);
}
//---------------------------------------------------------------------

void CPropLight::Deactivate()
{
	UNSUBSCRIBE_EVENT(UpdateTransform);

	GfxSrv->GetLevel()->RemoveEntity(pLightEntity);
	pLightEntity = NULL;

	CProperty::Deactivate();
}
//---------------------------------------------------------------------

bool CPropLight::OnUpdateTransform(const CEventBase& Event)
{
	pLightEntity->SetTransform(GetEntity()->Get<matrix44>(Attr::Transform));
	OK;
}
//---------------------------------------------------------------------

} // namespace Properties
