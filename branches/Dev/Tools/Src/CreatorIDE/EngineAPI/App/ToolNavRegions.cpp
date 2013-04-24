#include "ToolNavRegions.h"

#include <Input/InputServer.h>
#include <Input/Events/MouseBtnDown.h>
#include <Input/Events/MouseBtnUp.h>
#include <Game/Mgr/EnvQueryManager.h>
#include <App/CIDEApp.h>
#include <AI/Navigation/NavMeshDebugDraw.h>

namespace App
{
//ImplementRTTI(App::CToolNavRegions, App::IEditorTool);

static int IsPointInPoly(int nvert, const vector3* verts, const vector3& p)
{
	int i, j, c = 0;
	for (i = 0, j = nvert-1; i < nvert; j = i++)
	{
		const float* vi = verts[i].v;
		const float* vj = verts[j].v;
		if (((vi[2] > p.z) != (vj[2] > p.z)) &&
			(p.x < (vj[0]-vi[0]) * (p.z-vi[2]) / (vj[2]-vi[2]) + vi[0]) )
			c = !c;
	}
	return c;
}
//---------------------------------------------------------------------

void CToolNavRegions::Activate()
{
	MBDownX = MBDownY = -50000;
	SUBSCRIBE_INPUT_EVENT(MouseBtnDown, CToolNavRegions, OnMBDown, Input::InputPriority_Mapping);
	SUBSCRIBE_INPUT_EVENT(MouseBtnUp, CToolNavRegions, OnClick, Input::InputPriority_Mapping);
}
//---------------------------------------------------------------------

void CToolNavRegions::Deactivate()
{
	UNSUBSCRIBE_EVENT(MouseBtnDown);
	UNSUBSCRIBE_EVENT(MouseBtnUp);
}
//---------------------------------------------------------------------

bool CToolNavRegions::OnMBDown(const Events::CEventBase& Event)
{
	const Event::MouseBtnDown& Ev = ((const Event::MouseBtnDown&)Event);
	MBDownX = Ev.X;
	MBDownY = Ev.Y;
	FAIL;
}
//---------------------------------------------------------------------

bool CToolNavRegions::OnClick(const Events::CEventBase& Event)
{
	const Event::MouseBtnUp& Ev = ((const Event::MouseBtnUp&)Event);

	// Check mouse movement between down and up, for camera handling
	int DiffX = Ev.X - MBDownX;
	int DiffY = Ev.Y - MBDownY;
	if (DiffX * DiffX > 2 * 2 || DiffX * DiffX > 2 * 2) FAIL;

	const vector3& Pt = EnvQueryMgr->GetMousePos3D();

	if (Ev.Button == Input::MBLeft) // Create
	{
		// If clicked on that last pt, create the shape.
		if (PointCount && vector3::SqDistance(Pt, Points[PointCount - 1]) < 0.01f) // 0.1 * 0.1
		{
			if (HullSize > 2) // Create shape
			{
				CConvexVolume& Vol = *CIDEApp->CurrLevel.ConvexVolumes.Reserve(1);
				Vol.MinY = FLT_MAX;
				Vol.VertexCount = HullSize;

				for (int i = 0; i < HullSize; ++i)
				{
					Vol.Vertices[i] = Points[Hull[i]];
					Vol.MinY = n_min(Vol.MinY, Vol.Vertices[i].y);
				}

				Vol.MinY -= BoxDescent;
				Vol.MaxY = Vol.MinY + BoxHeight;
				Vol.Area = Area;

				if (!RegionID.IsValid())
					RegionID = CStrID(CIDEApp->GetStringInput(RegionID.CStr()).Get());

				Vol.ID = RegionID;

				RegionID = CStrID::Empty;

				CIDEApp->CurrLevel.ConvexChanged = true;

				//if (PolyOffset > 0.01f)
				//{
				//	float offset[MAX_PTS * 2 * 3];
				//	int noffset = rcOffsetPoly(Vertices, HullSize, PolyOffset, offset, MAX_PTS*2);
				//	if (noffset > 0) geom->addConvexVolume(offset, noffset, MinY, MaxY, Area);
				//}
			}
			
			PointCount = 0;
			HullSize = 0;
		}
		else // Add new point
		{
			if (PointCount < MAX_CONVEXVOL_PTS)
			{
				Points[PointCount] = Pt;
				++PointCount;
				HullSize = (PointCount > 1) ? ConvexHull(Points, PointCount, Hull) : 0;
			}
		}
	}
	else
	{
		CConvexVolume* pNearest = NULL;
		for (int i = 0; i < CIDEApp->CurrLevel.ConvexVolumes.Size(); ++i)
		{
			CConvexVolume& Vol = CIDEApp->CurrLevel.ConvexVolumes[i];
			if (IsPointInPoly(Vol.VertexCount, Vol.Vertices, Pt) && Pt.y >= Vol.MinY && Pt.y <= Vol.MaxY)
				pNearest = &Vol;
		}

		if (pNearest)
		{
			if (Ev.Button == Input::MBMiddle) // Delete
			{
				CIDEApp->CurrLevel.ConvexVolumes.Erase(pNearest);
				CIDEApp->CurrLevel.ConvexChanged = true;
			}
			else if (Ev.Button == Input::MBRight) // Name
			{
				CStrID Old = pNearest->ID;
				pNearest->ID = CStrID(CIDEApp->GetStringInput(Old.CStr()).Get());
				if (Old != pNearest->ID)
					CIDEApp->CurrLevel.ConvexIDChanged = true;
			}
		}
	}

	OK;
}
//---------------------------------------------------------------------

// Partially copied from InputGeom::drawConvexVolumes
void CToolNavRegions::Render()
{
	AI::CNavMeshDebugDraw DD;

	// Created

	DD.depthMask(false);

	DD.begin(DU_DRAW_TRIS);
	for (int i = 0; i < CIDEApp->CurrLevel.ConvexVolumes.Size(); ++i)
	{
		CConvexVolume& Vol = CIDEApp->CurrLevel.ConvexVolumes[i];
		unsigned int Color = duIntToCol((int)Vol.Area, 32);
		for (int j = 0, k = Vol.VertexCount - 1; j < Vol.VertexCount; k = j++)
		{
			const float* va = Vol.Vertices[k].v;
			const float* vb = Vol.Vertices[j].v;
			DD.vertex(Vol.Vertices[0].x, Vol.MaxY, Vol.Vertices[0].z, Color);
			DD.vertex(vb[0], Vol.MaxY, vb[2], Color);
			DD.vertex(va[0], Vol.MaxY, va[2], Color);
			DD.vertex(va[0], Vol.MinY, va[2], duDarkenCol(Color));
			DD.vertex(va[0], Vol.MaxY, va[2], Color);
			DD.vertex(vb[0], Vol.MaxY, vb[2], Color);
			DD.vertex(va[0], Vol.MinY, va[2], duDarkenCol(Color));
			DD.vertex(vb[0], Vol.MaxY, vb[2], Color);
			DD.vertex(vb[0], Vol.MinY, vb[2], duDarkenCol(Color));
		}
	}
	DD.end();

	DD.begin(DU_DRAW_LINES, 2.0f);
	for (int i = 0; i < CIDEApp->CurrLevel.ConvexVolumes.Size(); ++i)
	{
		CConvexVolume& Vol = CIDEApp->CurrLevel.ConvexVolumes[i];
		unsigned int Color = duIntToCol(Vol.Area, 220);
		for (int j = 0, k = Vol.VertexCount - 1; j < Vol.VertexCount; k = j++)
		{
			const float* va = Vol.Vertices[k].v;
			const float* vb = Vol.Vertices[j].v;
			DD.vertex(va[0], Vol.MinY,va[2], duDarkenCol(Color));
			DD.vertex(vb[0], Vol.MinY,vb[2], duDarkenCol(Color));
			DD.vertex(va[0], Vol.MaxY,va[2], Color);
			DD.vertex(vb[0], Vol.MaxY,vb[2], Color);
			DD.vertex(va[0], Vol.MinY,va[2], duDarkenCol(Color));
			DD.vertex(va[0], Vol.MaxY,va[2], Color);
		}
	}
	DD.end();

	DD.begin(DU_DRAW_POINTS, 3.0f);
	for (int i = 0; i < CIDEApp->CurrLevel.ConvexVolumes.Size(); ++i)
	{
		CConvexVolume& Vol = CIDEApp->CurrLevel.ConvexVolumes[i];
		unsigned int Color = duDarkenCol(duIntToCol(Vol.Area, 255));
		for (int j = 0; j < Vol.VertexCount; ++j)
		{
			DD.vertex(Vol.Vertices[j].x, Vol.Vertices[j].y + 0.1f, Vol.Vertices[j].z, Color);
			DD.vertex(Vol.Vertices[j].x, Vol.MinY, Vol.Vertices[j].z, Color);
			DD.vertex(Vol.Vertices[j].x, Vol.MaxY, Vol.Vertices[j].z, Color);
		}
	}
	DD.end();

	DD.depthMask(true);

	// Current

	if (PointCount > 0)
	{
		float MinY = FLT_MAX, MaxY = 0;
		for (int i = 0; i < PointCount; ++i)
			MinY = n_min(MinY, Points[i].y);
		MinY -= BoxDescent;
		MaxY = MinY + BoxHeight;

		//!!!NEED line size!
		DD.begin(DU_DRAW_LINES, 2.0f);
		for (int i = 0, j = HullSize - 1; i < HullSize; j = i++)
		{
			const float* vi = Points[Hull[j]].v;
			const float* vj = Points[Hull[i]].v;
			DD.vertex(vj[0], MinY,vj[2], duRGBA(255, 255, 255, 64));
			DD.vertex(vi[0], MinY,vi[2], duRGBA(255, 255, 255, 64));
			DD.vertex(vj[0], MaxY,vj[2], duRGBA(255, 255, 255, 64));
			DD.vertex(vi[0], MaxY,vi[2], duRGBA(255, 255, 255, 64));
			DD.vertex(vj[0], MinY,vj[2], duRGBA(255, 255, 255, 64));
			DD.vertex(vj[0], MaxY,vj[2], duRGBA(255, 255, 255, 64));
		}
		DD.end();

		DD.begin(DU_DRAW_POINTS, 4.0f);
		for (int i = 0; i < PointCount; ++i)
		{
			uint Color = (i == PointCount - 1) ? duRGBA(240, 32, 16, 255) : duRGBA(255, 255, 255, 255);
			DD.vertex(Points[i].x, Points[i].y + 0.1f, Points[i].z, Color);
		}
		DD.end();
	}
}
//---------------------------------------------------------------------

}