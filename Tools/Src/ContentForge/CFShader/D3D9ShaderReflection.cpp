#include "D3D9ShaderReflection.h"

#include <Data/StringUtils.h>

// Code is obtained from
// http://www.gamedev.net/topic/648016-replacement-for-id3dxconstanttable/
// This version has some cosmetic changes

#pragma pack(push)
#pragma pack(1)
struct CTHeader
{
	U32 Size;
	U32 Creator;
	U32 Version;
	U32 Constants;
	U32 ConstantInfo;
	U32 Flags;
	U32 Target;
};

struct CTInfo
{
	U32 Name;
	U16 RegisterSet;
	U16 RegisterIndex;
	U16 RegisterCount;
	U16 Reserved;
	U32 TypeInfo;
	U32 DefaultValue;
};

struct CTType
{
	U16 Class;
	U16 Type;
	U16 Rows;
	U16 Columns;
	U16 Elements;
	U16 StructMembers;
	U32 StructMemberInfo;
};
#pragma pack(pop)

const U32 SIO_COMMENT = 0x0000FFFE;
const U32 SIO_END = 0x0000FFFF;
const U32 SI_OPCODE_MASK = 0x0000FFFF;
const U32 SI_COMMENTSIZE_MASK = 0x7FFF0000;
const U32 CTAB_CONSTANT = 0x42415443;

bool D3D9Reflect(const void* pData, UPTR Size, CArray<CD3D9ConstantDesc>& OutConsts, CString& OutCreator)
{
	const U32* ptr = static_cast<const U32*>(pData);
	while(*++ptr != SIO_END)
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

			const CTHeader* header = reinterpret_cast<const CTHeader*>(ctab);
			if (ctab_size < sizeof(*header) || header->Size != sizeof(*header)) FAIL;
			OutCreator.Set(ctab + header->Creator);

			// Read constants
			CD3D9ConstantDesc* pDesc = OutConsts.Reserve(header->Constants);
			const CTInfo* info = reinterpret_cast<const CTInfo*>(ctab + header->ConstantInfo);
			for(U32 i = 0; i < header->Constants; ++i)
			{
				const CTType* type = reinterpret_cast<const CTType*>(ctab + info[i].TypeInfo);

				pDesc->Name = ctab + info[i].Name;
				pDesc->RegisterSet = static_cast<ERegisterSet>(info[i].RegisterSet);
				pDesc->RegisterIndex = info[i].RegisterIndex;
				pDesc->RegisterCount = info[i].RegisterCount;
				pDesc->Rows = type->Rows;
				pDesc->Columns = type->Columns;
				pDesc->Elements = type->Elements;
				pDesc->StructMembers = type->StructMembers;
				pDesc->Bytes = 4 * pDesc->Elements * pDesc->Rows * pDesc->Columns;
				++pDesc;
			}

			OK;
		}
	}

	FAIL;
}
//---------------------------------------------------------------------

// Parse source code and find "samplerX SamplerName { Texture = TextureName; }" pattern,
// as HLSL texture names are not saved in a metadata
void D3D9FindSamplerTextures(const char* pSrcText, UPTR Size, CDict<CString, CString>& OutSampToTex)
{
	char* pSrc = (char*)n_malloc(Size + 1);
	memcpy(pSrc, pSrcText, Size);
	pSrc[Size] = 0;

	Size = StringUtils::StripComments(pSrc);

	const char* pCurr = pSrc;
	while (pCurr = strstr(pCurr, "sampler"))
	{
		// Keyword 'sampler[XD]' and delimiters
		pCurr = strpbrk(pCurr, DEM_WHITESPACE);
		if (!pCurr) break;
		do ++pCurr; while (strchr(DEM_WHITESPACE, *pCurr));
		if (!*pCurr) break;

		// Sampler name
		const char* pSampName = pCurr;
		pCurr = strpbrk(pCurr, DEM_WHITESPACE";:{}");
		if (!pCurr) break;

		CString SamplerName(pSampName, pCurr - pSampName);

		// '{' - section start or ';' - sampler declaration end
		pCurr = strpbrk(pCurr, "{;");
		if (!pCurr) break;
		if (*pCurr == ';') continue;

		// 'Texture' token or '}' - section end
		const char* pSectionEnd = strpbrk(pCurr, "}");
		const char* pTextureToken = strstr(pCurr, "Texture");
		if (!pTextureToken || (pSectionEnd && pSectionEnd < pTextureToken)) continue;

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

		OutSampToTex.Add(SamplerName, TextureName);
	};

	n_free(pSrc);
}
//---------------------------------------------------------------------
