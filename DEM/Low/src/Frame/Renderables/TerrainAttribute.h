#pragma once
#include <Frame/RenderableAttribute.h>

// Scene node attribute with renderable terrain

namespace Render
{
	typedef Ptr<class CCDLODData> PCDLODData;
}

namespace Frame
{
class CLightAttribute;

class CTerrainAttribute: public CRenderableAttribute
{
	FACTORY_CLASS_DECL;

protected:

	using TMorton = U32;
	using TCellDim = U16; // Max 15 bits for quadtree which is more than enough for terrain subdivision

	struct CLightInfo
	{
		CLightAttribute*     pLightAttr = nullptr; //???need light's scene record instead to access bounds? or real shape is enough? or store intersection record? or UID?
		U32                  BoundsVersion = 0; // Light's last bounds version for which the coverage was calculated
		std::vector<TMorton> AffectedNodes;     // Sorted. Insertion and removal are not needed, so vector is the best choice.
	};

	struct CQuadTreeNode
	{
		U32            Version = 0; // Version of data in this node. Used for eliminating redundant synchronization with renderable views.
		std::set<UPTR> LightUIDs;
	};

	struct CNodeProcessingContext
	{
		rtm::vector4f Scale;
		rtm::vector4f Offset;
		CLightInfo*     pLightInfo;
	};

	Render::PCDLODData _CDLODData;
	CStrID             _MaterialUID;
	CStrID             _CDLODDataUID;
	CStrID             _HeightMapUID;
	float              _InvSplatSizeX = 1.f;
	float              _InvSplatSizeZ = 1.f;
	U32                _LightCacheBoundsVersion = 0;        // Renderable bounds version for which _Lights is updated
	U16                _LightCacheIntersectionsVersion = 0; // Object-light intersections version for which _Lights is updated
	U16                _MaxLODForDynamicLights = 3;         // LOD 0 can't be disabled on intent. TODO: read from attr settings

	// Cached information about lights affecting parts of the terrain
	std::map<UPTR, CLightInfo>                 _Lights; // Light UID -> Info
	std::unordered_map<TMorton, CQuadTreeNode> _Nodes;  // Morton code -> List of lights affecting the node

	std::vector<TMorton>                       _PrevAffectedNodes; // Stored here to avoid per frame allocations

	void StartAffectingNode(TMorton NodeCode, UPTR LightUID);
	void StopAffectingNode(TMorton NodeCode, UPTR LightUID);
	bool UpdateLightInQuadTreeNode(const CNodeProcessingContext& Ctx, TCellDim x, TCellDim z, U32 LOD);

	virtual void OnActivityChanged(bool Active) override;

public:

	virtual bool                  LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count) override;
	virtual Scene::PNodeAttribute Clone() override;
	virtual bool                  ValidateResources(Resources::CResourceManager& ResMgr) override;
	virtual Render::PRenderable   CreateRenderable() const override;
	virtual void                  UpdateRenderable(CView& View, Render::IRenderable& Renderable, bool ViewProjChanged) const override;
	virtual void                  UpdateLightList(CView& View, Render::IRenderable& Renderable, const CObjectLightIntersection* pHead) const override;
	virtual void                  OnLightIntersectionsUpdated() override;
	virtual U8                    GetLightTrackingFlags() const override { return TrackLightContactChanges | TrackLightRelativeMovement; }
	virtual bool                  GetLocalAABB(CAABB& OutBox, UPTR LOD = 0) const override;
};

typedef Ptr<CTerrainAttribute> PTerrainAttribute;

}
