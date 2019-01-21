#pragma once
#include <Render/Shader.h>
#include <Render/D3D11/USMShaderMetadata.h>

// Direct3D11 shader object implementation

struct ID3D11DeviceChild;
struct ID3D11VertexShader;
struct ID3D11HullShader;
struct ID3D11DomainShader;
struct ID3D11GeometryShader;
struct ID3D11PixelShader;

namespace Render
{
class CD3D11GPUDriver;

class CD3D11Shader: public CShader
{
	__DeclareClass(CD3D11Shader);

protected:

	ID3D11DeviceChild*	pD3DShader = nullptr;
	CUSMShaderMetadata	Metadata;
	U32					InputSignatureID = 0;

	void							InternalDestroy();

	friend class CD3D11GPUDriver; // for creation

public:

	virtual ~CD3D11Shader() { InternalDestroy(); }

	bool							Create(ID3D11DeviceChild* pShader); 
	bool							Create(ID3D11VertexShader* pShader);
	bool							Create(ID3D11HullShader* pShader);
	bool							Create(ID3D11DomainShader* pShader);
	bool							Create(ID3D11GeometryShader* pShader);
	bool							Create(ID3D11PixelShader* pShader);
	virtual void					Destroy() { InternalDestroy(); }

	virtual bool					IsValid() const override { return !!pD3DShader; }
	virtual const IShaderMetadata*	GetMetadata() const override { return &Metadata; }
	U32								GetInputSignatureID() const { return InputSignatureID; }

	ID3D11DeviceChild*				GetD3DShader() const { return pD3DShader; }
	ID3D11VertexShader*				GetD3DVertexShader() const;
	ID3D11HullShader*				GetD3DHullShader() const;
	ID3D11DomainShader*				GetD3DDomainShader() const;
	ID3D11GeometryShader*			GetD3DGeometryShader() const;
	ID3D11PixelShader*				GetD3DPixelShader() const;
};

typedef Ptr<CD3D11Shader> PD3D11Shader;

}
