#pragma once
#include <Core/Object.h>
#include <Data/Params.h>

// A preset of components used for entity creation

namespace DEM::Game
{

class CEntityTemplate: public DEM::Core::CObject
{
	RTTI_CLASS_DECL(DEM::Game::CEntityTemplate, DEM::Core::CObject);

protected:

	Data::CParams _Desc;

public:

	CEntityTemplate(Data::CParams&& Desc) : _Desc(std::move(Desc)) {}

	const Data::CParams& GetDesc() const { return _Desc; }
};

typedef Ptr<CEntityTemplate> PEntityTemplate;

}
