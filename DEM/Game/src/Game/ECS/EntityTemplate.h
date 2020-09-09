#pragma once
#include <Resources/ResourceObject.h>
#include <Data/Params.h>

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

	virtual bool IsResourceValid() const override { return _Desc.GetCount() > 0; }

	const Data::CParams& GetDesc() const { return _Desc; }
};

typedef Ptr<CEntityTemplate> PEntityTemplate;

}
