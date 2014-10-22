#include "D3D9DisplayDriver.h"

#include <Render/D3D9/D3D9DriverFactory.h>
#include <Core/Factory.h>
#define WIN32_LEAN_AND_MEAN
#define D3D_DISABLE_9EX
#include <d3d9.h>

namespace Render
{
__ImplementClass(Render::CD3D9DisplayDriver, 'D9DD', Render::CDisplayDriver);

bool CD3D9DisplayDriver::Init(DWORD AdapterNumber, DWORD OutputNumber)
{
	n_assert2(Output == 0, "D3D9 supports only one output (0) per video adapter");
	return CDisplayDriver::Init(AdapterNumber, OutputNumber);
}
//---------------------------------------------------------------------

// We don't cache available display modes since they can change after display driver was created
void CD3D9DisplayDriver::GetAvailableDisplayModes(EPixelFormat Format, CArray<CDisplayMode>& OutModes) const
{
	D3DDISPLAYMODE D3DDisplayMode = { 0 };
	D3DFORMAT D3DFormat = CD3D9DriverFactory::PixelFormatToD3DFormat(Format);
	UINT ModeCount = D3D9DrvFactory->GetDirect3D9()->GetAdapterModeCount(Adapter, D3DFormat);
	for (UINT i = 0; i < ModeCount; ++i)
	{
		if (!SUCCEEDED(D3D9DrvFactory->GetDirect3D9()->EnumAdapterModes(Adapter, D3DFormat, i, &D3DDisplayMode))) continue;
		CDisplayMode Mode(D3DDisplayMode.Width, D3DDisplayMode.Height, CD3D9DriverFactory::D3DFormatToPixelFormat(D3DDisplayMode.Format));
		if (OutModes.FindIndex(Mode) == INVALID_INDEX)
			OutModes.Add(Mode);
	}
}
//---------------------------------------------------------------------

bool CD3D9DisplayDriver::SupportsDisplayMode(const CDisplayMode& Mode) const
{
	D3DDISPLAYMODE D3DDisplayMode = { 0 }; 
	D3DFORMAT D3DFormat = CD3D9DriverFactory::PixelFormatToD3DFormat(Mode.PixelFormat);
	UINT ModeCount = D3D9DrvFactory->GetDirect3D9()->GetAdapterModeCount(Adapter, D3DFormat);
	for (UINT i = 0; i < ModeCount; i++)
	{
		if (!SUCCEEDED(D3D9DrvFactory->GetDirect3D9()->EnumAdapterModes(Adapter, D3DFormat, i, &D3DDisplayMode))) continue;
		if (Mode.Width == D3DDisplayMode.Width &&
			Mode.Height == D3DDisplayMode.Height &&
			Mode.PixelFormat == CD3D9DriverFactory::D3DFormatToPixelFormat(D3DDisplayMode.Format) &&
			Mode.RefreshRate.Numerator == D3DDisplayMode.RefreshRate &&
			Mode.RefreshRate.Denominator == 1 &&
			!Mode.Stereo) OK;
	}
	FAIL;
}
//---------------------------------------------------------------------

bool CD3D9DisplayDriver::GetCurrentDisplayMode(CDisplayMode& OutMode) const
{
	D3DDISPLAYMODE D3DDisplayMode = { 0 }; 
	if (!SUCCEEDED(D3D9DrvFactory->GetDirect3D9()->GetAdapterDisplayMode(Adapter, &D3DDisplayMode))) FAIL;
	OutMode.Width = D3DDisplayMode.Width;
	OutMode.Height = D3DDisplayMode.Height;
	OutMode.PixelFormat = CD3D9DriverFactory::D3DFormatToPixelFormat(D3DDisplayMode.Format);
	OutMode.RefreshRate.Numerator = D3DDisplayMode.RefreshRate;
	OutMode.RefreshRate.Denominator = 1;
	OutMode.Stereo = false;
	OK;
}
//---------------------------------------------------------------------

bool CD3D9DisplayDriver::GetDisplayMonitorInfo(CMonitorInfo& OutInfo) const
{
	HMONITOR hMonitor = D3D9DrvFactory->GetDirect3D9()->GetAdapterMonitor(Adapter);
	MONITORINFO Win32MonitorInfo = { sizeof(Win32MonitorInfo), 0 };
	if (!::GetMonitorInfo(hMonitor, &Win32MonitorInfo)) FAIL;
	OutInfo.Left = (ushort)Win32MonitorInfo.rcMonitor.left;
	OutInfo.Top = (ushort)Win32MonitorInfo.rcMonitor.top;
	OutInfo.Width = (ushort)(Win32MonitorInfo.rcMonitor.right - Win32MonitorInfo.rcMonitor.left);
	OutInfo.Height = (ushort)(Win32MonitorInfo.rcMonitor.bottom - Win32MonitorInfo.rcMonitor.top);
	OutInfo.IsPrimary = Win32MonitorInfo.dwFlags & MONITORINFOF_PRIMARY;
	//!!!device name can be obtained from adapter or MONITORINFOEX!
	OK;
}
//---------------------------------------------------------------------

}