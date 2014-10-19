#include "D3D9DisplayDriver.h"

#include <Core/Factory.h>
#define WIN32_LEAN_AND_MEAN
#define D3D_DISABLE_9EX
#include <d3d9.h>

namespace Render
{
__ImplementClass(Render::CD3D9DisplayDriver, 'D9DD', Render::CDisplayDriver);

void CD3D9DisplayDriver::GetAvailableDisplayModes(EPixelFormat Format, CArray<CDisplayMode>& OutModes) const
{
	n_assert(AdapterExists(Adapter));
	D3DDISPLAYMODE D3DDisplayMode = { 0 };
	D3DFORMAT D3DFormat = PixelFormatToD3DFormat(Format);
	UINT ModeCount = pD3D9->GetAdapterModeCount(Adapter, D3DFormat);
	for (UINT i = 0; i < ModeCount; i++)
	{
		if (!SUCCEEDED(pD3D9->EnumAdapterModes(Adapter, D3DFormat, i, &D3DDisplayMode))) continue;
		CDisplayMode Mode(D3DDisplayMode.Width, D3DDisplayMode.Height, D3DFormatToPixelFormat(D3DDisplayMode.Format));
		if (OutModes.FindIndex(Mode) == INVALID_INDEX)
			OutModes.Add(Mode);
	}
}
//---------------------------------------------------------------------

bool CD3D9DisplayDriver::SupportsDisplayMode(const CDisplayMode& Mode) const
{
	D3DDISPLAYMODE D3DDisplayMode = { 0 }; 
	D3DFORMAT D3DFormat = PixelFormatToD3DFormat(Mode.PixelFormat);
	UINT ModeCount = pD3D9->GetAdapterModeCount(Adapter, D3DFormat);
	for (UINT i = 0; i < ModeCount; i++)
	{
		if (!SUCCEEDED(pD3D9->EnumAdapterModes(Adapter, D3DFormat, i, &D3DDisplayMode))) continue;
		if (Mode.Width == D3DDisplayMode.Width &&
			Mode.Height == D3DDisplayMode.Height &&
			Mode.PixelFormat == D3DFormatToPixelFormat(D3DDisplayMode.Format) &&
			Mode.RefreshRate.Numerator == D3DDisplayMode.RefreshRate &&
			Mode.RefreshRate.Denominator == 1 &&
			!Mode.Stereo) OK;
	}
	FAIL;
}
//---------------------------------------------------------------------

bool CD3D9DisplayDriver::GetCurrentDisplayMode(CDisplayMode& OutMode) const
{
	n_assert(AdapterExists(Adapter));
	D3DDISPLAYMODE D3DDisplayMode = { 0 }; 
	HRESULT hr = pD3D9->GetAdapterDisplayMode(Adapter, &D3DDisplayMode);
	if (hr == D3DERR_DEVICELOST) FAIL;
	n_assert(SUCCEEDED(hr));
	OutMode.Width = D3DDisplayMode.Width;
	OutMode.Height = D3DDisplayMode.Height;
	OutMode.PixelFormat = D3DFormatToPixelFormat(D3DDisplayMode.Format);
	OutMode.RefreshRate.Numerator = D3DDisplayMode.RefreshRate;
	OutMode.RefreshRate.Denominator = 1;
	OutMode.Stereo = false;
	OK;
}
//---------------------------------------------------------------------

void CD3D9DisplayDriver::GetMonitorInfo(CMonitorInfo& OutInfo) const
{
	n_assert(AdapterExists(Adapter));
	HMONITOR hMonitor = pD3D9->GetAdapterMonitor(Adapter);
	MONITORINFO Win32MonitorInfo = { sizeof(Win32MonitorInfo), 0 };
	GetMonitorInfo(hMonitor, &Win32MonitorInfo);
	OutInfo.Left = (ushort)Win32MonitorInfo.rcMonitor.left;
	OutInfo.Top = (ushort)Win32MonitorInfo.rcMonitor.top;
	OutInfo.Width = (ushort)(Win32MonitorInfo.rcMonitor.right - Win32MonitorInfo.rcMonitor.left);
	OutInfo.Height = (ushort)(Win32MonitorInfo.rcMonitor.bottom - Win32MonitorInfo.rcMonitor.top);
	OutInfo.IsPrimary = Win32MonitorInfo.dwFlags & MONITORINFOF_PRIMARY;
}
//---------------------------------------------------------------------

D3DFORMAT CD3D9DisplayDriver::PixelFormatToD3DFormat(EPixelFormat Format)
{
	switch (Format)
	{
		case PixelFmt_X8R8G8B8:	return D3DFMT_X8R8G8B8;
		case PixelFmt_Invalid:
		default:				return D3DFMT_UNKNOWN;
	}
}
//---------------------------------------------------------------------

EPixelFormat CD3D9DisplayDriver::D3DFormatToPixelFormat(D3DFORMAT D3DFormat)
{
	switch (D3DFormat)
	{
		case D3DFMT_X8R8G8B8:	return PixelFmt_X8R8G8B8;
		case D3DFMT_UNKNOWN:
		default:				return PixelFmt_Invalid;
	}
}
//---------------------------------------------------------------------

}