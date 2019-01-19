#include "ResourceManager.h"

#include <Resources/Resource.h>
#include <Resources/ResourceCreator.h>
#include <IO/PathUtils.h>

namespace Resources
{
__ImplementSingleton(CResourceManager);

CResourceManager::CResourceManager(UPTR InitialCapacity): Registry(InitialCapacity)
{
	__ConstructSingleton;
}
//---------------------------------------------------------------------

CResourceManager::~CResourceManager()
{
	__DestructSingleton;
}
//---------------------------------------------------------------------

// NB: does not change creator for existing resource
PResource CResourceManager::RegisterResource(CStrID UID, const Core::CRTTI& RsrcType)
{
	PResource Rsrc;
	if (!Registry.Get(UID, Rsrc))
	{
		Rsrc = n_new(CResource(UID));
		Rsrc->SetCreator(GetDefaultCreator(PathUtils::GetExtension(UID.CStr()), &RsrcType));
		Registry.Add(UID, Rsrc);
	}
	return Rsrc;
}
//---------------------------------------------------------------------

// NB: does not change creator for existing resource
PResource CResourceManager::RegisterResource(CStrID UID, IResourceCreator* pCreator)
{
	PResource Rsrc;
	if (!Registry.Get(UID, Rsrc))
	{
		Rsrc = n_new(CResource(UID));
		Rsrc->SetCreator(pCreator);
		Registry.Add(UID, Rsrc);
	}
	return Rsrc;
}
//---------------------------------------------------------------------

CResource* CResourceManager::FindResource(CStrID UID) const
{
	PResource Rsrc;
	Registry.Get(UID, Rsrc);
	return Rsrc.Get();
}
//---------------------------------------------------------------------

void CResourceManager::UnregisterResource(CStrID UID)
{
	Registry.Remove(UID);
	//???force unload resource object here even if there are references to resource?
}
//---------------------------------------------------------------------

bool CResourceManager::RegisterDefaultCreator(const char* pFmtExtension, const Core::CRTTI* pRsrcType, IResourceCreator* pCreator, bool ClonePerResource)
{
	if (pCreator &&	pRsrcType && !pCreator->GetResultType().IsDerivedFrom(*pRsrcType)) FAIL;

	CString ExtStr(pFmtExtension);
	ExtStr.Trim();
	ExtStr.ToLower();
	CStrID Ext(ExtStr.CStr());

	auto It = std::find_if(DefaultCreators.begin(), DefaultCreators.end(), [Ext, pRsrcType](const CDefaultCreatorRecord& Rec)
	{
		return Rec.Extension == Ext && Rec.pRsrcType == pRsrcType;
	});

	if (!pCreator)
	{
		if (It != DefaultCreators.end())
		{
			DefaultCreators.erase(It);
			OK;
		}
		else FAIL;
	}

	CDefaultCreatorRecord Rec;
	Rec.Extension = Ext;
	Rec.pRsrcType = pRsrcType;
	Rec.Creator = pCreator;
	Rec.ClonePerResource = ClonePerResource;
	DefaultCreators.push_back(std::move(Rec));
	OK;
}
//---------------------------------------------------------------------

PResourceCreator CResourceManager::GetDefaultCreator(const char* pFmtExtension, const Core::CRTTI* pRsrcType)
{
	CString ExtStr(pFmtExtension);
	ExtStr.Trim();
	ExtStr.ToLower();
	CStrID Ext(ExtStr.CStr());

	const CDefaultCreatorRecord* pRec = nullptr;
	for (const auto& Rec : DefaultCreators)
	{
		// Check if explicitly requested parameters don't match the record
		if (Ext.IsValid() && Rec.Extension.IsValid() && Rec.Extension != Ext) continue;
		if (pRsrcType && Rec.pRsrcType && !pRsrcType->IsDerivedFrom(*Rec.pRsrcType)) continue;

		// The first matching record, nothing to compare with
		if (!pRec)
		{
			pRec = &Rec;
			continue;
		}

		// Prioritize by type first. The closer a type is to the
		// requested one, the more preferable it is.
		if (pRec->pRsrcType != Rec.pRsrcType)
		{
			if (Rec.pRsrcType->IsDerivedFrom(*pRec->pRsrcType))
				pRec = &Rec;
			continue;
		}

		// If no obvious priority comes from types, prefer record with an extension
		if (Rec.Extension.IsValid() && !pRec->Extension.IsValid())
			pRec = &Rec;
	}

	if (!pRec)
	{
		Sys::Error("CResourceManager::GetDefaultCreator() > no record for ext '%s' and type '%s'\n", Ext.CStr(), pRsrcType ? pRsrcType->GetName().CStr() : "<any>");
		return nullptr;
	}

	// TODO: revisit necessity & implement if needed
	n_assert(!pRec->ClonePerResource);

	return //pRec->ClonePerResource ? pRec->Loader->Clone() :
		pRec->Creator;
}
//---------------------------------------------------------------------

}