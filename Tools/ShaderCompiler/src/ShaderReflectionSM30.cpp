#include <ShaderMeta/SM30ShaderMeta.h>
#include <LogDelegate.h>
#include <D3D9ShaderReflectionAPI.h>
#include <algorithm>
#include <cassert>
#define WIN32_LEAN_AND_MEAN
#include <d3dcompiler.h>

#undef min
#undef max

bool ExtractSM30MetaFromBinaryAndSource(CSM30ShaderMeta& OutMeta, const void* pData, size_t Size,
	const char* pSource, size_t SourceSize, ID3DInclude* pInclude, const char* pSourcePath,
	const D3D_SHADER_MACRO* pDefines, DEMShaderCompiler::ILogDelegate* pLog)
{
	std::vector<CD3D9ConstantDesc> D3D9Consts;
	std::map<uint32_t, CD3D9StructDesc> D3D9Structs;
	std::string Creator;

	if (!D3D9Reflect(pData, Size, D3D9Consts, D3D9Structs, Creator)) return false;

	// Preprocess the source code

	ID3DBlob* pCodeText = nullptr;
	ID3DBlob* pErrorMsgs = nullptr;
	HRESULT hr = D3DPreprocess(pSource, SourceSize, pSourcePath, pDefines, pInclude, &pCodeText, &pErrorMsgs);

	if (FAILED(hr) || !pCodeText)
	{
		if (pLog) pLog->LogError(pErrorMsgs ? (const char*)pErrorMsgs->GetBufferPointer() : "<No D3D error message>");
		if (pCodeText) pCodeText->Release();
		if (pErrorMsgs) pErrorMsgs->Release();
		return false;
	}
	else if (pErrorMsgs)
	{
		if (pLog)
		{
			pLog->LogWarning("Preprocessed with warnings:\n");
			pLog->LogWarning((const char*)pErrorMsgs->GetBufferPointer());
		}
		pErrorMsgs->Release();
	}

	const std::string Source = static_cast<const char*>(pCodeText->GetBufferPointer());
	pCodeText->Release();

	// Collect structure layout metadata

	// Since D3D9Structs is a map, its elements have no indices. We generate linear indices for them.
	// These indices are valid to reference structs in a Structs vector.
	std::map<uint32_t, uint32_t> StructIDToIndex;
	size_t Idx = 0;
	for (const auto& Pair : D3D9Structs)
		StructIDToIndex.emplace(Pair.first, Idx++);

	OutMeta.Structs.reserve(D3D9Structs.size());
	for (const auto& Pair : D3D9Structs)
	{
		const CD3D9StructDesc& D3D9StructDesc = Pair.second;

		CSM30StructMeta StructMeta;

		StructMeta.Members.reserve(D3D9StructDesc.Members.size());
		for (const auto& D3D9ConstDesc : D3D9StructDesc.Members)
		{
			auto ItStruct = StructIDToIndex.find(D3D9ConstDesc.StructID);

			CSM30StructMemberMeta MemberMeta;

			MemberMeta.Name = D3D9ConstDesc.Name;
			MemberMeta.StructIndex = (ItStruct == StructIDToIndex.cend()) ? (uint32_t(-1)) : ItStruct->second;
			MemberMeta.RegisterOffset = D3D9ConstDesc.RegisterIndex;
			MemberMeta.ElementRegisterCount = D3D9ConstDesc.Type.ElementRegisterCount;
			MemberMeta.ElementCount = D3D9ConstDesc.Type.Elements;
			MemberMeta.Columns = static_cast<uint8_t>(D3D9ConstDesc.Type.Columns);
			MemberMeta.Rows = static_cast<uint8_t>(D3D9ConstDesc.Type.Rows);
			MemberMeta.Flags = 0;

			if (D3D9ConstDesc.Type.Class == PC_MATRIX_COLUMNS)
				MemberMeta.Flags |= SM30ShaderConst_ColumnMajor;

			assert(MemberMeta.ElementRegisterCount * MemberMeta.ElementCount == D3D9ConstDesc.RegisterCount);

			StructMeta.Members.push_back(std::move(MemberMeta));
		}

		OutMeta.Structs.push_back(std::move(StructMeta));
	}

	// Collect constant metadata

	std::map<std::string, std::vector<std::string>> SampToTex;
	D3D9FindSamplerTextures(Source.c_str(), SampToTex);

	std::map<std::string, std::string> ConstToBuf;

	for (size_t i = 0; i < D3D9Consts.size(); ++i)
	{
		CD3D9ConstantDesc& D3D9ConstDesc = D3D9Consts[i];

		if (D3D9ConstDesc.RegisterSet == RS_MIXED)
		{
			if (pLog) pLog->LogWarning(("    SM3.0 mixed-regset structs aren't supported, '" + D3D9ConstDesc.Name + "' skipped").c_str());
			continue;
		}
		else if (D3D9ConstDesc.RegisterSet == RS_SAMPLER)
		{
			CSM30SamplerMeta Meta;
			Meta.Name = D3D9ConstDesc.Name;
			Meta.RegisterStart = D3D9ConstDesc.RegisterIndex;
			Meta.RegisterCount = D3D9ConstDesc.RegisterCount;

			switch (D3D9ConstDesc.Type.Type)
			{
				case PT_SAMPLER1D:		Meta.Type = SM30Sampler_1D; break;
				case PT_SAMPLER3D:		Meta.Type = SM30Sampler_3D; break;
				case PT_SAMPLERCUBE:	Meta.Type = SM30Sampler_CUBE; break;
				default:				Meta.Type = SM30Sampler_2D; break;
			}

			size_t TexCount;
			auto STIt = SampToTex.find(D3D9ConstDesc.Name);
			if (STIt == SampToTex.cend()) TexCount = 0;
			else
			{
				const std::vector<std::string>& TexNames = STIt->second;
				TexCount = std::min(D3D9ConstDesc.RegisterCount, TexNames.size());
				for (size_t TexIdx = 0; TexIdx < TexCount; ++TexIdx)
				{
					const std::string& TexName = TexNames[TexIdx];
					if (!TexName.empty())
					{
						CSM30RsrcMeta Meta;
						Meta.Name = TexName;
						Meta.Register = D3D9ConstDesc.RegisterIndex + TexIdx;
						OutMeta.Resources.push_back(std::move(Meta));
					}
					else if (D3D9ConstDesc.RegisterCount > 1)
					{
						if (pLog) pLog->LogWarning(("Sampler '" + D3D9ConstDesc.Name + '[' + std::to_string(TexIdx) + "] has no texture bound, use initializer in a form of 'samplerX SamplerName[N] { { Texture = TextureName1; }, ..., { Texture = TextureNameN; } }'").c_str());
					}
					else
					{
						if (pLog) pLog->LogWarning(("Sampler '" + D3D9ConstDesc.Name + "' has no texture bound, use initializer in a form of 'samplerX SamplerName { Texture = TextureName; }'").c_str());
					}
				}
			}

			if (!TexCount)
			{
				if (pLog) pLog->LogWarning(("Sampler '" + D3D9ConstDesc.Name + "' has no textures bound, use initializer in a form of 'samplerX SamplerName { Texture = TextureName; }' or 'samplerX SamplerName[N] { { Texture = TextureName1; }, ..., { Texture = TextureNameN; } }'").c_str());
			}

			OutMeta.Samplers.push_back(std::move(Meta));
		}
		else // Constants
		{
			std::string BufferName;
			uint32_t SlotIndex = (uint32_t)(-1);
			D3D9FindConstantBuffer(Source.c_str(), D3D9ConstDesc.Name, BufferName, SlotIndex);

			size_t BufferIndex = 0;
			for (; BufferIndex < OutMeta.Buffers.size(); ++BufferIndex)
				if (OutMeta.Buffers[BufferIndex].Name == BufferName) break;

			if (BufferIndex == OutMeta.Buffers.size())
			{
				CSM30BufferMeta Meta;
				Meta.Name = BufferName;
				Meta.SlotIndex = SlotIndex;
				OutMeta.Buffers.push_back(std::move(Meta));
			}
			else
			{
				CSM30BufferMeta& Meta = OutMeta.Buffers[BufferIndex];
				if (SlotIndex == (uint32_t)(-1))
				{
					Meta.SlotIndex = SlotIndex;
				}
				else if (SlotIndex != Meta.SlotIndex)
				{
					if (pLog) pLog->LogWarning(("CBuffer '" + Meta.Name + "' is bound to different SlotIndex values (at least " +
						std::to_string(SlotIndex) + " and " + std::to_string(Meta.SlotIndex) + ") in the same shader, please fix it\n").c_str());
					return false;
				}
			}

			CSM30ConstMeta Meta;
			Meta.Name = D3D9ConstDesc.Name;
			Meta.BufferIndex = BufferIndex;

			auto ItStruct = StructIDToIndex.find(D3D9ConstDesc.StructID);
			Meta.StructIndex = (ItStruct == StructIDToIndex.cend()) ? (uint32_t(-1)) : ItStruct->second;

			switch (D3D9ConstDesc.RegisterSet)
			{
				case RS_FLOAT4:	Meta.RegisterSet = RS_Float4; break;
				case RS_INT4:	Meta.RegisterSet = RS_Int4; break;
				case RS_BOOL:	Meta.RegisterSet = RS_Bool; break;
				default:
				{
					if (pLog) pLog->LogError(("Unsupported SM3.0 register set " + std::to_string(D3D9ConstDesc.RegisterSet)).c_str());
					return false;
				}
			};

			Meta.RegisterStart = D3D9ConstDesc.RegisterIndex;
			Meta.ElementRegisterCount = D3D9ConstDesc.Type.ElementRegisterCount;
			Meta.ElementCount = D3D9ConstDesc.Type.Elements;
			Meta.Columns = static_cast<uint8_t>(D3D9ConstDesc.Type.Columns);
			Meta.Rows = static_cast<uint8_t>(D3D9ConstDesc.Type.Rows);
			Meta.Flags = 0;

			if (D3D9ConstDesc.Type.Class == PC_MATRIX_COLUMNS)
				Meta.Flags |= SM30ShaderConst_ColumnMajor;

			// Cache value
			Meta.RegisterCount = Meta.ElementRegisterCount * Meta.ElementCount;

			assert(Meta.RegisterCount == D3D9ConstDesc.RegisterCount);

			CSM30BufferMeta& BufMeta = OutMeta.Buffers[Meta.BufferIndex];
			auto& UsedRegs =
				(Meta.RegisterSet == RS_Float4) ? BufMeta.UsedFloat4 :
				((Meta.RegisterSet == RS_Int4) ? BufMeta.UsedInt4 :
					BufMeta.UsedBool);
			for (uint32_t r = D3D9ConstDesc.RegisterIndex; r < D3D9ConstDesc.RegisterIndex + D3D9ConstDesc.RegisterCount; ++r)
			{
				UsedRegs.emplace(r);
			}

			OutMeta.Consts.push_back(std::move(Meta));
		}
	}

	// Remove empty constant buffers and assign free slots to buffers for which no explicit value was specified

	OutMeta.Buffers.erase(std::remove_if(OutMeta.Buffers.begin(), OutMeta.Buffers.end(), [](const CSM30BufferMeta& Buffer)
	{
		return !Buffer.UsedFloat4.size() && !Buffer.UsedInt4.size() && !Buffer.UsedBool.size();
	}), OutMeta.Buffers.end());

	std::set<uint32_t> UsedSlotIndices;
	for (const auto& Buffer : OutMeta.Buffers)
		UsedSlotIndices.emplace(Buffer.SlotIndex);

	uint32_t NewSlotIndex = 0;
	for (size_t i = 0; i < OutMeta.Buffers.size(); ++i)
	{
		CSM30BufferMeta& Buffer = OutMeta.Buffers[i];
		if (Buffer.SlotIndex == (uint32_t)(-1))
		{
			while (UsedSlotIndices.find(NewSlotIndex) != UsedSlotIndices.cend())
				++NewSlotIndex;
			Buffer.SlotIndex = NewSlotIndex;
			++NewSlotIndex;
		}
	}

	return true;
}
//---------------------------------------------------------------------
