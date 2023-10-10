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

	struct CLightInfo
	{
		CLightAttribute* pLightAttr = nullptr; //???need light's scene record instead to access bounds? or real shape is enough? or store intersection record?
		U32              BoundsVersion = 0; // Light's last bounds version for which the coverage was calculated
		// list of LOD0 node indices (quadtree Morton) //  Affected quadtree nodes of the finest LOD 0
	};

	struct CQuadTreeNode
	{
		std::set<CLightAttribute*> Lights; //???or store UIDs?!
	};

	Render::PCDLODData _CDLODData;
	CStrID             _MaterialUID;
	CStrID             _CDLODDataUID;
	CStrID             _HeightMapUID;
	float              _InvSplatSizeX = 1.f;
	float              _InvSplatSizeZ = 1.f;
	U32                _LightCacheBoundsVersion = 0;        // Renderable bounds version for which _Lights is updated
	U16                _LightCacheIntersectionsVersion = 0; // Object-light intersections version for which _Lights is updated

	// Cached information about lights affecting parts of the terrain
	std::map<UPTR, CLightInfo>             _Lights; // UID -> Light info
	std::unordered_map<U32, CQuadTreeNode> _Nodes;  // Morton code -> Light list

	bool UpdateLightInQuadTree(const CLightAttribute* pLightAttr, bool NewLight);
	bool UpdateLightInQuadTreeNode(const CLightAttribute* pLightAttr, bool NewLight, U32 MortonCode);

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
