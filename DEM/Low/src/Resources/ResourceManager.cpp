#include "ResourceManager.h"

#include <Resources/Resource.h>
#include <Resources/ResourceCreator.h>
#include <IO/PathUtils.h>
#include <IO/IOServer.h>
#include <IO/Stream.h>

namespace Resources
{
__ImplementSingleton(CResourceManager);

CResourceManager::CResourceManager(IO::CIOServer* pIOServer, UPTR InitialCapacity)
	: Registry(InitialCapacity)
	, pIO(pIOServer)
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
PResource CResourceManager::RegisterResource(const char* pUID, const Core::CRTTI& RsrcType)
{
	CStrID UID = pIO ? CStrID(pIO->ResolveAssigns(pUID).CStr()): CStrID(pUID);
	PResource Rsrc;
	if (!Registry.Get(UID, Rsrc))
	{
		Rsrc = n_new(CResource(UID));

		CString Ext(PathUtils::GetExtension(UID.CStr()));
		if (Ext.IsValid())
		{
			IPTR SharpIdx = Ext.FindIndex('#');
			if (SharpIdx >= 0) Ext.TruncateRight(Ext.GetLength() - SharpIdx);
		}

		Rsrc->SetCreator(GetDefaultCreator(Ext, &RsrcType));
		Registry.Add(UID, Rsrc);
	}
	return Rsrc;
}
//---------------------------------------------------------------------

// NB: does not change creator for existing resource
PResource CResourceManager::RegisterResource(const char* pUID, IResourceCreator* pCreator)
{
	CStrID UID = pIO ? CStrID(pIO->ResolveAssigns(pUID).CStr()): CStrID(pUID);
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

CResource* CResourceManager::FindResource(const char* pUID) const
{
	return FindResource(pIO ? CStrID(pIO->ResolveAssigns(pUID).CStr()): CStrID(pUID));
}
//---------------------------------------------------------------------

CResource* CResourceManager::FindResource(CStrID UID) const
{
	PResource Rsrc;
	Registry.Get(UID, Rsrc);
	return Rsrc.Get();
}
//---------------------------------------------------------------------

void CResourceManager::UnregisterResource(const char* pUID)
{
	return UnregisterResource(pIO ? CStrID(pIO->ResolveAssigns(pUID).CStr()): CStrID(pUID));
}
//---------------------------------------------------------------------

//???need const char* variant?
void CResourceManager::UnregisterResource(CStrID UID)
{
	Registry.Remove(UID);
	//???force unload resource object here even if there are references to resource?
}
//---------------------------------------------------------------------

bool CResourceManager::RegisterDefaultCreator(const char* pFmtExtension, const Core::CRTTI* pRsrcType, IResourceCreator* pCreator)
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

	return pRec->Creator;
}
//---------------------------------------------------------------------

IO::PStream CResourceManager::CreateResourceStream(const char* pUID, const char*& pOutSubId)
{
	// Only generated resources are supported for resource manager not backed by an IO server
	if (!pIO) return nullptr;

	IO::PStream Stream;

	pOutSubId = strchr(pUID, '#');
	if (pOutSubId)
	{
		if (pOutSubId == pUID) return nullptr;

		CString Path(pUID, pOutSubId - pUID);
		Stream = pIO->CreateStream(Path);

		++pOutSubId; // Skip '#'
		if (*pOutSubId == 0) pOutSubId = nullptr;
	}
	else Stream = pIO->CreateStream(pUID);

	return (Stream && Stream->CanRead()) ? Stream : nullptr;
}
//---------------------------------------------------------------------

}