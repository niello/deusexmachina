#pragma once
#include <Resources/ResourceObject.h>
#include <Data/SerializeToParams.h>

// A preset of components used for entity creation

namespace DEM::Game
{

class CEntityTemplate: public Resources::CResourceObject
{
	RTTI_CLASS_DECL;

protected:

	Data::CParams _Desc;

public:

	CEntityTemplate(Data::CParams&& Desc) : _Desc(std::move(Desc)) {}
	virtual ~CEntityTemplate() override = default;

	virtual bool IsResourceValid() const override { return _Desc.GetCount() > 0; }

	const Data::CParams& GetDesc() const { return _Desc; }

	//???is needed? duplicate logic, component storages already implement it in LoadComponentFromParams!
	//!!!if remove, need to provide component loading WITHOUT registration in the storage!
	// NB: Out must be default-initialized, because HRD may store diff only
	template<typename T> bool GetComponent(CStrID ComponentID, T& Out)
	{
		Data::CData* pData;
		if (!_Desc.TryGet(pData, ComponentID)) return false;

		if constexpr (std::is_empty_v<T>)
		{
			// Empty component can be represented as a boolean or an empty section
			if (auto pBoolData = pData->As<bool>()) return *pBoolData;
			else if (auto pParamsData = pData->As<Data::PParams>()) return (*pParamsData).IsValidPtr();
		}
		else if constexpr (DEM::Meta::CMetadata<T>::IsRegistered)
		{
			DEM::ParamsFormat::Deserialize(*pData, Out);
			return true;
		}

		return false;
	}
};

typedef Ptr<CEntityTemplate> PEntityTemplate;

}
