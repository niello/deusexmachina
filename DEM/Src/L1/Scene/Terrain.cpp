#include "Terrain.h"

#include <Scene/SceneNode.h>

namespace Scene
{
ImplementRTTI(Scene::CTerrain, Scene::CSceneNodeAttr);
ImplementFactory(Scene::CTerrain);

bool CTerrain::LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader)
{
	switch (FourCC)
	{
		case 'XXXX': // XXXX
		{
			//DataReader.Read();
			//OK;
			FAIL;
		}
		default: FAIL;
	}
}
//---------------------------------------------------------------------

//???or in Init() / LoadResources()?
bool CTerrain::OnAdd()
{
	// create or load precreated minmax maps for different LOD levels
	// Hm, top LOD may not be a whole quad?

	// get or generate patch and quarterpatch meshes
	// if it is more effective, can use one geometry and play with primitive count to render

	OK;
}
//---------------------------------------------------------------------

//!!!main camera must be updated!
void CTerrain::Update()
{
	if (pNode->IsWorldMatrixChanged())
	{
		//pNode->GetWorldMatrix();
	}
}
//---------------------------------------------------------------------

}