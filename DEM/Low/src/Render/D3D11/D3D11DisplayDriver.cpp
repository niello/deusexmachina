#include "D3D11DisplayDriver.h"

#include <Render/D3D11/D3D11DriverFactory.h>
#define WIN32_LEAN_AND_MEAN
#include <DXGI.h>

/* Adapter from output:
IDXGIAdapter1* pAdapter = nullptr;
pDXGIOutput->GetParent(__uuidof(IDXGIAdapter1), (void**)&pAdapter);
*/

namespace Render
{

CD3D11DisplayDriver::CD3D11DisplayDriver(CD3D11DriverFactory& DriverFactory)
	: _DriverFactory(&DriverFactory)
{
}
//---------------------------------------------------------------------

bool CD3D11DisplayDriver::Init(UPTR AdapterNumber, UPTR OutputNumber)
{
	if (!CDisplayDriver::Init(AdapterNumber, OutputNumber)) FAIL;

	IDXGIAdapter1* pAdapter = nullptr;
	if (!SUCCEEDED(_DriverFactory->GetDXGIFactory()->EnumAdapters1(AdapterID, &pAdapter)))
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
	std::unique_ptr<DXGI_MODE_DESC[]> DXGIModes;
	do
	{
		// Starting with Direct3D 11.1, we recommend not to use GetDisplayModeList anymore to retrieve the matching 
		// display mode. Instead, use IDXGIOutput1::GetDisplayModeList1, which supports stereo display mode. (c) Docs
		if (!SUCCEEDED(pDXGIOutput->GetDisplayModeList(DXGIFormat, 0, &ModeCount, nullptr)) || !ModeCount) return 0;
		DXGIModes = std::make_unique<DXGI_MODE_DESC[]>(ModeCount);
		hr = pDXGIOutput->GetDisplayModeList(DXGIFormat, 0, &ModeCount, DXGIModes.get());
	}
	while (hr == DXGI_ERROR_MORE_DATA); // Somtimes new modes become available right between two calls, see DXGI docs

	UPTR Total = 0;
	OutModes.reserve(ModeCount);
	for (UINT i = 0; i < ModeCount; ++i)
	{
		const DXGI_MODE_DESC& DXGIMode = DXGIModes[i];

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

	return Total;
}
//---------------------------------------------------------------------

bool CD3D11DisplayDriver::SupportsDisplayMode(const CDisplayMode& Mode) const
{
	if (!pDXGIOutput) FAIL;

	DXGI_FORMAT DXGIFormat = CD3D11DriverFactory::PixelFormatToDXGIFormat(Mode.PixelFormat);

	HRESULT hr;
	UINT ModeCount = 0;
	std::unique_ptr<DXGI_MODE_DESC[]> DXGIModes;
	do
	{
		// Starting with Direct3D 11.1, we recommend not to use GetDisplayModeList anymore to retrieve the matching 
		// display mode. Instead, use IDXGIOutput1::GetDisplayModeList1, which supports stereo display mode. (c) Docs
		if (!SUCCEEDED(pDXGIOutput->GetDisplayModeList(DXGIFormat, 0, &ModeCount, nullptr)) || !ModeCount) FAIL;
		DXGIModes = std::make_unique<DXGI_MODE_DESC[]>(ModeCount);
		hr = pDXGIOutput->GetDisplayModeList(DXGIFormat, 0, &ModeCount, DXGIModes.get());
	}
	while (hr == DXGI_ERROR_MORE_DATA); // Sometimes new modes become available right between two calls, see DXGI docs

	for (UINT i = 0; i < ModeCount; ++i)
	{
		DXGI_MODE_DESC& DXGIMode = DXGIModes[i];
		if (Mode.Width == DXGIMode.Width &&
			Mode.Height == DXGIMode.Height &&
			DXGIFormat == DXGIMode.Format && //???doesn't always match?
			Mode.RefreshRate.Numerator == DXGIMode.RefreshRate.Numerator &&
			Mode.RefreshRate.Denominator == DXGIMode.RefreshRate.Denominator &&
			!Mode.Stereo)
		{
			OK;
		}
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CD3D11DisplayDriver::GetCurrentDisplayMode(CDisplayMode& OutMode) const
{
	if (!pDXGIOutput) FAIL;

	DXGI_OUTPUT_DESC Desc;
	if (!SUCCEEDED(pDXGIOutput->GetDesc(&Desc))) FAIL;

	DXGI_MODE_DESC ApproxMode, DXGIMode;

	// Get current mode
	ApproxMode.Width = Desc.DesktopCoordinates.right - Desc.DesktopCoordinates.left;
	ApproxMode.Height = Desc.DesktopCoordinates.bottom - Desc.DesktopCoordinates.top;
	ApproxMode.RefreshRate.Numerator = 0;
	ApproxMode.RefreshRate.Denominator = 0;
	ApproxMode.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	ApproxMode.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;

	// If pConcernedDevice is nullptr, Format cannot be DXGI_FORMAT_UNKNOWN. (c) Docs
	// TODO: GPU to args, optional
	//if (pGPUDriver)
	//	ApproxMode.Format = DXGI_FORMAT_UNKNOWN;
	ApproxMode.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	if (!SUCCEEDED(pDXGIOutput->FindClosestMatchingMode(&ApproxMode, &DXGIMode, nullptr))) FAIL;

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

	MONITORINFO Win32MonitorInfo;
	Win32MonitorInfo.cbSize = sizeof(Win32MonitorInfo);
	if (!::GetMonitorInfo(Desc.Monitor, &Win32MonitorInfo)) FAIL;

	OutInfo.Left = static_cast<I16>(Win32MonitorInfo.rcMonitor.left);
	OutInfo.Top = static_cast<I16>(Win32MonitorInfo.rcMonitor.top);
	OutInfo.Width = static_cast<U16>(Win32MonitorInfo.rcMonitor.right - Win32MonitorInfo.rcMonitor.left);
	OutInfo.Height = static_cast<U16>(Win32MonitorInfo.rcMonitor.bottom - Win32MonitorInfo.rcMonitor.top);
	OutInfo.IsPrimary = (Win32MonitorInfo.dwFlags & MONITORINFOF_PRIMARY);

	/* MONITORINFO -> MONITORINFOEX
	HDC hDC = ::CreateDC(TEXT("DISPLAY"), Win32MonitorInfo.szDevice, nullptr, nullptr);
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
