#include "../StdAPI.h"
#include <Game/Mgr/EntityManager.h>
#include <Game/Mgr/EnvQueryManager.h>
#include <Loading/EntityFactory.h>
#include "../App/CIDEApp.h"

using namespace Game;
using namespace App;

API int Entities_GetCount(CIDEAppHandle Handle)
{
	DeclareCIDEApp(Handle);
	return EntityMgr->GetNumEntities();
}
//---------------------------------------------------------------------

API void Entities_GetUIDByIndex(CIDEAppHandle Handle, int Idx, char* OutUID)
{
	DeclareCIDEApp(Handle);
	const char* Res = EntityMgr->GetEntityAt(Idx)->GetUniqueID().CStr();
	strcpy_s(OutUID, 255, Res);
}
//---------------------------------------------------------------------

API int Entities_GetNextFreeUIDOnLevel(const char* Base)
{
	nString BaseStr = Base;
	int Idx = 0;
	nString UIDStr;
	do UIDStr = BaseStr  + nString::FromInt(++Idx);
	while (EntityMgr->ExistsEntityByID(CStrID(UIDStr.Get())));
	return Idx;
}
//---------------------------------------------------------------------

API void Entities_GetCatByIndex(CIDEAppHandle Handle, int Idx, char* OutCategory)
{
	DeclareCIDEApp(Handle);
	const char* Res = EntityMgr->GetEntityAt(Idx)->GetCategory().CStr();
	strcpy_s(OutCategory, 255, Res);
}
//---------------------------------------------------------------------

API const char* Entities_GetCatByUID(const char* UID)
{
	Game::PEntity Ent = EntityMgr->GetEntityByID(CStrID(UID));
	return (Ent.isvalid()) ? Ent->GetCategory().CStr() : "";
}
//---------------------------------------------------------------------

API const char* Entities_GetUIDUnderMouse()
{
	Game::CEntity* pEnt = EnvQueryMgr->GetEntityUnderMouse();
	return (pEnt) ? pEnt->GetUniqueID().CStr() : "";
}
//---------------------------------------------------------------------

API bool Entities_CreateByCategory(CIDEAppHandle Handle, const char* UID, const char* Category)
{
	DeclareCIDEApp(Handle);
	if (EntityMgr->ExistsEntityByID(CStrID(UID))) FAIL;
	CIDEApp->CurrentEntity = EntityFct->CreateEntityByCategory(CStrID(UID), CStrID(Category));
	return CIDEApp->CurrentEntity.isvalid();
}
//---------------------------------------------------------------------

API bool Entities_CreateFromTemplate(CIDEAppHandle Handle, const char* UID, const char* Category, const char* TemplateID)
{
	DeclareCIDEApp(Handle);
	if (EntityMgr->ExistsEntityByID(CStrID(UID))) FAIL;
	CIDEApp->CurrentEntity = EntityFct->CreateEntityByTemplate(CStrID(UID), CStrID(Category), CStrID(TemplateID));
	return CIDEApp->CurrentEntity.isvalid();
}
//---------------------------------------------------------------------

static DB::CDataset* pDS = NULL;
bool InTemplate = false;

API bool Entities_BeginTemplate(const char* UID, const char* Category, bool CreateIfNotExist)
{
	InTemplate = true;
	pDS = EntityFct->GetTemplate(CStrID(UID), CStrID(Category), CreateIfNotExist);
	return pDS != NULL;
}
//---------------------------------------------------------------------

API void Entities_EndTemplate()
{
	InTemplate = false;
	pDS = NULL;
}
//---------------------------------------------------------------------

API bool Entities_AttachToWorld(CIDEAppHandle Handle)
{
	DeclareCIDEApp(Handle);
	if (CIDEApp->CurrentEntity.isvalid())
	{
		EntityMgr->AttachEntity(CIDEApp->CurrentEntity);
		OK;
	}
	FAIL;
}
//---------------------------------------------------------------------

API void Entities_DeleteByUID(const char* UID)
{
	Game::PEntity Ent = EntityMgr->GetEntityByID(CStrID(UID));
	if (Ent.isvalid()) EntityMgr->DeleteEntity(Ent);
}
//---------------------------------------------------------------------

API void Entities_DeleteTemplate(const char* UID, const char* Category)
{
	EntityFct->DeleteTemplate(CStrID(UID), CStrID(Category));
	pDS = NULL;
}
//---------------------------------------------------------------------

API bool Entities_SetCurrentByUID(CIDEAppHandle Handle, const char* UID)
{
	DeclareCIDEApp(Handle);
	if(UID)
		CIDEApp->CurrentEntity = EntityMgr->GetEntityByID(CStrID(UID));
	else CIDEApp->CurrentEntity = NULL;
	return CIDEApp->CurrentEntity.isvalid();
}
//---------------------------------------------------------------------

API bool Entities_GetBool(CIDEAppHandle Handle, int AttrID)
{
	DeclareCIDEApp(Handle);

	if (InTemplate)
		return (pDS && pDS->GetValueTable()->HasColumn((DB::CAttrID)AttrID)) ?
			pDS->Get<bool>((DB::CAttrID)AttrID) :
			((DB::CAttrID)AttrID)->GetDefaultValue().GetValue<bool>();
	else
		return (CIDEApp->CurrentEntity.isvalid()) ?
			CIDEApp->CurrentEntity->Get<bool>((DB::CAttrID)AttrID) :
			((DB::CAttrID)AttrID)->GetDefaultValue().GetValue<bool>();
}
//---------------------------------------------------------------------

API int Entities_GetInt(CIDEAppHandle Handle, int AttrID)
{
	DeclareCIDEApp(Handle);

	if (InTemplate)
		return (pDS && pDS->GetValueTable()->HasColumn((DB::CAttrID)AttrID)) ?
			pDS->Get<int>((DB::CAttrID)AttrID) :
			((DB::CAttrID)AttrID)->GetDefaultValue().GetValue<int>();
	else
		return (CIDEApp->CurrentEntity.isvalid()) ?
			CIDEApp->CurrentEntity->Get<int>((DB::CAttrID)AttrID) :
			((DB::CAttrID)AttrID)->GetDefaultValue().GetValue<int>();
}
//---------------------------------------------------------------------

API float Entities_GetFloat(CIDEAppHandle Handle, int AttrID)
{
	DeclareCIDEApp(Handle);

	if (InTemplate)
		return (pDS && pDS->GetValueTable()->HasColumn((DB::CAttrID)AttrID)) ?
			pDS->Get<float>((DB::CAttrID)AttrID) :
			((DB::CAttrID)AttrID)->GetDefaultValue().GetValue<float>();
	else
		return (CIDEApp->CurrentEntity.isvalid()) ?
			CIDEApp->CurrentEntity->Get<float>((DB::CAttrID)AttrID) :
			((DB::CAttrID)AttrID)->GetDefaultValue().GetValue<float>();
}
//---------------------------------------------------------------------

API void Entities_GetString(CIDEAppHandle Handle, int AttrID, char* Out)
{
	DeclareCIDEApp(Handle);

	nString Tmp;
	if (InTemplate)
		Tmp = (pDS && pDS->GetValueTable()->HasColumn((DB::CAttrID)AttrID)) ?
			pDS->Get<nString>((DB::CAttrID)AttrID) :
			((DB::CAttrID)AttrID)->GetDefaultValue().GetValue<nString>();
	else
		Tmp = (CIDEApp->CurrentEntity.isvalid()) ?
			CIDEApp->CurrentEntity->Get<nString>((DB::CAttrID)AttrID) :
			((DB::CAttrID)AttrID)->GetDefaultValue().GetValue<nString>();
	LPCSTR OutStr = Tmp.Get();
	if(OutStr) strcpy_s(Out, 1023, OutStr);
}
//---------------------------------------------------------------------

API void Entities_GetStrID(CIDEAppHandle Handle, int AttrID, char* Out)
{
	DeclareCIDEApp(Handle);

	CStrID Tmp;
	if (InTemplate)
		Tmp = (pDS && pDS->GetValueTable()->HasColumn((DB::CAttrID)AttrID)) ?
			pDS->Get<CStrID>((DB::CAttrID)AttrID) :
			((DB::CAttrID)AttrID)->GetDefaultValue().GetValue<CStrID>();
	else
		Tmp = (CIDEApp->CurrentEntity.isvalid()) ?
			CIDEApp->CurrentEntity->Get<CStrID>((DB::CAttrID)AttrID) :
			((DB::CAttrID)AttrID)->GetDefaultValue().GetValue<CStrID>();
	LPCSTR OutStr = Tmp.CStr();
	if(OutStr) strcpy_s(Out, 255, OutStr);
}
//---------------------------------------------------------------------

API void Entities_GetVector4(CIDEAppHandle Handle, int AttrID, float Out[4])
{
	DeclareCIDEApp(Handle);

	vector4 Tmp;
	if (InTemplate)
		Tmp = (pDS && pDS->GetValueTable()->HasColumn((DB::CAttrID)AttrID)) ?
			pDS->Get<vector4>((DB::CAttrID)AttrID) :
			((DB::CAttrID)AttrID)->GetDefaultValue().GetValue<vector4>();
	else
		Tmp = (CIDEApp->CurrentEntity.isvalid()) ?
			CIDEApp->CurrentEntity->Get<vector4>((DB::CAttrID)AttrID) :
			((DB::CAttrID)AttrID)->GetDefaultValue().GetValue<vector4>();
	memcpy(Out, &Tmp.x, sizeof(float) * 4);
}
//---------------------------------------------------------------------

API void Entities_GetMatrix44(CIDEAppHandle Handle, int AttrID, float Out[16])
{
	DeclareCIDEApp(Handle);

	matrix44 Tmp;
	if (InTemplate)
		Tmp = (pDS && pDS->GetValueTable()->HasColumn((DB::CAttrID)AttrID)) ?
			pDS->Get<matrix44>((DB::CAttrID)AttrID) :
			((DB::CAttrID)AttrID)->GetDefaultValue().GetValue<matrix44>();
	else
		Tmp = (CIDEApp->CurrentEntity.isvalid()) ?
			CIDEApp->CurrentEntity->Get<matrix44>((DB::CAttrID)AttrID) :
			((DB::CAttrID)AttrID)->GetDefaultValue().GetValue<matrix44>();
	memcpy(Out, &(Tmp.m[0][0]), sizeof(float) * 16);
}
//---------------------------------------------------------------------

API void Entities_SetBool(CIDEAppHandle Handle, int AttrID, bool Value)
{
	DeclareCIDEApp(Handle);

	if (InTemplate) pDS->ForceSet<bool>((DB::CAttrID)AttrID, Value);
	else if (CIDEApp->CurrentEntity.isvalid())
		CIDEApp->CurrentEntity->Set<bool>((DB::CAttrID)AttrID, Value);
}
//---------------------------------------------------------------------

API void Entities_SetInt(CIDEAppHandle Handle, int AttrID, int Value)
{
	DeclareCIDEApp(Handle);

	if (InTemplate) pDS->ForceSet<int>((DB::CAttrID)AttrID, Value);
	else if (CIDEApp->CurrentEntity.isvalid())
		CIDEApp->CurrentEntity->Set<int>((DB::CAttrID)AttrID, Value);
}
//---------------------------------------------------------------------

API void Entities_SetFloat(CIDEAppHandle Handle, int AttrID, float Value)
{
	DeclareCIDEApp(Handle);

	if (InTemplate) pDS->ForceSet<float>((DB::CAttrID)AttrID, Value);
	else if (CIDEApp->CurrentEntity.isvalid())
		CIDEApp->CurrentEntity->Set<float>((DB::CAttrID)AttrID, Value);
}
//---------------------------------------------------------------------

API void Entities_SetString(CIDEAppHandle Handle, int AttrID, const char* Value)
{
	DeclareCIDEApp(Handle);

	if (InTemplate) pDS->ForceSet<nString>((DB::CAttrID)AttrID, Value);
	else if (CIDEApp->CurrentEntity.isvalid())
		CIDEApp->CurrentEntity->Set<nString>((DB::CAttrID)AttrID, Value);
}
//---------------------------------------------------------------------

API void Entities_SetStrID(CIDEAppHandle Handle, int AttrID, const char* Value)
{
	DeclareCIDEApp(Handle);

	if (InTemplate) pDS->ForceSet<CStrID>((DB::CAttrID)AttrID, CStrID(Value));
	else if (CIDEApp->CurrentEntity.isvalid())
		CIDEApp->CurrentEntity->Set<CStrID>((DB::CAttrID)AttrID, CStrID(Value));
}
//---------------------------------------------------------------------

API void Entities_SetVector4(CIDEAppHandle Handle, int AttrID, float Value[4])
{
	vector4 Vct(Value[0], Value[1], Value[2], Value[3]);
	
	DeclareCIDEApp(Handle);

	if (InTemplate) pDS->ForceSet<vector4>((DB::CAttrID)AttrID, Vct);
	else if (CIDEApp->CurrentEntity.isvalid())
		CIDEApp->CurrentEntity->Set<vector4>((DB::CAttrID)AttrID, Vct);
}
//---------------------------------------------------------------------

API void Entities_SetMatrix44(CIDEAppHandle Handle, int AttrID, float Value[16])
{
	matrix44 Mtx(Value[0],  Value[1],  Value[2],  Value[3],
				 Value[4],  Value[5],  Value[6],  Value[7],
				 Value[8],  Value[9],  Value[10], Value[11],
				 Value[12], Value[13], Value[14], Value[15]);

	DeclareCIDEApp(Handle);
	
	if (InTemplate) pDS->ForceSet<matrix44>((DB::CAttrID)AttrID, Mtx);
	else if (CIDEApp->CurrentEntity.isvalid())
		CIDEApp->CurrentEntity->Set<matrix44>((DB::CAttrID)AttrID, Mtx);
}
//---------------------------------------------------------------------

API bool Entities_SetUID(CIDEAppHandle Handle, const char* UID, const char* NewUID)
{
	DeclareCIDEApp(Handle);

	return CIDEApp->CurrentEntity.isvalid() ?
		EntityMgr->ChangeEntityID(CIDEApp->CurrentEntity, CStrID(NewUID)) :
		false;
}
//---------------------------------------------------------------------

API void Entities_UpdateAttrs(CIDEAppHandle Handle)
{
	DeclareCIDEApp(Handle);

	if (CIDEApp->CurrentEntity.isvalid())
	{
		//CIDEApp->CurrentEntity->Deactivate();
		//CIDEApp->CurrentEntity->Activate();
		//if (GameSrv->HasStarted()) pEntity->FireEvent(CStrID("OnStart"));
		EntityMgr->RemoveEntity(CIDEApp->CurrentEntity);
		EntityMgr->AttachEntity(CIDEApp->CurrentEntity);
	}
}
//---------------------------------------------------------------------
