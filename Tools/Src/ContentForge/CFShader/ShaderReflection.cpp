#include "ShaderReflection.h"

#include <IO/BinaryReader.h>
#include <IO/BinaryWriter.h>
#include <ConsoleApp.h>				// For n_msg
#include <D3D9ShaderReflection.h>
#include <D3DCompiler.inl>

bool D3D9CollectShaderMetadata(const void* pData, UPTR Size, const char* pSource, UPTR SourceSize, CD3D9ShaderMeta& Out)
{
	CArray<CD3D9ConstantDesc> D3D9Consts;
	CString Creator;

	if (!D3D9Reflect(pData, Size, D3D9Consts, Creator)) FAIL;

	CDict<CString, CString> SampToTex;
	D3D9FindSamplerTextures(pSource, SourceSize, SampToTex);

	CD3D9ShaderBufferMeta* pMeta = Out.Buffers.Reserve(1);
	pMeta->Name = "$Global";

	for (UPTR i = 0; i < D3D9Consts.GetCount(); ++i)
	{
		CD3D9ConstantDesc& D3D9ConstDesc = D3D9Consts[i];

		if (D3D9ConstDesc.RegisterSet == RS_SAMPLER)
		{
			CD3D9ShaderRsrcMeta* pMeta = Out.Samplers.Reserve(1);
			pMeta->SamplerName = D3D9ConstDesc.Name;
			pMeta->Register = D3D9ConstDesc.RegisterIndex; //???support sampler arrays?

			int STIdx = SampToTex.FindIndex(D3D9ConstDesc.Name);
			if (STIdx != INVALID_INDEX) pMeta->TextureName = SampToTex.ValueAt(STIdx);
		}
		else
		{
			CD3D9ShaderConstMeta* pMeta = Out.Consts.Reserve(1);
			pMeta->Name = D3D9ConstDesc.Name;
			pMeta->RegSet = D3D9ConstDesc.RegisterSet;
			pMeta->Offset = D3D9ConstDesc.RegisterIndex;
			pMeta->Size = D3D9ConstDesc.RegisterCount;

			// Try to get owning pseudo-buffer from extra info, else use default

			pMeta->BufferIndex = 0; // Default, global buffer

			CD3D9ShaderBufferMeta& BufMeta = Out.Buffers[pMeta->BufferIndex];
			CArray<UPTR>& UsedRegs = (pMeta->RegSet == RS_FLOAT4) ? BufMeta.UsedFloat4 : ((pMeta->RegSet == RS_INT4) ? BufMeta.UsedInt4 : BufMeta.UsedBool);
			for (UPTR r = D3D9ConstDesc.RegisterIndex; r < D3D9ConstDesc.RegisterIndex + D3D9ConstDesc.RegisterCount; ++r)
			{
				if (!UsedRegs.Contains(r)) UsedRegs.Add(r);
			}
		}
	}

	UPTR i = 0;
	while (i < Out.Buffers.GetCount())
	{
		CD3D9ShaderBufferMeta& B = Out.Buffers[i];
		if (!B.UsedFloat4.GetCount() &&
			!B.UsedInt4.GetCount() &&
			!B.UsedBool.GetCount())
		{
			Out.Buffers.RemoveAt(i);
		}
		else ++i;
	};

	OK;
}
//---------------------------------------------------------------------

bool D3D11CollectShaderMetadata(const void* pData, UPTR Size, CD3D11ShaderMeta& Out)
{
	ID3D11ShaderReflection* pReflector = NULL;
	HRESULT hr = D3D11Reflect(pData, Size, &pReflector);

	if (FAILED(hr)) FAIL;

	//!!!may save!
	//D3D_FEATURE_LEVEL FeatureLevel;
	//pReflector->GetMinFeatureLevel(&FeatureLevel);

	//pReflector->GetRequiresFlags();

	D3D11_SHADER_DESC D3DDesc;
	pReflector->GetDesc(&D3DDesc);

	for (UINT RsrcIdx = 0; RsrcIdx < D3DDesc.BoundResources; ++RsrcIdx)
	{
		D3D11_SHADER_INPUT_BIND_DESC RsrcDesc;
		pReflector->GetResourceBindingDesc(RsrcIdx, &RsrcDesc);

		// D3D_SIF_USERPACKED - may fail assertion if not set!

		switch (RsrcDesc.Type)
		{
			case D3D_SIT_TEXTURE:
			{
				n_assert(RsrcDesc.BindCount == 1);
				CD3D11ShaderRsrcMeta* pMeta = Out.Resources.Reserve(1);
				pMeta->Name = RsrcDesc.Name;
				pMeta->Register = RsrcDesc.BindPoint; //???add array support? BindCount
				break;
			}
			case D3D_SIT_SAMPLER:
			{
				// D3D_SIF_COMPARISON_SAMPLER
				n_assert(RsrcDesc.BindCount == 1);
				CD3D11ShaderRsrcMeta* pMeta = Out.Samplers.Reserve(1);
				pMeta->Name = RsrcDesc.Name;
				pMeta->Register = RsrcDesc.BindPoint; //???add array support? BindCount
				break;
			}
			case D3D_SIT_CBUFFER:
			case D3D_SIT_TBUFFER: //!!!resource, not cb! Var address = resource register (up to 128!) and Offset (big)
			case D3D_SIT_STRUCTURED: //!!!resource, not cb! Var address = resource register (up to 128!) and Offset (big)
			{
				n_assert(RsrcDesc.BindCount == 1); //???support arrays?

				ID3D11ShaderReflectionConstantBuffer* pCB = pReflector->GetConstantBufferByName(RsrcDesc.Name);
				if (!pCB) continue;

				D3D11_SHADER_BUFFER_DESC D3DBufDesc;
				pCB->GetDesc(&D3DBufDesc);
				if (!D3DBufDesc.Variables) continue;

				DWORD TypeMask;
				if (RsrcDesc.Type == D3D_SIT_TBUFFER) TypeMask = D3D11Buffer_Texture;
				else if (RsrcDesc.Type == D3D_SIT_STRUCTURED) TypeMask = D3D11Buffer_Structured;
				else TypeMask = 0;

				n_assert_dbg(!TypeMask); //!!!add tbuffer & sbuffer support!

				CD3D11ShaderBufferMeta* pMeta = Out.Buffers.Reserve(1);
				pMeta->Name = RsrcDesc.Name;
				pMeta->Register = (RsrcDesc.BindPoint | TypeMask);
				pMeta->ElementSize = D3DBufDesc.Size;
				pMeta->ElementCount = 1;

				for (UINT VarIdx = 0; VarIdx < D3DBufDesc.Variables; ++VarIdx)
				{
					ID3D11ShaderReflectionVariable* pVar = pCB->GetVariableByIndex(VarIdx);
					if (!pVar) continue;

					D3D11_SHADER_VARIABLE_DESC D3DVarDesc;
					pVar->GetDesc(&D3DVarDesc);

					//D3D_SVF_USERPACKED             = 1,
					//D3D_SVF_USED                   = 2,

					ID3D11ShaderReflectionType* pVarType = pVar->GetType();
					if (!pVarType) continue;

					D3D11_SHADER_TYPE_DESC D3DTypeDesc;
					pVarType->GetDesc(&D3DTypeDesc);

					CD3D11ShaderConstMeta* pMeta = Out.Consts.Reserve(1);
					pMeta->Name = D3DVarDesc.Name;
					pMeta->BufferIndex = Out.Buffers.GetCount() - 1;
					pMeta->Offset = D3DVarDesc.StartOffset;
					pMeta->Size = D3DVarDesc.Size;

					//D3D11_SHADER_TYPE_DESC
				}

				break;
			}
		}
	}

	pReflector->Release();

	OK;
}
//---------------------------------------------------------------------

void WriteRegisterRanges(const CArray<UPTR>& UsedRegs, IO::CBinaryWriter& W, const char* pRegisterSetName)
{
	U64 RangeCountOffset = W.GetStream().GetPosition();
	W.Write<U32>(0);

	if (!UsedRegs.GetCount()) return;

	U32 RangeCount = 0;
	UPTR CurrStart = UsedRegs[0], CurrCount = 1;
	for (UPTR r = 1; r < UsedRegs.GetCount(); ++r)
	{
		UPTR Reg = UsedRegs[r];
		if (Reg == CurrStart + CurrCount) ++CurrCount;
		else
		{
			// New range detected
			W.Write<U32>(CurrStart);
			W.Write<U32>(CurrCount);
			++RangeCount;
			n_msg(VL_DETAILS, "    Range: %s %d to %d\n", pRegisterSetName, CurrStart, CurrStart + CurrCount - 1);
			CurrStart = Reg;
			CurrCount = 1;
		}
	}

	if (CurrStart != (UPTR)-1)
	{
		W.Write<U32>(CurrStart);
		W.Write<U32>(CurrCount);
		++RangeCount;
		n_msg(VL_DETAILS, "    Range: %s %d to %d\n", pRegisterSetName, CurrStart, CurrStart + CurrCount - 1);
	}

	U64 EndOffset = W.GetStream().GetPosition();
	W.GetStream().Seek(RangeCountOffset, IO::Seek_Begin);
	W.Write<U32>(RangeCount);
	W.GetStream().Seek(EndOffset, IO::Seek_Begin);
}
//---------------------------------------------------------------------

bool D3D9SaveShaderMetadata(IO::CBinaryWriter& W, const CD3D9ShaderMeta& Meta)
{
	W.Write<U32>(Meta.Buffers.GetCount());
	for (UPTR i = 0; i < Meta.Buffers.GetCount(); ++i)
	{
		CD3D9ShaderBufferMeta& Obj = Meta.Buffers[i];
		Obj.UsedFloat4.Sort();
		Obj.UsedInt4.Sort();
		Obj.UsedBool.Sort();

		W.Write(Obj.Name);

		WriteRegisterRanges(Obj.UsedFloat4, W, "float4");
		WriteRegisterRanges(Obj.UsedInt4, W, "int4");
		WriteRegisterRanges(Obj.UsedBool, W, "bool");

		n_msg(VL_DETAILS, "    CBuffer: %s\n", Obj.Name.CStr());
	}

	W.Write<U32>(Meta.Consts.GetCount());
	for (UPTR i = 0; i < Meta.Consts.GetCount(); ++i)
	{
		const CD3D9ShaderConstMeta& Obj = Meta.Consts[i];

		U8 RegSet;
		switch (Obj.RegSet)
		{
			case RS_FLOAT4:	RegSet = 0; break;
			case RS_INT4:	RegSet = 1; break;
			case RS_BOOL:	RegSet = 2; break;
			default:		continue;
		};

		W.Write(Obj.Name);
		W.Write(Obj.BufferIndex);
		W.Write<U8>(RegSet);
		W.Write(Obj.Offset);
		W.Write(Obj.Size);

		const char* pRegisterSetName = NULL;
		switch (Obj.RegSet)
		{
			case RS_FLOAT4:	pRegisterSetName = "float4"; break;
			case RS_INT4:	pRegisterSetName = "int4"; break;
			case RS_BOOL:	pRegisterSetName = "bool"; break;
		};
				
		n_msg(VL_DETAILS, "    Const: %s, %s %d to %d\n",
				Obj.Name.CStr(), pRegisterSetName, Obj.Offset, Obj.Offset + Obj.Size - 1);
	}

	W.Write<U32>(Meta.Samplers.GetCount());
	for (UPTR i = 0; i < Meta.Samplers.GetCount(); ++i)
	{
		const CD3D9ShaderRsrcMeta& Obj = Meta.Samplers[i];
		W.Write(Obj.SamplerName);
		W.Write(Obj.TextureName);
		W.Write(Obj.Register);		//!!!need sampler arrays! need one texture to multiple samplers!

		n_msg(VL_DETAILS, "    Sampler: %s (texture %s), %d slot(s) from %d\n", Obj.SamplerName.CStr(), Obj.TextureName.CStr(), 1, Obj.Register);
	}

	OK;
}
//---------------------------------------------------------------------

bool D3D11SaveShaderMetadata(IO::CBinaryWriter& W, const CD3D11ShaderMeta& Meta)
{
	W.Write<U32>(Meta.Buffers.GetCount());
	for (UPTR i = 0; i < Meta.Buffers.GetCount(); ++i)
	{
		//!!!arrays, tbuffers & sbuffers aren't supported for now!
		const CD3D11ShaderBufferMeta& Obj = Meta.Buffers[i];
		W.Write(Obj.Name);
		W.Write(Obj.Register);
		W.Write(Obj.ElementSize);
		W.Write(Obj.ElementCount);
		const char* pBufferTypeStr = NULL;
		if (Obj.Register & D3D11Buffer_Texture) pBufferTypeStr = "TBuffer";
		else if (Obj.Register & D3D11Buffer_Structured) pBufferTypeStr = "SBuffer";
		else pBufferTypeStr = "CBuffer";
		n_msg(VL_DETAILS, "    %s: %s, %d slot(s) from %d\n", pBufferTypeStr, Obj.Name.CStr(), 1, Obj.Register);
	}

	W.Write<U32>(Meta.Consts.GetCount());
	for (UPTR i = 0; i < Meta.Consts.GetCount(); ++i)
	{
		const CD3D11ShaderConstMeta& Obj = Meta.Consts[i];
		W.Write(Obj.Name);
		W.Write(Obj.BufferIndex);
		W.Write(Obj.Offset);
		W.Write(Obj.Size);
		n_msg(VL_DETAILS, "      Const: %s, offset %d, size %d\n", //, type %d
				Obj.Name.CStr(), Obj.Offset, Obj.Size); //, D3DTypeDesc.Type);
	}

	W.Write<U32>(Meta.Resources.GetCount());
	for (UPTR i = 0; i < Meta.Resources.GetCount(); ++i)
	{
		//!!!arrays aren't supported for now!
		const CD3D11ShaderRsrcMeta& Obj = Meta.Resources[i];
		W.Write(Obj.Name);
		W.Write(Obj.Register);
		n_msg(VL_DETAILS, "    Texture: %s, %d slot(s) from %d\n", Obj.Name.CStr(), 1, Obj.Register);
	}

	W.Write<U32>(Meta.Samplers.GetCount());
	for (UPTR i = 0; i < Meta.Samplers.GetCount(); ++i)
	{
		//!!!arrays aren't supported for now!
		const CD3D11ShaderRsrcMeta& Obj = Meta.Samplers[i];
		W.Write(Obj.Name);
		W.Write(Obj.Register);
		n_msg(VL_DETAILS, "    Sampler: %s, %d slot(s) from %d\n", Obj.Name.CStr(), 1, Obj.Register);
	}

	OK;
}
//---------------------------------------------------------------------

void ReadRegisterRanges(CArray<UPTR>& UsedRegs, IO::CBinaryReader& R)
{
	UsedRegs.Clear();

	U32 RangeCount = 0;
	R.Read<U32>(RangeCount);
	for (UPTR r = 1; r < RangeCount; ++r)
	{
		U32 Curr;
		U32 CurrCount;
		R.Read<U32>(Curr);
		R.Read<U32>(CurrCount);
		U32 CurrEnd = Curr + CurrCount;
		for (; Curr < CurrEnd; ++Curr)
			UsedRegs.Add((UPTR)Curr);
	}
}
//---------------------------------------------------------------------

bool D3D9LoadShaderMetadata(IO::CBinaryReader& R, CD3D9ShaderMeta& Meta)
{
	Meta.Buffers.Clear();
	Meta.Consts.Clear();
	Meta.Samplers.Clear();

	U32 Count;

	R.Read<U32>(Count);
	CD3D9ShaderBufferMeta* pBuf = Meta.Buffers.Reserve(Count, false);
	for (; pBuf < Meta.Buffers.End(); ++pBuf)
	{
		CD3D9ShaderBufferMeta& Obj = *pBuf;
		Obj.UsedFloat4.Sort();
		Obj.UsedInt4.Sort();
		Obj.UsedBool.Sort();

		R.Read(Obj.Name);

		ReadRegisterRanges(Obj.UsedFloat4, R);
		ReadRegisterRanges(Obj.UsedInt4, R);
		ReadRegisterRanges(Obj.UsedBool, R);
	}

	R.Read<U32>(Count);
	CD3D9ShaderConstMeta* pConst = Meta.Consts.Reserve(Count, false);
	for (; pConst < Meta.Consts.End(); ++pConst)
	{
		CD3D9ShaderConstMeta& Obj = *pConst;

		U8 RegSet;
		switch (Obj.RegSet)
		{
			case RS_FLOAT4:	RegSet = 0; break;
			case RS_INT4:	RegSet = 1; break;
			case RS_BOOL:	RegSet = 2; break;
			default:		continue;
		};

		R.Read(Obj.Name);
		R.Read(Obj.BufferIndex);
		R.Read<U8>(RegSet);
		R.Read(Obj.Offset);
		R.Read(Obj.Size);

		const char* pRegisterSetName = NULL;
		switch (Obj.RegSet)
		{
			case RS_FLOAT4:	pRegisterSetName = "float4"; break;
			case RS_INT4:	pRegisterSetName = "int4"; break;
			case RS_BOOL:	pRegisterSetName = "bool"; break;
		};
	}

	R.Read<U32>(Count);
	CD3D9ShaderRsrcMeta* pSamp = Meta.Samplers.Reserve(Count, false);
	for (; pSamp < Meta.Samplers.End(); ++pSamp)
	{
		CD3D9ShaderRsrcMeta& Obj = *pSamp;
		R.Read(Obj.SamplerName);
		R.Read(Obj.TextureName);
		R.Read(Obj.Register);		//!!!need sampler arrays! need one texture to multiple samplers!
	}

	OK;
}
//---------------------------------------------------------------------

bool D3D11LoadShaderMetadata(IO::CBinaryReader& R, CD3D11ShaderMeta& Meta)
{
	Meta.Buffers.Clear();
	Meta.Consts.Clear();
	Meta.Resources.Clear();
	Meta.Samplers.Clear();

	U32 Count;

	R.Read<U32>(Count);
	CD3D11ShaderBufferMeta* pBuf = Meta.Buffers.Reserve(Count, false);
	for (; pBuf < Meta.Buffers.End(); ++pBuf)
	{
		//!!!arrays, tbuffers & sbuffers aren't supported for now!
		CD3D11ShaderBufferMeta& Obj = *pBuf;
		R.Read(Obj.Name);
		R.Read(Obj.Register);
		R.Read(Obj.ElementSize);
		R.Read(Obj.ElementCount);
	}

	R.Read<U32>(Count);
	CD3D11ShaderConstMeta* pConst = Meta.Consts.Reserve(Count, false);
	for (; pConst < Meta.Consts.End(); ++pConst)
	{
		CD3D11ShaderConstMeta& Obj = *pConst;
		R.Read(Obj.Name);
		R.Read(Obj.BufferIndex);
		R.Read(Obj.Offset);
		R.Read(Obj.Size);
	}

	R.Read<U32>(Count);
	CD3D11ShaderRsrcMeta* pRsrc = Meta.Resources.Reserve(Count, false);
	for (; pRsrc < Meta.Resources.End(); ++pRsrc)
	{
		//!!!arrays aren't supported for now!
		CD3D11ShaderRsrcMeta& Obj = *pRsrc;
		R.Read(Obj.Name);
		R.Read(Obj.Register);
	}

	R.Read<U32>(Count);
	CD3D11ShaderRsrcMeta* pSamp = Meta.Samplers.Reserve(Count, false);
	for (; pSamp < Meta.Samplers.End(); ++pSamp)
	{
		//!!!arrays aren't supported for now!
		CD3D11ShaderRsrcMeta& Obj = *pSamp;
		R.Read(Obj.Name);
		R.Read(Obj.Register);
	}

	OK;
}
//---------------------------------------------------------------------
