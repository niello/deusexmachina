#include "ToolNavRegions.h"

#include <Input/InputServer.h>
#include <Input/Events/MouseBtnDown.h>
#include <Input/Events/MouseBtnUp.h>
#include <Game/Mgr/EnvQueryManager.h>
#include <App/CIDEApp.h>
#include <gfx2/ngfxserver2.h>
#include <AI/Navigation/NavMeshDebugDraw.h>

namespace App
{
//ImplementRTTI(App::CToolNavRegions, App::IEditorTool);

int CToolNavRegions::ConvexUID = 0; //!!!Init to the max existing UID + 1 on level nav data loaded!

// Returns true if 'c' is left of line 'a'-'b'.
inline bool left(const float* a, const float* b, const float* c)
{ 
	const float u1 = b[0] - a[0];
	const float v1 = b[2] - a[2];
	const float u2 = c[0] - a[0];
	const float v2 = c[2] - a[2];
	return u1 * v2 - v1 * u2 < 0;
}
//---------------------------------------------------------------------

// Returns true if 'a' is more lower-left than 'b'.
inline bool cmppt(const float* a, const float* b)
{
	if (a[0] < b[0]) return true;
	if (a[0] > b[0]) return false;
	if (a[2] < b[2]) return true;
	if (a[2] > b[2]) return false;
	return false;
}
//---------------------------------------------------------------------

static int ConvexHull(const vector3* pts, int npts, int* out)
{
	// Find lower-leftmost point.
	int hull = 0;
	for (int i = 1; i < npts; ++i)
		if (cmppt(pts[i].v, pts[hull].v))
			hull = i;
	// Gift wrap hull.
	int endpt = 0;
	int i = 0;
	do
	{
		out[i++] = hull;
		endpt = 0;
		for (int j = 1; j < npts; ++j)
			if (hull == endpt || left(pts[hull].v, pts[endpt].v, pts[j].v))
				endpt = j;
		hull = endpt;
	}
	while (endpt != out[0]);
	
	return i;
}
//---------------------------------------------------------------------

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
	if (Ev.Button == Input::MBLeft)
	{
		MBDownX = Ev.X;
		MBDownY = Ev.Y;
	}
	FAIL;
}
//---------------------------------------------------------------------

bool CToolNavRegions::OnClick(const Events::CEventBase& Event)
{
	const Event::MouseBtnUp& Ev = ((const Event::MouseBtnUp&)Event);

	const vector3& Pt = EnvQueryMgr->GetMousePos3D();

	if (Ev.Button == Input::MBLeft) // Create
	{
		// Check mouse movement between down and up, for camera handling
		int DiffX = Ev.X - MBDownX;
		int DiffY = Ev.Y - MBDownY;
		if (DiffX * DiffX > 2 * 2 || DiffX * DiffX > 2 * 2) FAIL;

		// If clicked on that last pt, create the shape.
		if (PointCount && vector3::SqDistance(Pt, Points[PointCount - 1]) < 0.04f) // 0.2 * 0.2
		{
			if (HullSize > 2) // Create shape.
			{
				CConvexVolume& Vol = *CIDEApp->ConvexVolumes.Reserve(1);
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
				Vol.UID = ConvexUID++;

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
		else
		{
			// Add new point 
			if (PointCount < MAX_CONVEXVOL_PTS)
			{
				Points[PointCount] = Pt;
				++PointCount;
				HullSize = (PointCount > 1) ? ConvexHull(Points, PointCount, Hull) : 0;
			}
		}
	}
	else if (Ev.Button == Input::MBMiddle) // Delete
	{
		int NearestIdx = -1;
		for (int i = 0; i < CIDEApp->ConvexVolumes.Size(); ++i)
		{
			CConvexVolume& Vol = CIDEApp->ConvexVolumes[i];
			if (IsPointInPoly(Vol.VertexCount, Vol.Vertices, Pt) && Pt.y >= Vol.MinY && Pt.y <= Vol.MaxY)
				NearestIdx = i;
		}
		if (NearestIdx != -1) CIDEApp->ConvexVolumes.Erase(NearestIdx);
	}

	OK;
}
//---------------------------------------------------------------------

// Partly copied from InputGeom::drawConvexVolumes
void CToolNavRegions::Render()
{
	nGfxServer2::Instance()->BeginShapes();

	AI::CNavMeshDebugDraw DD;

	// Created

	DD.depthMask(false);

	DD.begin(DU_DRAW_TRIS);
	for (int i = 0; i < CIDEApp->ConvexVolumes.Size(); ++i)
	{
		CConvexVolume& Vol = CIDEApp->ConvexVolumes[i];
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
	for (int i = 0; i < CIDEApp->ConvexVolumes.Size(); ++i)
	{
		CConvexVolume& Vol = CIDEApp->ConvexVolumes[i];
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
	for (int i = 0; i < CIDEApp->ConvexVolumes.Size(); ++i)
	{
		CConvexVolume& Vol = CIDEApp->ConvexVolumes[i];
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

	nGfxServer2::Instance()->EndShapes();
}
//---------------------------------------------------------------------

}