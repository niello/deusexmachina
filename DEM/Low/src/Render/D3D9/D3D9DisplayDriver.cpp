#include "D3D9DisplayDriver.h"

#include <Render/D3D9/D3D9DriverFactory.h>
#include "DEMD3D9.h"

namespace Render
{
__ImplementClassNoFactory(Render::CD3D9DisplayDriver, Render::CDisplayDriver);

CD3D9DisplayDriver::CD3D9DisplayDriver(CD3D9DriverFactory& DriverFactory)
	: _DriverFactory(&DriverFactory)
{
}
//---------------------------------------------------------------------

bool CD3D9DisplayDriver::Init(UPTR AdapterNumber, UPTR OutputNumber)
{
	n_assert2(OutputNumber == 0, "D3D9 supports only one output (0) per video adapter");
	return CDisplayDriver::Init(AdapterNumber, OutputNumber);
}
//---------------------------------------------------------------------

// We don't cache available display modes since they can change after display driver was created
UPTR CD3D9DisplayDriver::GetAvailableDisplayModes(EPixelFormat Format, std::vector<CDisplayMode>& OutModes) const
{
	D3DDISPLAYMODE D3DDisplayMode = { 0 };
	D3DFORMAT D3DFormat = CD3D9DriverFactory::PixelFormatToD3DFormat(Format);
	UINT ModeCount = _DriverFactory->GetDirect3D9()->GetAdapterModeCount(AdapterID, D3DFormat);
	UPTR Total = 0;
	for (UINT i = 0; i < ModeCount; ++i)
	{
		if (!SUCCEEDED(_DriverFactory->GetDirect3D9()->EnumAdapterModes(AdapterID, D3DFormat, i, &D3DDisplayMode))) continue;
		
		CDisplayMode Mode(
			D3DDisplayMode.Width,
			D3DDisplayMode.Height,
			CD3D9DriverFactory::D3DFormatToPixelFormat(D3DDisplayMode.Format));
		Mode.RefreshRate.Numerator = D3DDisplayMode.RefreshRate;
		Mode.RefreshRate.Denominator = 1;
		Mode.Stereo = false;

		if (std::find(OutModes.cbegin(), OutModes.cend(), Mode) == OutModes.cend())
		{
			OutModes.push_back(std::move(Mode));
			++Total;
		}
	}

	return Total;
}
//---------------------------------------------------------------------

bool CD3D9DisplayDriver::SupportsDisplayMode(const CDisplayMode& Mode) const
{
	D3DDISPLAYMODE D3DDisplayMode = { 0 }; 
	D3DFORMAT D3DFormat = CD3D9DriverFactory::PixelFormatToD3DFormat(Mode.PixelFormat);
	UINT ModeCount = _DriverFactory->GetDirect3D9()->GetAdapterModeCount(AdapterID, D3DFormat);
	for (UINT i = 0; i < ModeCount; ++i)
	{
		if (!SUCCEEDED(_DriverFactory->GetDirect3D9()->EnumAdapterModes(AdapterID, D3DFormat, i, &D3DDisplayMode))) continue;
		if (Mode.Width == D3DDisplayMode.Width &&
			Mode.Height == D3DDisplayMode.Height &&
			D3DFormat == D3DDisplayMode.Format && //???doesn't always match?
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
	if (!SUCCEEDED(_DriverFactory->GetDirect3D9()->GetAdapterDisplayMode(AdapterID, &D3DDisplayMode))) FAIL;
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
	//???store in CD3D9DisplayDriver on creation?
	HMONITOR hMonitor = _DriverFactory->GetDirect3D9()->GetAdapterMonitor(AdapterID);

	MONITORINFO Win32MonitorInfo;
	Win32MonitorInfo.cbSize = sizeof(Win32MonitorInfo);
	if (!::GetMonitorInfo(hMonitor, &Win32MonitorInfo)) FAIL;

	OutInfo.Left = static_cast<I16>(Win32MonitorInfo.rcMonitor.left);
	OutInfo.Top = static_cast<I16>(Win32MonitorInfo.rcMonitor.top);
	OutInfo.Width = static_cast<U16>(Win32MonitorInfo.rcMonitor.right - Win32MonitorInfo.rcMonitor.left);
	OutInfo.Height = static_cast<U16>(Win32MonitorInfo.rcMonitor.bottom - Win32MonitorInfo.rcMonitor.top);
	OutInfo.IsPrimary = (Win32MonitorInfo.dwFlags & MONITORINFOF_PRIMARY);
	//!!!device name can be obtained from adapter or MONITORINFOEX!

	OK;
}
//---------------------------------------------------------------------

}

// Old own FindClosestMatchingMode()
/*
		if (DisplayMode.PixelFormat == D3DFMT_UNKNOWN)
			DisplayMode.PixelFormat = D3DFMT_X8R8G8B8;

		CArray<CDisplayMode> Modes;
		Display.GetAvailableDisplayModes((UPTR)D3DAdapter, D3DPresentParams.BackBufferFormat, Modes);
		if (Modes.FindIndex(DisplayMode) == INVALID_INDEX)
		{
			// Find available mode the most close to the requested one
			float IdealAspect = Display.GetDisplayMode().GetAspectRatio();
			float IdealResolution = (float)Display.GetDisplayMode().Width * (float)Display.GetDisplayMode().Height;
			float MinMetric = FLT_MAX;
			int MinIdx = INVALID_INDEX;
			for (int i = 0; i < Modes.GetCount(); ++i)
			{
				const CDisplayMode& Mode = Modes[i];
				float AspectDiff = Mode.GetAspectRatio() - IdealAspect;
				float ResolutionDiff = (float)(Mode.Width * Mode.Height) - IdealResolution;
				float Metric = AspectDiff * AspectDiff + ResolutionDiff * ResolutionDiff;
				if (Metric < MinMetric)
				{
					MinMetric = Metric;
					MinIdx = i;
				}
			}
			n_assert(MinIdx != INVALID_INDEX);
			DisplayMode.Width = Modes[MinIdx].Width;
			DisplayMode.Height = Modes[MinIdx].Height;
		}
*/