#pragma once
#include <Core/Object.h>

// Sampler state describes how texture will be sampled in shaders

namespace Render
{

class CSampler: public DEM::Core::CObject
{
	//RTTI_CLASS_DECL;

public:

};

typedef Ptr<CSampler> PSampler;

}
