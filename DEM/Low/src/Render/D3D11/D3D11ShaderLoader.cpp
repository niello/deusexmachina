#include "D3D11ShaderLoader.h"

#include <Render/D3D11/D3D11DriverFactory.h>
#include <Render/D3D11/D3D11GPUDriver.h>
#include <Render/D3D11/D3D11Shader.h>
#include <Render/ShaderLibrary.h>
#include <IO/BinaryReader.h>

namespace Resources
{

const Core::CRTTI& CD3D11ShaderLoader::GetResultType() const
{
	return Render::CD3D11Shader::RTTI;
}
//---------------------------------------------------------------------

PResourceObject CD3D11ShaderLoader::CreateResource(CStrID UID)
{
	if (GPU.IsNullPtr() || !GPU->IsA<Render::CD3D11GPUDriver>()) return nullptr;

	const char* pSubId;
	IO::PStream Stream = OpenStream(UID, pSubId);
	if (!Stream) return nullptr;

	IO::CBinaryReader R(*Stream);

	Data::CFourCC FileSig;
	if (!R.Read(FileSig)) return nullptr;

	// Shader type is autodetected from the file signature
	Render::EShaderType ShaderType;
	switch (FileSig.Code)
	{
		case 'VS40':
		case 'VS41':
		case 'VS50':	ShaderType = Render::ShaderType_Vertex; break;
		case 'PS40':
		case 'PS41':
		case 'PS50':	ShaderType = Render::ShaderType_Pixel; break;
		case 'GS40':
		case 'GS41':
		case 'GS50':	ShaderType = Render::ShaderType_Geometry; break;
		case 'HS50':	ShaderType = Render::ShaderType_Hull; break;
		case 'DS50':	ShaderType = Render::ShaderType_Domain; break;
		default:		return nullptr;
	};

	U32 BinaryOffset;
	if (!R.Read(BinaryOffset)) return nullptr;

	U32 ShaderFileID;
	if (!R.Read(ShaderFileID)) return nullptr;

	U32 InputSignatureID;
	if (!R.Read(InputSignatureID)) return nullptr;

	U64 MetadataOffset = Stream->GetPosition();
	U64 FileSize = Stream->GetSize();
	UPTR BinarySize = (UPTR)FileSize - (UPTR)BinaryOffset;
	if (!BinarySize) return nullptr;
	void* pData = n_malloc(BinarySize);
	if (!pData) return nullptr;
	if (!Stream->Seek(BinaryOffset, IO::Seek_Begin) || Stream->Read(pData, BinarySize) != BinarySize)
	{
		n_free(pData);
		return nullptr;
	}

	void* pSigData = NULL;
	if (ShaderType == Render::ShaderType_Vertex) // || ShaderType == Render::ShaderType_Geometry)
	{
		// Vertex shader input comes from input assembler stage (IA). In D3D10 and later
		// input layouts (input signatures, vertex declarations) are created from VS
		// input signatures (or at least are validated against them). Input layout, once
		// created, can be reused with any vertex shader with the same input signature.

		if (!InputSignatureID) InputSignatureID = ShaderFileID;

		if (!D3D11DrvFactory->FindShaderInputSignature(InputSignatureID))
		{
			UPTR SigSize;
			if (InputSignatureID == ShaderFileID)
			{
				pSigData = pData;
				SigSize = BinarySize;
			}
			// TODO: in a new system signature shader ID will be built from in-library shader ID
			// by changing the subresource index. If shader is standalone (not in-library), its
			// signature ID must be the same as a shader file ID, so its own data will be used.
			else if (!ShaderLibrary->GetRawDataByID(InputSignatureID, pSigData, SigSize))
			{
				n_free(pData);
				return nullptr;
			}

			if (!D3D11DrvFactory->RegisterShaderInputSignature(InputSignatureID, pSigData, SigSize))
			{
				n_free(pData);
				n_free(pSigData);
				return nullptr;
			}
		}
	}

	Render::PD3D11Shader Shader = static_cast<Render::CD3D11Shader*>(GPU->CreateShader(ShaderType, pData, BinarySize).Get());
	if (Shader.IsNullPtr()) return nullptr;

	if (pSigData != pData) n_free(pData);

	Shader->InputSignatureID = InputSignatureID;

	if (!Stream->Seek(MetadataOffset, IO::Seek_Begin)) return nullptr;

	if (!Shader->Metadata.Load(*Stream)) return nullptr;

	return Shader.Get();
}
//---------------------------------------------------------------------

}