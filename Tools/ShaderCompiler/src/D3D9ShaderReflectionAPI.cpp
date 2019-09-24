#include "D3D9ShaderReflectionAPI.h"
#include <Utils.h>
#include <cassert>

// ProcessConstant() and D3D9Reflect() code is obtained from
// http://www.gamedev.net/topic/648016-replacement-for-id3dxconstanttable/
// This version has some cosmetic changes

#pragma pack(push)
#pragma pack(1)
struct CCTABHeader
{
	uint32_t Size;
	uint32_t Creator;
	uint32_t Version;
	uint32_t Constants;
	uint32_t ConstantInfo;
	uint32_t Flags;
	uint32_t Target;
};

struct CCTABInfo
{
	uint32_t Name;
	uint16_t RegisterSet;
	uint16_t RegisterIndex;
	uint16_t RegisterCount;
	uint16_t Reserved;
	uint32_t TypeInfo;
	uint32_t DefaultValue;
};

struct CCTABType
{
	uint16_t Class;
	uint16_t Type;
	uint16_t Rows;
	uint16_t Columns;
	uint16_t Elements;
	uint16_t StructMembers;
	uint32_t StructMemberInfo;
};

struct CCTABStructMemberInfo
{
	uint32_t Name;
	uint32_t TypeInfo;
};
#pragma pack(pop)

const uint32_t SIO_COMMENT = 0x0000FFFE;
const uint32_t SIO_END = 0x0000FFFF;
const uint32_t SI_OPCODE_MASK = 0x0000FFFF;
const uint32_t SI_COMMENTSIZE_MASK = 0x7FFF0000;
const uint32_t CTAB_CONSTANT = 0x42415443;			// 'CTAB' - constant table

//#define D3DXCONSTTABLE_LARGEADDRESSAWARE          0x20000

bool ProcessConstant(const char* ctab,
					 const CCTABInfo& ConstInfo,
					 const CCTABType& TypeInfo,
					 const char* pName,
					 size_t RegisterIndex,
					 size_t RegisterCount,
					 std::vector<CD3D9ConstantDesc>& OutConsts,
					 std::map<uint32_t, CD3D9StructDesc>& OutStructs)
{
	CD3D9ConstantDesc* pDesc = nullptr;
	for (size_t i = 0; i < OutConsts.size(); ++i)
	{
		if (OutConsts[i].Name == pName)
		{
			pDesc = &OutConsts[i];
			if (pDesc->Type.Class == PC_STRUCT)
			{
				// Mixed register set structure, not supported by DEM
				pDesc->RegisterSet = RS_MIXED;
				pDesc->RegisterIndex = 0;
				pDesc->RegisterCount = 0;
				return true;
			}
			else return false;
		}
	}

	if (!pDesc)
	{
		CD3D9ConstantDesc Desc;
		Desc.Name = pName;
		Desc.Type.Class = (EPARAMETER_CLASS)TypeInfo.Class;
		Desc.Type.Type = (EPARAMETER_TYPE)TypeInfo.Type;
		Desc.Type.Rows = TypeInfo.Rows;
		Desc.Type.Columns = TypeInfo.Columns;
		Desc.Type.Elements = TypeInfo.Elements > 0 ? TypeInfo.Elements : 1;
		Desc.Type.StructMembers = TypeInfo.StructMembers;
		OutConsts.push_back(std::move(Desc));
		pDesc = &OutConsts.back();
	}
 
	pDesc->RegisterSet = static_cast<EREGISTER_SET>(ConstInfo.RegisterSet);

	//pDesc->pDefaultValue = ConstInfo.DefaultValue ? ctab + ConstInfo.DefaultValue : nullptr;

	size_t ElementRegisterCount = 0;
	if (TypeInfo.Class == PC_STRUCT)
	{
		// If happens, search 'not a struct' and correct value in else section right below
		assert(TypeInfo.StructMemberInfo != 0);

		pDesc->StructID = TypeInfo.StructMemberInfo;

		auto It = OutStructs.find(TypeInfo.StructMemberInfo);
		if (It == OutStructs.end())
		{
			CD3D9StructDesc NewStructDesc;
			NewStructDesc.Bytes = 0;
			NewStructDesc.Type = PT_VOID;

			const CCTABStructMemberInfo* pMembers = (CCTABStructMemberInfo*)(ctab + TypeInfo.StructMemberInfo);
			for (uint16_t i = 0; i < TypeInfo.StructMembers; ++i)
			{
				const CCTABType* pMemberTypeInfo = reinterpret_cast<const CCTABType*>(ctab + pMembers[i].TypeInfo);
				if (!ProcessConstant(ctab, ConstInfo, *pMemberTypeInfo, ctab + pMembers[i].Name, ElementRegisterCount, RegisterCount - ElementRegisterCount, NewStructDesc.Members, OutStructs)) return false;
				ElementRegisterCount += NewStructDesc.Members[i].RegisterCount;
				NewStructDesc.Bytes += NewStructDesc.Members[i].Type.Bytes;

				if (NewStructDesc.Type == PT_VOID)
					NewStructDesc.Type = NewStructDesc.Members[i].Type.Type;
				else if (NewStructDesc.Type != NewStructDesc.Members[i].Type.Type)
					NewStructDesc.Type = PT_MIXED;
			}

			// Dictionary may be modified in recursive calls, pointer may become invalid, so
			// we fill structure on a stack and add only when done.
			It = OutStructs.emplace(TypeInfo.StructMemberInfo, std::move(NewStructDesc)).first;
		}

		pDesc->Type.Bytes = It->second.Bytes;
		pDesc->Type.Type = It->second.Type;
		if (It->second.Type == PT_MIXED) pDesc->RegisterSet = RS_MIXED;
	}
	else
	{
		//uint16_t offsetdiff = TypeInfo.Columns * TypeInfo.Rows;

		pDesc->StructID = 0; // not a struct

		switch (pDesc->RegisterSet)
		{
			case RS_SAMPLER:
			{
				ElementRegisterCount = 1;

				// Sampler array where some last elements aren't referenced
				if (pDesc->Type.Elements > RegisterCount)
					pDesc->Type.Elements = RegisterCount;

				break;
			}
			case RS_BOOL:
			case RS_INT4:
			{
				// Do not know why, but int registers are treated as int1, not int4.
				// They say only top-level integers behave this way, but it seems all of them do.
				ElementRegisterCount = TypeInfo.Columns * TypeInfo.Rows;
				break;
			}
			case RS_FLOAT4:
			{
				switch (TypeInfo.Class)
				{
					case PC_SCALAR:
						//offsetdiff = TypeInfo.Rows * 4;
						ElementRegisterCount = TypeInfo.Columns * TypeInfo.Rows;
						break;
 
					case PC_VECTOR:
						//offsetdiff = TypeInfo.Rows * 4;
						ElementRegisterCount = 1;
						break;
					
					case PC_MATRIX_ROWS:
						//offsetdiff = TypeInfo.Rows * 4;
						ElementRegisterCount = TypeInfo.Rows;
						break;
 
					case PC_MATRIX_COLUMNS:
						//offsetdiff = TypeInfo.Columns * 4;
						ElementRegisterCount = TypeInfo.Columns;
						break;
				}
			}
		}

		//if (pDefValOffset) *pDefValOffset += offsetdiff * 4; // offset in bytes => offsetdiff * sizeof(DWORD)

		pDesc->Type.Bytes = 4 * pDesc->Type.Elements * pDesc->Type.Rows * pDesc->Type.Columns;
	}

	pDesc->Type.ElementRegisterCount = ElementRegisterCount;
	pDesc->RegisterIndex = RegisterIndex;
	pDesc->RegisterCount = ElementRegisterCount * pDesc->Type.Elements;

	assert(pDesc->RegisterCount <= RegisterCount);

	return true;
}
//---------------------------------------------------------------------

bool D3D9Reflect(const void* pData, size_t Size, std::vector<CD3D9ConstantDesc>& OutConsts, std::map<uint32_t, CD3D9StructDesc>& OutStructs, std::string& OutCreator)
{
	const uint32_t* ptr = static_cast<const uint32_t*>(pData);
	while (*++ptr != SIO_END)
	{
		if ((*ptr & SI_OPCODE_MASK) == SIO_COMMENT)
		{
			// Check for CTAB comment
			uint32_t comment_size = (*ptr & SI_COMMENTSIZE_MASK) >> 16;
			if (*(ptr + 1) != CTAB_CONSTANT)
			{
				ptr += comment_size;
				continue;
			}

			// Read header
			const char* ctab = reinterpret_cast<const char*>(ptr + 2);
			size_t ctab_size = (comment_size - 1) * 4;

			const CCTABHeader* header = reinterpret_cast<const CCTABHeader*>(ctab);
			if (ctab_size < sizeof(*header) || header->Size != sizeof(*header)) return false;
			OutCreator.assign(ctab + header->Creator);

			// Read constants
			const CCTABInfo* info = reinterpret_cast<const CCTABInfo*>(ctab + header->ConstantInfo);
			for (uint32_t i = 0; i < header->Constants; ++i)
			{
				const CCTABType* pTypeInfo = reinterpret_cast<const CCTABType*>(ctab + info[i].TypeInfo);
				if (!ProcessConstant(ctab, info[i], *pTypeInfo, ctab + info[i].Name, info[i].RegisterIndex, info[i].RegisterCount, OutConsts, OutStructs)) return false;
			}

			return true;
		}
	}

	return false;
}
//---------------------------------------------------------------------

// Pass only preprocessed code here.
// Parses source code and finds "samplerX SamplerName { Texture = TextureName; }" or
// "sampler SamplerName[N] { { Texture = Tex1; }, ..., { Texture = TexN; } }" pattern,
// as HLSL texture names are not saved in a metadata. Doesn't support annotations.
void D3D9FindSamplerTextures(const char* pSrcText, std::map<std::string, std::vector<std::string>>& OutSampToTex)
{
	const char* pCurr = pSrcText;
	while (pCurr = strstr(pCurr, "sampler"))
	{
		// Keyword 'sampler[XD]'
		pCurr = strpbrk(pCurr, " \r\n\t");
		if (!pCurr) break;
		
		// Skip delimiters after a keyword
		do ++pCurr; while (strchr(" \r\n\t", *pCurr));
		if (!*pCurr) break;

		// Sampler name
		const char* pSampName = pCurr;
		pCurr = strpbrk(pCurr, " \r\n\t[;:{}");
		if (!pCurr) break;

		std::string SamplerName(pSampName, pCurr - pSampName);

		bool IsArray = (*pCurr == '[');

		// Parse optional array
		if (IsArray)
		{
			// '{' - array initializer list start or ';' - sampler declaration end (we skip 'N]' part)
			pCurr = strpbrk(pCurr, "{;");
			if (!pCurr) break;
			if (*pCurr == ';') continue;
			++pCurr;
		}

		auto It = OutSampToTex.find(SamplerName);
		auto* pTexNames = (It == OutSampToTex.end()) ? &OutSampToTex[SamplerName] : &It->second;

		do
		{
			// '{' - section start or '}' - array initializer list end or ';' - sampler declaration end
			const char* pTerm = IsArray ? "{}" : "{;";
			pCurr = strpbrk(pCurr, pTerm);
			if (!pCurr || *pCurr == ';' || *pCurr == '}') break;

			// 'Texture' token or '}' - section end
			const char* pSectionEnd = strpbrk(pCurr, "}");
			const char* pTextureToken = strstr(pCurr, "Texture");
			if (!pTextureToken)
			{
				// Nothing more to parse, as we look for 'Texture' tokens
				pCurr = nullptr;
				break;
			}
			if (pSectionEnd && pSectionEnd < pTextureToken)
			{
				// Empty section, skip it, add empty texture name to preserve register mapping
				pTexNames->push_back(std::string());
				pCurr = pSectionEnd + 1;
				continue;
			}

			// '=' of a texture declaration
			pCurr = strchr(pTextureToken, '=');

			// Skip delimiters after a '='
			do ++pCurr; while (strchr(" \r\n\t", *pCurr));
			if (!*pCurr) break;

			// Texture name
			const char* pTexName = pCurr;
			pCurr = strpbrk(pCurr, " \r\n\t;:{}");
			if (!pCurr) break;

			std::string TextureName(pTexName, pCurr - pTexName);
			trim(TextureName, " \r\n\t'\"");

			pTexNames->push_back(std::move(TextureName));

			// Proceed to the end of the section
			pCurr = pSectionEnd + 1;
		}
		while (true);

		if (!pCurr) break;
	}
}
//---------------------------------------------------------------------

// Pass only preprocessed code here.
// Parses source code, finds annotations of shader constats and fills additional constant metadata.
void D3D9FindConstantBuffer(const char* pSrcText, const std::string& ConstName, std::string& OutBufferName, uint32_t& OutSlotIndex)
{
	bool CBufferFound = false;
	const char* pCurr = pSrcText;
	while (pCurr = strstr(pCurr, ConstName.c_str()))
	{
		// Ensure it is a whole word, if not, search next
		if (!strchr(" \r\n\t", *(pCurr - 1)) || !strchr(" \r\n\t=:<[", *(pCurr + ConstName.size())))
		{
			pCurr += ConstName.size();
			continue;
		}

		// Annotation start or statement/declaration end
		pCurr = strpbrk(pCurr, "<;");
		if (!pCurr) break;
		if (*pCurr != '<') continue;
		++pCurr;
		const char* pAnnotationStart = pCurr;

		// Annotation end
		const char* pAnnotationEnd = strpbrk(pCurr, ">");
		if (!pAnnotationEnd) break;

		// Find 'CBuffer' annotation
		while (pCurr = strstr(pCurr, "CBuffer"))
		{
			// Reached the end of annotation
			if (pCurr >= pAnnotationEnd) break;

			// Ensure it is a whole word, if not, search for next matching annotation
			if (!strchr(" \r\n\t", *(pCurr - 1)) || !strchr(" \r\n\t=", *(pCurr + 7)))
			{
				pCurr = strpbrk(pCurr + 7, ";");
				if (!pCurr) break;
				continue;
			}

			// Find string value start or 'CBuffer' annotation end
			pCurr = strpbrk(pCurr, "\";");
			if (!pCurr || *pCurr == ';') break;

			++pCurr;
			const char* pNameStart = pCurr;

			// Find string value end or 'CBuffer' annotation end (which means no closing '\"' exist)
			pCurr = strpbrk(pCurr, "\";");
			if (!pCurr || *pCurr == ';') break;

			OutBufferName.assign(pNameStart, pCurr - pNameStart);
			CBufferFound = true;
			break;
		}

		// Find 'SlotIndex' annotation
		pCurr = pAnnotationStart;
		while (pCurr = strstr(pCurr, "SlotIndex"))
		{
			// Reached the end of annotation
			if (pCurr >= pAnnotationEnd) break;

			// Ensure it is a whole word, if not, search for next matching annotation
			if (!strchr(" \r\n\t", *(pCurr - 1)) || !strchr(" \r\n\t=", *(pCurr + 9)))
			{
				pCurr = strpbrk(pCurr + 9, ";");
				if (!pCurr) break;
				continue;
			}

			// Find value start
			pCurr = strpbrk(pCurr, "=");
			if (!pCurr) break;
			++pCurr;
			if (!*pCurr) break;

			const char* pIntValueStart = pCurr;

			// Find 'SlotIndex' annotation end
			pCurr = strpbrk(pCurr, ";");
			if (!pCurr) break;

			const char* pIntValueEnd = pCurr;

			std::string IntValue(pIntValueStart, pIntValueEnd - pIntValueStart);
			trim(IntValue, " \r\n\t");

			OutSlotIndex = std::atoi(IntValue.c_str());
			break;
		}

		// Annotation block was found, no chance to find another one
		break;
	}

	if (!CBufferFound) OutBufferName = "$Global";
}
//---------------------------------------------------------------------
