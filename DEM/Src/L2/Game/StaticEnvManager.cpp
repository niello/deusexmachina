#include "StaticEnvManager.h"

#include <Data/Params.h>
#include <Data/DataArray.h>

namespace Game
{
__ImplementSingleton(CStaticEnvManager);

bool CStaticEnvManager::CanEntityBeStatic(const Data::CParams& Desc) const
{
	//!!!???check props?!

	Data::PParams Attrs;
	if (!Desc.Get(Attrs, CStrID("Attrs"))) FAIL;

	if (!Attrs->Has(CStrID("Transform"))) FAIL;

	//???do controllers really deny loading of entity as a static object?
	//scene allows node without an entity to be controlled

	// We have animations that can move us
	if (Attrs->Get<CString>(CStrID("AnimDesc"), CString::Empty).IsValid()) FAIL;

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

void CStaticEnvManager::DeleteStaticObject(IPTR Idx)
{
	CStaticObject& Obj = *Objects.ValueAt(Idx);
	if (Obj.IsValidPtr()) Obj.Term();
	Objects.RemoveAt(Idx);
}
//---------------------------------------------------------------------

void CStaticEnvManager::DeleteStaticObjects(const CGameLevel& Level)
{
	for (int i = Objects.GetCount() - 1; i >= 0; --i)
		if (&Objects.ValueAt(i)->GetLevel() == &Level)
			DeleteStaticObject(i);
}
//---------------------------------------------------------------------

void CStaticEnvManager::DeleteAllStaticObjects()
{
	for (UPTR i = 0; i < Objects.GetCount(); ++i)
		if (Objects.ValueAt(i)->IsValidPtr())
			Objects.ValueAt(i)->Term();
	Objects.Clear();
}
//---------------------------------------------------------------------

}