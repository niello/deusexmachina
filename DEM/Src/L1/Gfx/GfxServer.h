#pragma once
#ifndef __DEM_L2_GFX_SERVER_H__
#define __DEM_L2_GFX_SERVER_H__

#include <Data/Singleton.h>
#include <Data/TTable.h>
#include <Gfx/Level.h>
#include <Gfx/Entity.h>
#include <Time/TimeSource.h>
#include <gfx2/DisplayMode.h>
#include <util/ndictionary.h>

// The Graphics::CGfxServer is the central object of the graphics subsystem.
// The tasks of the graphics subsystem are:
// - display initialization and management
// - optimal rendering of graphics entities
// - culling
// The lowlevel task of rendering and resource management is left to Nebula.

class nGfxServer2;
class nSceneServer;
class nVariableServer;
class nParticleServer;
class nParticleServer2;

namespace Graphics
{
class CResource;
typedef Ptr<class CShapeEntity> PShapeEntity;

#define GfxSrv Graphics::CGfxServer::Instance()

struct CAnimInfo
{
	nString	Name;
	float	HotSpotTime;

	CAnimInfo(): HotSpotTime(0.f) {}
};

class CGfxServer: public Core::CRefCounted
{
	DeclareRTTI;
	__DeclareSingleton(CGfxServer);

private:

	friend class CLightEntity;
	friend class CEntity;
	
	static const float MaxLODDistThreshold;
	static const float MinLODSizeThreshold;

	bool								_IsOpen;
	Ptr<CLevel>							CurrLevel;
	Data::CTTable<nArray<CAnimInfo>>	AnimTable;

	nRef<nGfxServer2>       gfxServer;
	nRef<nSceneServer>      sceneServer;
	nRef<nVariableServer>   variableServer;
	nRef<nParticleServer>   particleServer;
	nRef<nParticleServer2>  particleServer2;

	nRef<nRoot>							GfxRoot;

	uint								FrameID;	// an unique FrameID

	nDictionary<const Core::CRTTI*, float> LODDistThresholds;
	nDictionary<const Core::CRTTI*, float> LODSizeThresholds;

public:

	CDisplayMode		DisplayMode;
	nString				WindowTitle;
	nString				WindowIcon;
	nString				RenderPath;
	nString				FeatureSet;
	Time::PTimeSource	EntityTimeSrc;

	CGfxServer();
	virtual ~CGfxServer();

	bool				Open();
	void				Close();
	void				Trigger();
	bool				BeginRender();
	void				Render();
	void				RenderDebug();
	void				EndRender();
	void				DragDropSelect(const vector3& lookAt, float angleOfView, float aspectRatio, nArray<PEntity>& entities);
	void				CreateGfxEntities(const nString& RsrcName, const matrix44& World, nArray<PShapeEntity>& OutEntities);

	//!!!AnimSet to CStrID!
	const CAnimInfo&	GetAnimationName(const nString& AnimSet, const nString& AnimName, bool Random = true);

	nRoot*				GetGfxRoot() const { return GfxRoot.get(); }
	//???!!!inline?
	void				GetRelativeXY(int XAbs, int YAbs, float& XRel, float& YRel) const;

	bool				IsOpen() const { return _IsOpen; }
	void				SetLevel(CLevel* pLevel) { CurrLevel = pLevel; }
	CLevel*				GetLevel() const { return CurrLevel.get_unsafe(); }
	CCameraEntity*		GetCamera() const { return CurrLevel.isvalid() ? CurrLevel->GetCamera() : NULL; }
	uint				GetFrameID() const { return FrameID; }
	void				SetDistThreshold(const Core::CRTTI* pType, float MaxDist);
	float				GetDistThreshold(const Core::CRTTI* pType) const;
	void				SetSizeThreshold(const Core::CRTTI* pType, float MinSize);
	float				GetSizeThreshold(const Core::CRTTI* pType) const;
};
//---------------------------------------------------------------------

inline void CGfxServer::SetDistThreshold(const Core::CRTTI* pType, float MaxDist)
{
	if (!LODDistThresholds.Contains(pType)) LODDistThresholds.Add(pType, MaxDist);
	else LODDistThresholds[pType] = MaxDist;
}
//---------------------------------------------------------------------

inline float CGfxServer::GetDistThreshold(const Core::CRTTI* pType) const
{
	int Idx = LODDistThresholds.FindIndex(pType);
	return (Idx == INVALID_INDEX) ? MaxLODDistThreshold : LODDistThresholds.ValueAtIndex(Idx);
}
//---------------------------------------------------------------------

inline void CGfxServer::SetSizeThreshold(const Core::CRTTI* pType, float MinSize)
{
	if (!LODSizeThresholds.Contains(pType)) LODSizeThresholds.Add(pType, MinSize);
	else LODSizeThresholds[pType] = MinSize;
}
//---------------------------------------------------------------------

inline float CGfxServer::GetSizeThreshold(const Core::CRTTI* pType) const
{
	int Idx = LODSizeThresholds.FindIndex(pType);
	return (Idx == INVALID_INDEX) ? MinLODSizeThreshold : LODSizeThresholds.ValueAtIndex(Idx);
}
//---------------------------------------------------------------------

}

#endif
