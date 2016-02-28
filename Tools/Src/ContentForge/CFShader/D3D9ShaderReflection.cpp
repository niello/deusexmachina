#include "D3D9ShaderReflection.h"

#include <Data/StringUtils.h>

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
			CD3D9ConstantDesc* pDesc = OutConsts.Reserve(header->Constants);
			const CCTABInfo* info = reinterpret_cast<const CCTABInfo*>(ctab + header->ConstantInfo);
			for(U32 i = 0; i < header->Constants; ++i, ++pDesc)
			{
				const CCTABType* pTypeInfo = reinterpret_cast<const CCTABType*>(ctab + info[i].TypeInfo);

				pDesc->Name = ctab + info[i].Name;
				pDesc->RegisterSet = static_cast<EREGISTER_SET>(info[i].RegisterSet);
				pDesc->RegisterIndex = info[i].RegisterIndex;
				pDesc->RegisterCount = info[i].RegisterCount;
				pDesc->Class = (EPARAMETER_CLASS)pTypeInfo->Class;
				pDesc->Type = (EPARAMETER_TYPE)pTypeInfo->Type;
				pDesc->Rows = pTypeInfo->Rows;
				pDesc->Columns = pTypeInfo->Columns;
				pDesc->Elements = pTypeInfo->Elements; // is_element ? 1 : pTypeInfo->Elements; top level always is_element FALSE
				pDesc->StructMembers = pTypeInfo->StructMembers;
				pDesc->Bytes = 4 * pDesc->Elements * pDesc->Rows * pDesc->Columns;

				//pDesc->pDefaultValue = info[i].DefaultValue ? ctab + info[i].DefaultValue : NULL;

				//WORD max_index = constant_info[i].RegisterIndex + constant_info[i].RegisterCount;

				UPTR ChildCount;
				if (pDesc->Elements > 1) // && !is_element
				{
					ChildCount = pDesc->Elements;
/* For arrays of structs:
//???add constants for each or accept index in an engine's SetConst method?
may store array of constants each with its registers but indexable as it is an array actually
         for (i = 0; i < count; ++i)
         {
             checkhr = parse_ctab_constant_type(ctab, typeoffset,
                     &constant->constants[i], TRUE, index + size, max_index, offset,
                     nameoffset, regset);
 
             size += constant->constants[i].desc.RegisterCount;
         }
*/
				}
				else if (pDesc->Class == PC_STRUCT && pDesc->StructMembers > 0)
				{
					ChildCount = pDesc->StructMembers;
					//memberinfo = (CCTABStructMemberInfo*)(ctab + pTypeInfo->StructMemberInfo);
/*
         for (i = 0; i < count; ++i)
         {
             checkhr = parse_ctab_constant_type(ctab, memberinfo[i].TypeInfo,
                     &constant->constants[i], FALSE, index + size, max_index, offset,
                     memberinfo[i].Name, regset);
 
             size += constant->constants[i].desc.RegisterCount;
         }
*/
				}
				else
				{
					/*
1878         WORD offsetdiff = pTypeInfo->Columns * pTypeInfo->Rows;
1880 
1881         size = pTypeInfo->Columns * pTypeInfo->Rows;
1882 
1883         switch (regset)
1884         {
1885             case D3DXRS_BOOL:
1888                 break;
1889 
1890             case D3DXRS_FLOAT4:
1891             case D3DXRS_INT4:
1892                 switch (pTypeInfo->Class)
1893                 {
1894                     case D3DXPC_VECTOR:
1895                         size = 1;
1896                         // fall through 
1897                     case D3DXPC_SCALAR:
1898                         offsetdiff = pTypeInfo->Rows * 4;
1899                         break;
1900 
1901                     case D3DXPC_MATRIX_ROWS:
1902                         offsetdiff = pTypeInfo->Rows * 4;
1903                         size = pTypeInfo->Rows;
1904                         break;
1905 
1906                     case D3DXPC_MATRIX_COLUMNS:
1907                         offsetdiff = pTypeInfo->Columns * 4;
1908                         size = pTypeInfo->Columns;
1909                         break;
1914                 }
1915                 break;
1916 
1917             case D3DXRS_SAMPLER:
1918                 size = 1;
1920                 break;
1925         }
1932 
1933         
1934         if (pDefValOffset) *pDefValOffset += offsetdiff * 4; // offset in bytes => offsetdiff * sizeof(DWORD)
					*/
				}

/*
1937     pDesc->RegisterCount = max(0, min(max_index - index, size));
1938     pDesc->Bytes = 4 * pDesc->Elements * pTypeInfo->Rows * pTypeInfo->Columns;
*/

				// Top level const's register count may be 4 times bigger as as they assume register size is 1 instead of 4
				if (pDesc->RegisterSet == RS_INT4)
				{
					pDesc->RegisterCount = info[i].RegisterCount;
				}
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
