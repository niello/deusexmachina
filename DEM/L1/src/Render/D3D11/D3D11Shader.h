#pragma once
#ifndef __DEM_L1_RENDER_D3D11_SHADER_H__
#define __DEM_L1_RENDER_D3D11_SHADER_H__

#include <Render/Shader.h>
#include <Render/D3D11/USMShaderMetadata.h>

// Direct3D11 shader object implementation

struct ID3D11DeviceChild;
struct ID3D11VertexShader;
struct ID3D11HullShader;
struct ID3D11DomainShader;
struct ID3D11GeometryShader;
struct ID3D11PixelShader;

namespace Resources
{
	class CD3D11ShaderLoader;
}

namespace Render
{

class CD3D11Shader: public CShader
{
	__DeclareClass(CD3D11Shader);

protected:

	ID3D11DeviceChild*	pD3DShader;
	CUSMShaderMetadata	Metadata;

	void							InternalDestroy();

	friend class Resources::CD3D11ShaderLoader;

public:

	UPTR				InputSignatureID;

	CD3D11Shader(): pD3DShader(NULL), InputSignatureID(0) {}
	virtual ~CD3D11Shader() { InternalDestroy(); }

	bool							Create(ID3D11DeviceChild* pShader); 
	bool							Create(ID3D11VertexShader* pShader);
	bool							Create(ID3D11HullShader* pShader);
	bool							Create(ID3D11DomainShader* pShader);
	bool							Create(ID3D11GeometryShader* pShader);
	bool							Create(ID3D11PixelShader* pShader);
	virtual void					Destroy() { InternalDestroy(); }

	virtual const IShaderMetadata*	GetMetadata() const { return &Metadata; }
	virtual bool					IsResourceValid() const { return !!pD3DShader; }

	ID3D11DeviceChild*				GetD3DShader() const { return pD3DShader; }
	ID3D11VertexShader*				GetD3DVertexShader() const { n_assert_dbg(Type == ShaderType_Vertex); return (ID3D11VertexShader*)pD3DShader; }
	ID3D11HullShader*				GetD3DHullShader() const { n_assert_dbg(Type == ShaderType_Hull); return (ID3D11HullShader*)pD3DShader; }
	ID3D11DomainShader*				GetD3DDomainShader() const { n_assert_dbg(Type == ShaderType_Domain); return (ID3D11DomainShader*)pD3DShader; }
	ID3D11GeometryShader*			GetD3DGeometryShader() const { n_assert_dbg(Type == ShaderType_Geometry); return (ID3D11GeometryShader*)pD3DShader; }
	ID3D11PixelShader*				GetD3DPixelShader() const { n_assert_dbg(Type == ShaderType_Pixel); return (ID3D11PixelShader*)pD3DShader; }
};

typedef Ptr<CD3D11Shader> PD3D11Shader;

}

#endif
