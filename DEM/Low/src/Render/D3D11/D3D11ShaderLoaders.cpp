#include "D3D11ShaderLoaders.h"

#include <Render/D3D11/D3D11DriverFactory.h>
#include <Render/D3D11/D3D11GPUDriver.h>
#include <Render/D3D11/D3D11Shader.h>
#include <Render/ShaderLibrary.h>
#include <IO/BinaryReader.h>
#include <Core/Factory.h>

namespace Resources
{

const Core::CRTTI& CD3D11ShaderLoader::GetResultType() const
{
	return Render::CD3D11Shader::RTTI;
}
//---------------------------------------------------------------------

PResourceObject CD3D11ShaderLoader::LoadImpl(CStrID UID, Render::EShaderType ShaderType)
{
	if (GPU.IsNullPtr() || !GPU->IsA<Render::CD3D11GPUDriver>()) FAIL;

	const char* pSubId;
	IO::PStream Stream = OpenStream(UID, pSubId);
	if (!Stream) return nullptr;

	IO::CBinaryReader R(*Stream);

	Data::CFourCC FileSig;
	if (!R.Read(FileSig)) FAIL;

	switch (ShaderType)
	{
		//???separate type and target? type =, target >=
		case Render::ShaderType_Vertex:		if (FileSig != 'VS40' && FileSig != 'VS41' && FileSig != 'VS50') FAIL; break;
		case Render::ShaderType_Pixel:		if (FileSig != 'PS40' && FileSig != 'PS41' && FileSig != 'PS50') FAIL; break;
		case Render::ShaderType_Geometry:	if (FileSig != 'GS40' && FileSig != 'GS41' && FileSig != 'GS50') FAIL; break;
		case Render::ShaderType_Hull:		if (FileSig != 'HS50') FAIL; break;
		case Render::ShaderType_Domain:		if (FileSig != 'DS50') FAIL; break;
		default:
		{
			// Shader type autodetection
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
				default:		FAIL;
			};
			break;
		}
	}

	U32 BinaryOffset;
	if (!R.Read(BinaryOffset)) FAIL;

	U32 ShaderFileID;
	if (!R.Read(ShaderFileID)) FAIL;

	U32 InputSignatureID;
	if (!R.Read(InputSignatureID)) FAIL;

	U64 MetadataOffset = Stream->GetPosition();
	U64 FileSize = Stream->GetSize();
	UPTR BinarySize = (UPTR)FileSize - (UPTR)BinaryOffset;
	if (!BinarySize) FAIL;
	void* pData = n_malloc(BinarySize);
	if (!pData) FAIL;
	if (!Stream->Seek(BinaryOffset, IO::Seek_Begin) || Stream->Read(pData, BinarySize) != BinarySize)
	{
		n_free(pData);
		FAIL;
	}

	void* pSigData = NULL;
	if (ShaderType == Render::ShaderType_Vertex) // || ShaderType == Render::ShaderType_Geometry)
	{
		if (!InputSignatureID) InputSignatureID = ShaderFileID;

		if (!D3D11DrvFactory->FindShaderInputSignature(InputSignatureID))
		{
			UPTR SigSize;
			if (InputSignatureID == ShaderFileID)
			{
				pSigData = pData;
				SigSize = BinarySize;
			}
			else if (!ShaderLibrary->GetRawDataByID(InputSignatureID, pSigData, SigSize))
			{
				n_free(pData);
				FAIL;
			}

			if (!D3D11DrvFactory->RegisterShaderInputSignature(InputSignatureID, pSigData, SigSize))
			{
				n_free(pData);
				n_free(pSigData);
				FAIL;
			}
		}
	}

	Render::PD3D11Shader Shader = (Render::CD3D11Shader*)GPU->CreateShader(ShaderType, pData, BinarySize).Get();
	if (Shader.IsNullPtr()) FAIL;

	if (pSigData != pData) n_free(pData);

	Shader->InputSignatureID = InputSignatureID;

	if (!Stream->Seek(MetadataOffset, IO::Seek_Begin)) FAIL;

	if (!Shader->Metadata.Load(*Stream)) FAIL;

	return Shader.Get();
}
//---------------------------------------------------------------------

}