#include "D3D9ShaderReflection.h"

#include <Data/StringUtils.h>
#include <ConsoleApp.h>			// For n_msg

// Code is obtained from
// http://www.gamedev.net/topic/648016-replacement-for-id3dxconstanttable/
// This version has some cosmetic changes

#pragma pack(push)
#pragma pack(1)
struct CCTABHeader
{
	U32 Size;
	U32 Creator;
	U32 Version;
	U32 Constants;
	U32 ConstantInfo;
	U32 Flags;
	U32 Target;
};

struct CCTABInfo
{
	U32 Name;
	U16 RegisterSet;
	U16 RegisterIndex;
	U16 RegisterCount;
	U16 Reserved;
	U32 TypeInfo;
	U32 DefaultValue;
};

struct CCTABType
{
	U16 Class;
	U16 Type;
	U16 Rows;
	U16 Columns;
	U16 Elements;
	U16 StructMembers;
	U32 StructMemberInfo;
};

struct CCTABStructMemberInfo
{
	U32 Name;
	U32 TypeInfo;
};
#pragma pack(pop)

const U32 SIO_COMMENT = 0x0000FFFE;
const U32 SIO_END = 0x0000FFFF;
const U32 SI_OPCODE_MASK = 0x0000FFFF;
const U32 SI_COMMENTSIZE_MASK = 0x7FFF0000;
const U32 CTAB_CONSTANT = 0x42415443;			// 'CTAB' - constant table

//#define D3DXCONSTTABLE_LARGEADDRESSAWARE          0x20000

bool ProcessConstant(const char* ctab,
					 const CCTABInfo& ConstInfo,
					 const CCTABType& TypeInfo,
					 const char* pName,
					 UPTR RegisterIndex,
					 UPTR RegisterCount,
					 CArray<CD3D9ConstantDesc>& OutConsts)
{
	CD3D9ConstantDesc* pDesc = NULL;
	for (UPTR i = 0; i < OutConsts.GetCount(); ++i)
	{
		if (OutConsts[i].Name == pName)
		{
			pDesc = &OutConsts[i];
			if (pDesc->Type.Class == PC_STRUCT)
			{
				// Mixed-registerset structure, not supported by DEM
				pDesc->RegisterSet = RS_MIXED;
				pDesc->RegisterIndex = 0;
				pDesc->RegisterCount = 0;
				OK;
			}
			else
			{
				n_msg(VL_ERROR, "Duplicate parameter name '%s' in SM3.0 shader\n", pName);
				FAIL;
			}
			break;
		}
	}

	if (!pDesc)
	{
		pDesc = OutConsts.Add();
		pDesc->Name = pName;
		pDesc->Type.Class = (EPARAMETER_CLASS)TypeInfo.Class;
		pDesc->Type.Type = (EPARAMETER_TYPE)TypeInfo.Type;
		pDesc->Type.Rows = TypeInfo.Rows;
		pDesc->Type.Columns = TypeInfo.Columns;
		pDesc->Type.Elements = TypeInfo.Elements > 0 ? TypeInfo.Elements : 1;
		pDesc->Type.StructMembers = TypeInfo.StructMembers;
	}
 
	pDesc->RegisterSet = static_cast<EREGISTER_SET>(ConstInfo.RegisterSet);

	//pDesc->pDefaultValue = ConstInfo.DefaultValue ? ctab + ConstInfo.DefaultValue : NULL;

	UPTR ElementRegisterCount = 0;
	if (TypeInfo.Class == PC_STRUCT)
	{
		pDesc->Type.Bytes = 0;
		UPTR ChildCount = TypeInfo.StructMembers;
		const CCTABStructMemberInfo* pMembers = (CCTABStructMemberInfo*)(ctab + TypeInfo.StructMemberInfo);
		for (UPTR i = 0; i < ChildCount; ++i)
		{
			const CCTABType* pMemberTypeInfo = reinterpret_cast<const CCTABType*>(ctab + pMembers[i].TypeInfo);
			if (!ProcessConstant(ctab, ConstInfo, *pMemberTypeInfo, ctab + pMembers[i].Name, RegisterIndex + ElementRegisterCount, RegisterCount - ElementRegisterCount, pDesc->Members)) FAIL;
			ElementRegisterCount += pDesc->Members[i].RegisterCount;
			pDesc->Type.Bytes += pDesc->Members[i].Type.Bytes;
		}
	}
	else
	{
		//U16 offsetdiff = TypeInfo.Columns * TypeInfo.Rows;

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

	n_assert(pDesc->RegisterCount <= RegisterCount);

	OK;
}
//---------------------------------------------------------------------

bool D3D9Reflect(const void* pData, UPTR Size, CArray<CD3D9ConstantDesc>& OutConsts, CString& OutCreator)
{
	const U32* ptr = static_cast<const U32*>(pData);
	while (*++ptr != SIO_END)
	{
		if ((*ptr & SI_OPCODE_MASK) == SIO_COMMENT)
		{
			// Check for CTAB comment
			U32 comment_size = (*ptr & SI_COMMENTSIZE_MASK) >> 16;
			if (*(ptr + 1) != CTAB_CONSTANT)
			{
				ptr += comment_size;
				continue;
			}

			// Read header
			const char* ctab = reinterpret_cast<const char*>(ptr + 2);
			UPTR ctab_size = (comment_size - 1) * 4;

			const CCTABHeader* header = reinterpret_cast<const CCTABHeader*>(ctab);
			if (ctab_size < sizeof(*header) || header->Size != sizeof(*header)) FAIL;
			OutCreator.Set(ctab + header->Creator);

			// Read constants
			const CCTABInfo* info = reinterpret_cast<const CCTABInfo*>(ctab + header->ConstantInfo);
			for (U32 i = 0; i < header->Constants; ++i)
			{
				const CCTABType* pTypeInfo = reinterpret_cast<const CCTABType*>(ctab + info[i].TypeInfo);
				if (!ProcessConstant(ctab, info[i], *pTypeInfo, ctab + info[i].Name, info[i].RegisterIndex, info[i].RegisterCount, OutConsts)) FAIL;
			}

			OK;
		}
	}

	FAIL;
}
//---------------------------------------------------------------------

// Parse source code and find "samplerX SamplerName { Texture = TextureName; }" or
// "sampler LinearSampler[N] { { Texture = Tex1; }, ..., { Texture = TexN; } }" pattern,
// as HLSL texture names are not saved in a metadata
void D3D9FindSamplerTextures(const char* pSrcText, UPTR Size, CDict<CString, CArray<CString>>& OutSampToTex)
{
	char* pSrc = (char*)n_malloc(Size + 1);
	memcpy(pSrc, pSrcText, Size);
	pSrc[Size] = 0;

	Size = StringUtils::StripComments(pSrc);

	const char* pCurr = pSrc;
	while (pCurr = strstr(pCurr, "sampler"))
	{
		// Keyword 'sampler[XD]'
		pCurr = strpbrk(pCurr, DEM_WHITESPACE);
		if (!pCurr) break;
		
		// Skip delimiters after a keyword
		do ++pCurr; while (strchr(DEM_WHITESPACE, *pCurr));
		if (!*pCurr) break;

		// Sampler name
		const char* pSampName = pCurr;
		pCurr = strpbrk(pCurr, DEM_WHITESPACE"[;:{}");
		if (!pCurr) break;

		CString SamplerName(pSampName, pCurr - pSampName);

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

		CArray<CString>* pTexNames;
		IPTR Idx = OutSampToTex.FindIndex(SamplerName);
		if (Idx == INVALID_INDEX) pTexNames = &OutSampToTex.Add(SamplerName);
		else pTexNames = &OutSampToTex.ValueAt(Idx);

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
				pCurr = NULL;
				break;
			}
			if (pSectionEnd && pSectionEnd < pTextureToken)
			{
				// Empty section, skip it, add empty texture name to preserve register mapping
				pTexNames->Add(CString::Empty);
				pCurr = pSectionEnd + 1;
				continue;
			}

			// '=' of a texture declaration
			pCurr = strchr(pTextureToken, '=');

			// Skip delimiters after a '='
			do ++pCurr; while (strchr(DEM_WHITESPACE, *pCurr));
			if (!*pCurr) break;

			// Texture name
			const char* pTexName = pCurr;
			pCurr = strpbrk(pCurr, DEM_WHITESPACE";:{}");
			if (!pCurr) break;

			CString TextureName(pTexName, pCurr - pTexName);
			TextureName.Trim(DEM_WHITESPACE"'\"");

			pTexNames->Add(TextureName);

			// Proceed to the end of the section
			pCurr = pSectionEnd + 1;
		}
		while (true);

		if (!pCurr) break;
	};

	n_free(pSrc);
}
//---------------------------------------------------------------------
