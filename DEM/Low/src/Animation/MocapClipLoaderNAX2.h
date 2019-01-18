#pragma once
#ifndef __DEM_L1_MOCAP_CLIP_LOADER_NAX2_H__
#define __DEM_L1_MOCAP_CLIP_LOADER_NAX2_H__

#include <Resources/ResourceCreator.h>

// Loads motion capture animation clip from The Nebula Device 2 'nax2' format

namespace Render
{
	typedef Ptr<class CSkinInfo> PSkinInfo;
}

namespace Resources
{

class CMocapClipLoaderNAX2: public IResourceCreator
{
	__DeclareClass(CMocapClipLoaderNAX2);

public:

	Render::PSkinInfo ReferenceSkinInfo; // NAX2 is bound to a particular skin

	CMocapClipLoaderNAX2();
	virtual ~CMocapClipLoaderNAX2();

	virtual const Core::CRTTI&			GetResultType() const;
	virtual IO::EStreamAccessPattern	GetStreamAccessPattern() const { return IO::SAP_SEQUENTIAL; }
	virtual PResourceObject				Load(IO::CStream& Stream);
};

typedef Ptr<CMocapClipLoaderNAX2> PMocapClipLoaderNAX2;

}

#endif
