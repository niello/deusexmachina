#include "D3D11DisplayDriver.h"

#include <Render/D3D11/D3D11DriverFactory.h>
#include <Core/Factory.h>
#define WIN32_LEAN_AND_MEAN
#include <DXGI.h>

namespace Render
{
__ImplementClass(Render::CD3D11DisplayDriver, 'D1DD', Render::CDisplayDriver);

bool CD3D11DisplayDriver::Init(UPTR AdapterNumber, UPTR OutputNumber)
{
	if (!CDisplayDriver::Init(AdapterNumber, OutputNumber)) FAIL;

	IDXGIAdapter1* pAdapter = NULL;
	if (!SUCCEEDED(D3D11DrvFactory->GetDXGIFactory()->EnumAdapters1(AdapterID, &pAdapter)))
	{
		Term();
		FAIL;
	}

	if (!SUCCEEDED(pAdapter->EnumOutputs(OutputID, &pDXGIOutput)))
	{
		pAdapter->Release();
		Term();
		FAIL;
	}

	pAdapter->Release();
	OK;
}
//---------------------------------------------------------------------

void CD3D11DisplayDriver::InternalTerm()
{
	SAFE_RELEASE(pDXGIOutput);
}
//---------------------------------------------------------------------

UPTR CD3D11DisplayDriver::GetAvailableDisplayModes(EPixelFormat Format, std::vector<CDisplayMode>& OutModes) const
{
	if (!pDXGIOutput) return 0;

	DXGI_FORMAT DXGIFormat = CD3D11DriverFactory::PixelFormatToDXGIFormat(Format);

	HRESULT hr;
	UINT ModeCount = 0;
	DXGI_MODE_DESC* pDXGIModes = nullptr;
	do
	{
		if (pDXGIModes) _freea(pDXGIModes);
		if (!SUCCEEDED(pDXGIOutput->GetDisplayModeList(DXGIFormat, 0, &ModeCount, NULL)) || !ModeCount) return 0;
		pDXGIModes = (DXGI_MODE_DESC*)_malloca(sizeof(DXGI_MODE_DESC) * ModeCount);
		hr = pDXGIOutput->GetDisplayModeList(DXGIFormat, 0, &ModeCount, pDXGIModes);
	}
	while (hr == DXGI_ERROR_MORE_DATA); // Somtimes new modes become available right between two calls, see DXGI docs

	UPTR Total = 0;
	OutModes.reserve(ModeCount);
	for (UINT i = 0; i < ModeCount; ++i)
	{
		const DXGI_MODE_DESC& DXGIMode = pDXGIModes[i];

		CDisplayMode NewMode;
		NewMode.Width = DXGIMode.Width;
		NewMode.Height = DXGIMode.Height;
		NewMode.PixelFormat = CD3D11DriverFactory::DXGIFormatToPixelFormat(DXGIMode.Format);
		NewMode.RefreshRate.Numerator = DXGIMode.RefreshRate.Numerator;
		NewMode.RefreshRate.Denominator = DXGIMode.RefreshRate.Denominator;
		NewMode.Stereo = false; // Useful with DXGI 1.2 and above only

		if (std::find(OutModes.cbegin(), OutModes.cend(), NewMode) == OutModes.cend())
		{
			OutModes.push_back(std::move(NewMode));
			++Total;
		}
	}

	_freea(pDXGIModes);

	return Total;
}
//---------------------------------------------------------------------

bool CD3D11DisplayDriver::SupportsDisplayMode(const CDisplayMode& Mode) const
{
	if (!pDXGIOutput) FAIL;

	DXGI_FORMAT DXGIFormat = CD3D11DriverFactory::PixelFormatToDXGIFormat(Mode.PixelFormat);

	HRESULT hr;
	UINT ModeCount = 0;
	DXGI_MODE_DESC* pDXGIModes = NULL;
	do
	{
		if (pDXGIModes) _freea(pDXGIModes);
		if (!SUCCEEDED(pDXGIOutput->GetDisplayModeList(DXGIFormat, 0, &ModeCount, NULL)) || !ModeCount) FAIL;
		pDXGIModes = (DXGI_MODE_DESC*)_malloca(sizeof(DXGI_MODE_DESC) * ModeCount);
		hr = pDXGIOutput->GetDisplayModeList(DXGIFormat, 0, &ModeCount, pDXGIModes);
	}
	while (hr == DXGI_ERROR_MORE_DATA); // Sometimes new modes become available right between two calls, see DXGI docs

	for (UINT i = 0; i < ModeCount; ++i)
	{
		DXGI_MODE_DESC& DXGIMode = pDXGIModes[i];
		if (Mode.Width == DXGIMode.Width &&
			Mode.Height == DXGIMode.Height &&
			DXGIFormat == DXGIMode.Format && //???doesn't always match?
			Mode.RefreshRate.Numerator == DXGIMode.RefreshRate.Numerator &&
			Mode.RefreshRate.Denominator == DXGIMode.RefreshRate.Denominator &&
			!Mode.Stereo)
		{
			_freea(pDXGIModes);
			OK;
		}
	}

	_freea(pDXGIModes);
	FAIL;
}
//---------------------------------------------------------------------

bool CD3D11DisplayDriver::GetCurrentDisplayMode(CDisplayMode& OutMode) const
{
	if (!pDXGIOutput) FAIL;

	//???or adapter device name?

	/*
	DXGI_OUTPUT_DESC Desc;
	if (!SUCCEEDED(pDXGIOutput->GetDesc(&Desc))) FAIL;

	MONITORINFOEX Win32MonitorInfo;
	::ZeroMemory(&Win32MonitorInfo, sizeof(Win32MonitorInfo));
	Win32MonitorInfo.cbSize = sizeof(Win32MonitorInfo);
	if (!::GetMonitorInfo(Desc.Monitor, &Win32MonitorInfo)) FAIL;

	// Can't believe, but there is no way to obtain current display mode via DXGI without a swap chain
	DEVMODE DevMode = { 0 };
	DevMode.dmSize = sizeof(DevMode);
	if (!::EnumDisplaySettings(Win32MonitorInfo.szDevice, ENUM_CURRENT_SETTINGS, &DevMode)) FAIL;

	//???enumerate all matching modes and select one?
	//!!!OutMode.PixelFormat = 
	OutMode.Stereo = false; //???how to be when it could be stereo?

	DXGI_MODE_DESC ApproxMode, DXGIMode;

	ApproxMode.Width = DevMode.dmPelsWidth;
	ApproxMode.Height = DevMode.dmPelsHeight;
	ApproxMode.RefreshRate.Numerator = DevMode.dmDisplayFrequency;
	ApproxMode.RefreshRate.Denominator = 1;
	ApproxMode.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	ApproxMode.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;

	//???use SRGB formats?
	switch (DevMode.dmBitsPerPel)
	{
		case 16: ApproxMode.Format = DXGI_FORMAT_B5G6R5_UNORM; break;
		case 24: ApproxMode.Format = DXGI_FORMAT_B8G8R8X8_UNORM; break;
		case 32: ApproxMode.Format = DXGI_FORMAT_B8G8R8A8_UNORM; break;
		default: FAIL;
	}
	*/

	DXGI_MODE_DESC ApproxMode, DXGIMode;

	// Get current mode
	ApproxMode.Width = 0;
	ApproxMode.Height = 0;
	ApproxMode.RefreshRate.Numerator = 0;
	ApproxMode.RefreshRate.Denominator = 0;
	ApproxMode.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	ApproxMode.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;

	//???If pConcernedDevice is NULL, Format cannot be DXGI_FORMAT_UNKNOWN. (c) Docs?

	if (!SUCCEEDED(pDXGIOutput->FindClosestMatchingMode(&ApproxMode, &DXGIMode, NULL))) FAIL;

	OutMode.Width = DXGIMode.Width;
	OutMode.Height = DXGIMode.Height;
	OutMode.PixelFormat = CD3D11DriverFactory::DXGIFormatToPixelFormat(DXGIMode.Format);
	OutMode.RefreshRate.Numerator = DXGIMode.RefreshRate.Numerator;
	OutMode.RefreshRate.Denominator = DXGIMode.RefreshRate.Denominator;
	OutMode.Stereo = false; // Useful with DXGI 1.2 and above only

	OK;
}
//---------------------------------------------------------------------

bool CD3D11DisplayDriver::GetDisplayMonitorInfo(CMonitorInfo& OutInfo) const
{
	if (!pDXGIOutput) FAIL;

	DXGI_OUTPUT_DESC Desc;
	if (!SUCCEEDED(pDXGIOutput->GetDesc(&Desc))) FAIL;

	MONITORINFOEX Win32MonitorInfo;
	Win32MonitorInfo.cbSize = sizeof(Win32MonitorInfo);
	if (!::GetMonitorInfo(Desc.Monitor, &Win32MonitorInfo)) FAIL;

	OutInfo.Left = (U16)Win32MonitorInfo.rcMonitor.left;
	OutInfo.Top = (U16)Win32MonitorInfo.rcMonitor.top;
	OutInfo.Width = (U16)(Win32MonitorInfo.rcMonitor.right - Win32MonitorInfo.rcMonitor.left);
	OutInfo.Height = (U16)(Win32MonitorInfo.rcMonitor.bottom - Win32MonitorInfo.rcMonitor.top);
	OutInfo.IsPrimary = Win32MonitorInfo.dwFlags & MONITORINFOF_PRIMARY;

	/*
	HDC hDC = ::CreateDC(TEXT("DISPLAY"), Win32MonitorInfo.szDevice, NULL, NULL);
	if (hDC)
	{
		OutInfo.NativeWidth = ::GetDeviceCaps(hDC, HORZRES);
		OutInfo.NativeHeight = ::GetDeviceCaps(hDC, VERTRES);
		::DeleteDC(hDC);
	}
	*/

	OK;
}
//---------------------------------------------------------------------

}
