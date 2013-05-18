#include "StaticEnvManager.h"

#include <Data/DataServer.h>
#include <Data/DataArray.h>

namespace Game
{
__ImplementClassNoFactory(Game::CStaticEnvManager, Core::CRefCounted);
__ImplementSingleton(CStaticEnvManager);

using namespace Physics;

bool CStaticEnvManager::CanEntityBeStatic(const Data::CParams& Desc) const
{
	if (!Desc.Has(CStrID("Transform"))) FAIL;

	//???do controllers really deny loading of entity as a static object?
	//scene allows node without an entity to be controlled

	// We have physics bodies that can move us
	const nString& PhysDesc = Desc.Get<nString>(CStrID("Physics"), NULL);
	if (PhysDesc.IsValid())
	{
		// It uses HRD cache, so it isn't so slow
		Data::PParams Desc = DataSrv->LoadPRM(nString("physics:") + PhysDesc + ".prm");
		int Idx = Desc->IndexOf(CStrID("Bodies"));
		if (Idx != INVALID_INDEX && Desc->Get<Data::PDataArray>(Idx)->GetCount() > 0) FAIL;
	}

	// We have animations that can move us
	if (Desc.Get<nString>(CStrID("AnimDesc"), NULL).IsValid()) FAIL;

	OK;
}
//---------------------------------------------------------------------

PStaticObject CStaticEnvManager::CreateStaticObject(CStrID UID, CGameLevel& Level)
{
	n_assert(UID.IsValid());
	n_assert(!StaticObjectExists(UID)); //???return NULL or existing object?
	PStaticObject Obj = n_new(CStaticObject(UID, Level));
	Objects.Add(UID, Obj);
	return Obj;
}
//---------------------------------------------------------------------

void CStaticEnvManager::DeleteStaticObject(int Idx)
{
	CStaticObject& Obj = *Objects.ValueAtIndex(Idx);
	if (Obj.IsValid()) Obj.Term();
	Objects.EraseAt(Idx);
}
//---------------------------------------------------------------------

void CStaticEnvManager::DeleteStaticObjects(const CGameLevel& Level)
{
	for (int i = Objects.GetCount() - 1; i >= 0; --i)
		if (&Objects.ValueAtIndex(i)->GetLevel() == &Level)
			DeleteStaticObject(i);
}
//---------------------------------------------------------------------

void CStaticEnvManager::DeleteAllStaticObjects()
{
	for (int i = 0; i < Objects.GetCount(); ++i)
		if (Objects.ValueAtIndex(i)->IsValid())
			Objects.ValueAtIndex(i)->Term();
	Objects.Clear();
}
//---------------------------------------------------------------------

}