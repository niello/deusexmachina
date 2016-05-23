#pragma once
#ifndef __DEM_L1_KEYFRAME_CLIP_LOADER_KFA_H__
#define __DEM_L1_KEYFRAME_CLIP_LOADER_KFA_H__

#include <Resources/ResourceLoader.h>

// Loads keyframed animation clip from a native DEM 'kfa' format

namespace Resources
{

class CKeyframeClipLoaderKFA: public CResourceLoader
{
	__DeclareClass(CKeyframeClipLoaderKFA);

public:

	//virtual ~CKeyframeClipLoaderKFA() {}

	virtual const Core::CRTTI&			GetResultType() const;
	virtual bool						IsProvidedDataValid() const { OK; } //!!!implement properly!
	virtual IO::EStreamAccessPattern	GetStreamAccessPattern() const { return IO::SAP_SEQUENTIAL; }
	virtual PResourceObject				Load(IO::CStream& Stream);
};

typedef Ptr<CKeyframeClipLoaderKFA> PKeyframeClipLoaderKFA;

}

#endif
