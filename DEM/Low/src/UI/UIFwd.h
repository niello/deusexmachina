#pragma once

// UI system declarations and helpers

namespace UI
{

enum EDrawMode
{
	DrawMode_Opaque			= 0x01,
	DrawMode_Transparent	= 0x02,
	DrawMode_All			= (DrawMode_Opaque | DrawMode_Transparent)
};

static const U32 DrawModeFlagWindowOpaque = 1U << 2;

}
