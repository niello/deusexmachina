#include "ResourceManager.h"

#include <Resources/Resource.h>
#include <Resources/ResourceCreator.h>
#include <IO/PathUtils.h>
#include <IO/IOServer.h>
#include <IO/Stream.h>
#include <Data/StringUtils.h>

namespace Resources
{

CResourceManager::CResourceManager(IO::CIOServer* pIOServer, UPTR InitialCapacity)
	: Registry(InitialCapacity)
	, pIO(pIOServer)
{
}
//---------------------------------------------------------------------

CResourceManager::~CResourceManager() = default;
//---------------------------------------------------------------------

// NB: does not change creator for existing resource
PResource CResourceManager::RegisterResource(const char* pUID, const DEM::Core::CRTTI& RsrcType)
{
	if (!pUID || !*pUID) return nullptr;

	const CStrID UID = pIO ? CStrID(pIO->ResolveAssigns(pUID).c_str()): CStrID(pUID);
	auto It = Registry.find(UID);
	if (It != Registry.cend()) return It->second;

	auto pCreator = GetDefaultCreator(UID, RsrcType).Get();
	if (!pCreator) return nullptr;

	PResource Rsrc = n_new(CResource(UID));
	Rsrc->SetCreator(pCreator);
	Registry.emplace(UID, Rsrc);
	return Rsrc;
}
//---------------------------------------------------------------------

// NB: does not change creator for existing resource
void CResourceManager::RegisterResource(PResource& Resource, const DEM::Core::CRTTI& RsrcType)
{
	if (!Resource || !Resource->GetUID())
	{
		Resource = nullptr;
		return;
	}

	const CStrID UID = pIO ? CStrID(pIO->ResolveAssigns(Resource->GetUID().CStr()).c_str()) : Resource->GetUID();
	auto It = Registry.find(UID);
	if (It != Registry.cend())
	{
		Resource = It->second;
		return;
	}

	IResourceCreator* pCreator = Resource->GetCreator();
	if (!pCreator)
	{
		pCreator = GetDefaultCreator(UID, RsrcType).Get();
		if (!pCreator)
		{
			Resource = nullptr;
			return;
		}
	}

	if (Resource->GetUID() != UID)
		Resource = n_new(CResource(UID));
	Resource->SetCreator(pCreator);
	Registry.emplace(UID, Resource);
}
//---------------------------------------------------------------------

// NB: does not change creator for existing resource
PResource CResourceManager::RegisterResource(const char* pUID, IResourceCreator* pCreator)
{
	if (!pUID || !*pUID) return nullptr;

	CStrID UID = pIO ? CStrID(pIO->ResolveAssigns(pUID).c_str()): CStrID(pUID);
	auto It = Registry.find(UID);
	if (It != Registry.cend()) return It->second;

	PResource Rsrc = n_new(CResource(UID));
	Rsrc->SetCreator(pCreator);
	Registry.emplace(UID, Rsrc);
	return Rsrc;
}
//---------------------------------------------------------------------

CResource* CResourceManager::FindResource(const char* pUID) const
{
	return FindResource(pIO ? CStrID(pIO->ResolveAssigns(pUID)): CStrID(pUID));
}
//---------------------------------------------------------------------

CResource* CResourceManager::FindResource(CStrID UID) const
{
	auto It = Registry.find(UID);
	return (It != Registry.cend()) ? It->second.Get() : nullptr;
}
//---------------------------------------------------------------------

void CResourceManager::UnregisterResource(const char* pUID)
{
	return UnregisterResource(pIO ? CStrID(pIO->ResolveAssigns(pUID)): CStrID(pUID));
}
//---------------------------------------------------------------------

//???need const char* variant?
void CResourceManager::UnregisterResource(CStrID UID)
{
	Registry.erase(UID);
	//???force unload resource object here even if there are references to resource?
}
//---------------------------------------------------------------------

bool CResourceManager::RegisterDefaultCreator(const char* pFmtExtension, const DEM::Core::CRTTI* pRsrcType, IResourceCreator* pCreator)
{
	if (pCreator &&	pRsrcType && !pCreator->GetResultType().IsDerivedFrom(*pRsrcType)) FAIL;

	std::string ExtStr(StringUtils::Trim(pFmtExtension));
	std::transform(ExtStr.begin(), ExtStr.end(), ExtStr.begin(), [](unsigned char c) { return std::tolower(c); });
	CStrID Ext(ExtStr);

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

PResourceCreator CResourceManager::GetDefaultCreator(CStrID UID, const DEM::Core::CRTTI& RsrcType) const
{
	std::string Ext(PathUtils::GetExtension(UID.CStr()));
	if (!Ext.empty())
	{
		IPTR SharpIdx = Ext.find('#');
		if (SharpIdx != std::string::npos)
			Ext.resize(SharpIdx);
	}

	return GetDefaultCreator(Ext.c_str(), &RsrcType);
}
//---------------------------------------------------------------------

PResourceCreator CResourceManager::GetDefaultCreator(const char* pFmtExtension, const DEM::Core::CRTTI* pRsrcType) const
{
	std::string ExtStr(StringUtils::Trim(pFmtExtension));
	std::transform(ExtStr.begin(), ExtStr.end(), ExtStr.begin(), [](unsigned char c) { return std::tolower(c); });
	CStrID Ext(ExtStr);

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
		if (pRsrcType)
			Sys::Error("CResourceManager::GetDefaultCreator() > no record for extension '{}' and type '{}'\n"_format(Ext, pRsrcType->GetName()));
		else
			Sys::Error("CResourceManager::GetDefaultCreator() > no record for extension '{}'\n"_format(Ext));
		return nullptr;
	}

	return pRec->Creator;
}
//---------------------------------------------------------------------

IO::PStream CResourceManager::CreateResourceStream(const char* pUID, const char*& pOutSubId, IO::EStreamAccessPattern Pattern)
{
	ZoneScoped;

	// Only generated resources are supported for resource manager not backed by an IO server
	if (!pIO) return nullptr;

	IO::PStream Stream;

	pOutSubId = strchr(pUID, '#');
	if (pOutSubId)
	{
		if (pOutSubId == pUID) return nullptr;

		std::string Path(pUID, pOutSubId - pUID);
		Stream = pIO->CreateStream(Path.c_str(), IO::SAM_READ, Pattern);

		++pOutSubId; // Skip '#'
		if (*pOutSubId == 0) pOutSubId = nullptr;
	}
	else Stream = pIO->CreateStream(pUID, IO::SAM_READ, Pattern);

	return (Stream && Stream->CanRead()) ? Stream : nullptr;
}
//---------------------------------------------------------------------

}
