#pragma once
#ifndef __DEM_L1_RENDER_SAMPLER_H__
#define __DEM_L1_RENDER_SAMPLER_H__

#include <Core/Object.h>

// Sampler state that describes how texture will be sampled in shaders

namespace Render
{

class CSampler: public Core::CObject
{
	//__DeclareClassNoFactory;

public:

};

typedef Ptr<CSampler> PSampler;

}

#endif
