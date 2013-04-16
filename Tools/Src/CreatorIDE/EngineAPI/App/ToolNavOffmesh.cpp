#include "ToolNavOffmesh.h"

#include <Input/InputServer.h>
#include <Input/Events/MouseBtnDown.h>
#include <Input/Events/MouseBtnUp.h>
#include <Game/Mgr/EnvQueryManager.h>
#include <App/CIDEApp.h>
#include <AI/Navigation/NavMeshDebugDraw.h>

namespace App
{
//ImplementRTTI(App::CToolNavOffmesh, App::IEditorTool);

void CToolNavOffmesh::Activate()
{
	MBDownX = MBDownY = -50000;
	SUBSCRIBE_INPUT_EVENT(MouseBtnDown, CToolNavOffmesh, OnMBDown, Input::InputPriority_Mapping);
	SUBSCRIBE_INPUT_EVENT(MouseBtnUp, CToolNavOffmesh, OnClick, Input::InputPriority_Mapping);
}
//---------------------------------------------------------------------

void CToolNavOffmesh::Deactivate()
{
	UNSUBSCRIBE_EVENT(MouseBtnDown);
	UNSUBSCRIBE_EVENT(MouseBtnUp);
}
//---------------------------------------------------------------------

bool CToolNavOffmesh::OnMBDown(const Events::CEventBase& Event)
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

bool CToolNavOffmesh::OnClick(const Events::CEventBase& Event)
{
	const Event::MouseBtnUp& Ev = ((const Event::MouseBtnUp&)Event);

	const vector3& Pt = EnvQueryMgr->GetMousePos3D();

	const float AgentRadius = 0.3f; //!!!m_sample->getAgentRadius();

	if (Ev.Button == Input::MBLeft) // Create
	{
		// Check mouse movement between down and up, for camera handling
		int DiffX = Ev.X - MBDownX;
		int DiffY = Ev.Y - MBDownY;
		if (DiffX * DiffX > 2 * 2 || DiffX * DiffX > 2 * 2) FAIL;

		if (!HitPosSet)
		{
			HitPos = Pt;
			HitPosSet = true;
		}
		else
		{
			COffmeshConnection& Conn = *CIDEApp->CurrLevel.OffmeshConnections.Reserve(1);
			Conn.From = HitPos;
			Conn.To = Pt;
			Conn.Radius = AgentRadius; //!!!SET!
			Conn.Bidirectional = Bidirectional;
			Conn.Area = Area;
			Conn.Flags = 1; //!!!SET! //???or in navmesh?
			HitPosSet = false;
			CIDEApp->CurrLevel.OffmeshChanged = true;
		}
	}
	else if (Ev.Button == Input::MBMiddle) // Delete
	{
		// Find nearest link end-point
		float NearestDist = FLT_MAX;
		int NearestIdx = -1;
		for (int i = 0; i < CIDEApp->CurrLevel.OffmeshConnections.Size(); ++i)
		{
			COffmeshConnection& Conn = CIDEApp->CurrLevel.OffmeshConnections[i];
			float Dist = vector3::SqDistance(Pt, Conn.From);
			if (Dist < NearestDist)
			{
				NearestDist = Dist;
				NearestIdx = i;
			}
			Dist = vector3::SqDistance(Pt, Conn.To);
			if (Dist < NearestDist)
			{
				NearestDist = Dist;
				NearestIdx = i;
			}
		}

		if (NearestIdx != -1 && sqrtf(NearestDist) < AgentRadius)
		{
			CIDEApp->CurrLevel.OffmeshConnections.Erase(NearestIdx);
			CIDEApp->CurrLevel.OffmeshChanged = true;
		}
	}

	OK;
}
//---------------------------------------------------------------------

// Partially copied from InputGeom::drawOffMeshConnections
void CToolNavOffmesh::Render()
{
	//GFX
	/*
	nGfxServer2::Instance()->BeginShapes();

	AI::CNavMeshDebugDraw DD;

	const float AgentRadius = 0.3f; //!!!m_sample->getAgentRadius();
	
	if (HitPosSet)
		duDebugDrawCross(&DD, HitPos.x, HitPos.y + 0.1f, HitPos.z, AgentRadius, duRGBA(0, 0, 0, 128), 2.0f);

	unsigned int ConnColor = duRGBA(192, 0, 128, 192);
	unsigned int BaseColor = duRGBA(0, 0, 0, 64);
	DD.depthMask(false);

	DD.begin(DU_DRAW_LINES, 2.0f);
	for (int i = 0; i < CIDEApp->CurrLevel.OffmeshConnections.Size(); ++i)
	{
		COffmeshConnection& Conn = CIDEApp->CurrLevel.OffmeshConnections[i];

		DD.vertex(Conn.From.x, Conn.From.y, Conn.From.z, BaseColor);
		DD.vertex(Conn.From.x, Conn.From.y + 0.2f, Conn.From.z, BaseColor);
		DD.vertex(Conn.To.x, Conn.To.y, Conn.To.z, BaseColor);
		DD.vertex(Conn.To.x, Conn.To.y + 0.2f, Conn.To.z, BaseColor);
		
		// AgentRadius -> Conn.Radius
		duAppendCircle(&DD, Conn.From.x, Conn.From.y + 0.1f, Conn.From.z, AgentRadius, BaseColor);
		duAppendCircle(&DD, Conn.To.x, Conn.To.y + 0.1f, Conn.To.z, AgentRadius, BaseColor);

		//if (hilight)
			duAppendArc(&DD, Conn.From.x, Conn.From.y, Conn.From.z, Conn.To.x, Conn.To.y, Conn.To.z, 0.25f,
				(Conn.Bidirectional) ? 0.6f : 0.0f, 0.6f, ConnColor);
	}	
	DD.end();

	DD.depthMask(true);

	nGfxServer2::Instance()->EndShapes();
	*/
}
//---------------------------------------------------------------------

}