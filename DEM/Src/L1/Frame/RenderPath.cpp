#include "RenderPath.h"

#include <Frame/RenderPhase.h>
#include <Frame/View.h>
#include <Frame/NodeAttrCamera.h>
#include <Render/GPUDriver.h>
#include <Render/ConstantBuffer.h> // For IsInEditMode() only
#include <Render/ShaderMetadata.h>

namespace Frame
{
__ImplementClassNoFactory(Frame::CRenderPath, Resources::CResourceObject);

CRenderPath::~CRenderPath()
{
	if (pGlobals) n_delete(pGlobals);
}
//---------------------------------------------------------------------

//!!!EFFECT CONSTS DUPLICATE!
bool WriteEffectConstValue(Render::CGPUDriver& GPU, Render::CConstBufferRecord* Buffers, U32& BufferCount, U32 MaxBufferCount, const Render::CEffectConstant* pConst, const void* pValue, UPTR Size)
{
	// Number of buffers is typically very small, so linear search is not performance-critical
	U32 BufferIdx = 0;
	for (; BufferIdx < BufferCount; ++BufferIdx)
		if (Buffers[BufferIdx].Handle == pConst->BufferHandle) break;

	if (BufferIdx == BufferCount)
	{
		if (BufferCount >= MaxBufferCount) FAIL;

		//!!!request temporary buffer from the pool instead!
		static Render::PConstantBuffer Buffer = GPU.CreateConstantBuffer(pConst->BufferHandle, Render::Access_CPU_Write | Render::Access_GPU_Read);

		++BufferCount;
		Buffers[BufferIdx].Handle = pConst->BufferHandle;
		Buffers[BufferIdx].Buffer = Buffer;
		Buffers[BufferIdx].ShaderTypes = (1 << pConst->ShaderType);
						
		if (!GPU.BeginShaderConstants(*Buffer)) FAIL;
	}
	else
	{
		if (!Buffers[BufferIdx].Buffer->IsInEditMode())
		//if (!Buffers[BufferIdx].ShaderTypes)
		{
			if (!GPU.BeginShaderConstants(*Buffers[BufferIdx].Buffer)) FAIL;
		}
		Buffers[BufferIdx].ShaderTypes |= (1 << pConst->ShaderType);
	}

	return GPU.SetShaderConstant(*Buffers[BufferIdx].Buffer, pConst->Handle, 0, pValue, Size);
}
//---------------------------------------------------------------------

//!!!EFFECT CONSTS DUPLICATE!
bool ApplyConstBuffers(Render::CGPUDriver& GPU, Render::CConstBufferRecord* Buffers, U32 BufferCount)
{
	for (U32 BufferIdx = 0; BufferIdx < BufferCount; ++BufferIdx)
	{
		const Render::CConstBufferRecord& CBRec = Buffers[BufferIdx];
		if (!CBRec.ShaderTypes) continue;
		
		Render::CConstantBuffer& Buffer = *CBRec.Buffer;

		//Buffer.IsInEditMode()
		if (CBRec.ShaderTypes) GPU.CommitShaderConstants(Buffer);
		
		for (UPTR j = 0; j < Render::ShaderType_COUNT; ++j)
			if (CBRec.ShaderTypes & (1 << j))
				GPU.BindConstantBuffer((Render::EShaderType)j, CBRec.Handle, &Buffer);

		//???!!!where to reset shader types?! here?
		Buffers[BufferIdx].ShaderTypes = 0;
	}

	OK;
}
//---------------------------------------------------------------------

bool CRenderPath::Render(CView& View)
{
	//!!!clear all phases' render targets and DS surfaces at the beginning
	//of the frame, as recommended, especially for SLI, also it helps rendering
	//as phases must not clear RTs and therefore know are they the first to
	//render into or RT/DS already contains this frame data
	//!!!DBG TMP!
	View.GPU->ClearRenderTarget(*View.RTs[0], vector4(0.1f, 0.7f, 0.1f, 1.f));
	if (View.DSBuffer.IsValidPtr())
		View.GPU->ClearDepthStencilBuffer(*View.DSBuffer, Render::Clear_Depth, 1.f, 0);

	// Global params
	//???move to a separate phase? user then may implement it using knowledge about its global shader params.
	//as RP is not overridable, it is not a good place to reference global param names
	//some Globals phase with an association Const -> Shader param name
	//then find const Render::CEffectConstant* for each used const by name
	
	U32 GlobalCBCount = (U32)View.GlobalCBs.GetCount();
	
	if (View.GetCamera())
	{
		const Render::CEffectConstant* pConstViewProj = GetGlobalConstant(CStrID("ViewProj"));
		if (pConstViewProj)
		{
			const matrix44& ViewProj = View.GetCamera()->GetViewProjMatrix();
			WriteEffectConstValue(*View.GPU, View.GlobalCBs.Begin(), GlobalCBCount, GlobalCBCount, pConstViewProj, ViewProj.m, sizeof(matrix44));
		}
	}

	ApplyConstBuffers(*View.GPU, View.GlobalCBs.Begin(), GlobalCBCount);

	for (UPTR i = 0; i < Phases.GetCount(); ++i)
	{
		if (!Phases[i]->Render(View))
		{
			//???clear tmp view data?
			FAIL;
		}
	}

	//???clear tmp view data?

	OK;
}
//---------------------------------------------------------------------

}