#include "GPUDriver.h"

#include <Render/VertexLayout.h>
#include <System/OSWindow.h>
#include <Data/Params.h>

namespace Render
{

void CGPUDriver::PrepareWindowAndBackBufferSize(Sys::COSWindow& Window, UINT& Width, UINT& Height)
{
	// Zero Width or Height means backbuffer matching window or display size.
	// But if at least one of these values specified, we should adjst window size.
	// A child window is an exception, we don't want renderer to resize it,
	// so we force a backbuffer size to a child window size.
	if (Width > 0 || Height > 0)
	{
		if (Window.IsChild())
		{
			Width = Window.GetWidth();
			Height = Window.GetHeight();
		}
		else
		{
			Data::CRect WindowRect = Window.GetRect();
			if (Width > 0) WindowRect.W = Width;
			else Width = WindowRect.W;
			if (Height > 0) WindowRect.H = Height;
			else Height = WindowRect.H;
			Window.SetRect(WindowRect);
		}
	}
}
//---------------------------------------------------------------------

//!!!D3D11 needs a shader signature!
PVertexLayout CGPUDriver::CreateVertexLayout(const CArray<CVertexComponent>& Components)
{
	if (!Components.GetCount()) return NULL;
	CStrID Signature = CVertexLayout::BuildSignature(Components);
	int Idx = VertexLayouts.FindIndex(Signature);
	if (Idx != INVALID_INDEX) return VertexLayouts.ValueAt(Idx);
	PVertexLayout Layout = NULL; //InternalCreateVertexLayout();
	if (!Layout.IsValid() || !Layout->Create(Components)) return NULL;
	VertexLayouts.Add(Signature, Layout);
	return Layout;
}
//---------------------------------------------------------------------

//!!!for DX9, can just skip generation of input signatures on the tool side!
PShader CGPUDriver::CreateShader(const Data::CParams& Desc)
{
	// Load all the different param metadata sets (as the same params are often valid for a number of techs)
	// Create slots for dynamic constant buffers, don't create actual buffers yet, as some of them may be used by invalid techs only
	// Create slots for input signatures, assign IDs of already registered ones
	// Read technique count

	// For each technique:
		// Read feature flags
		// Assign ref to the param metadata of this tech
		// Read pass count

		// For each pass:
			// Get all GPU shaders from resource manager (no separate mgrs, as files are different & unique)
			// If some shaders weren't loaded, mark all the tech as invalid, free all its resources and leave as a stub or even don't add
			// Get vertex shader input signature by index, if not registered, register and pass loaded VS blob for input layout creation, and fill slot
			//???load render state?

		// Get refs for used dynamic buffers, if one not created yet, create it actually and fill slot
		//???load default sampler states and other default values for material creation?

	return NULL;
}
//---------------------------------------------------------------------

}