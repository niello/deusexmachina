#pragma once
#ifndef __DEM_L1_MOCAP_CLIP_LOADER_NAX2_H__
#define __DEM_L1_MOCAP_CLIP_LOADER_NAX2_H__

#include <Resources/ResourceLoader.h>

// Loads motion capture animation clip from The Nebula Device 2 'nax2' format

namespace Render
{
	typedef Ptr<class CSkinInfo> PSkinInfo;
}

namespace Resources
{

class CMocapClipLoaderNAX2: public CResourceLoader
{
public:

	Render::PSkinInfo ReferenceSkinInfo; // NAX2 is bound to a particular skin

	CMocapClipLoaderNAX2(CResourceManager& ResourceManager);
	virtual ~CMocapClipLoaderNAX2();

	virtual const Core::CRTTI&			GetResultType() const override;
	virtual PResourceObject				CreateResource(CStrID UID) override;
};

typedef Ptr<CMocapClipLoaderNAX2> PMocapClipLoaderNAX2;

}

#endif
