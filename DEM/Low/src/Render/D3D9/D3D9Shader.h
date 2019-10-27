#pragma once
#include <Render/Shader.h>

// Direct3D9 shader object implementation

struct IUnknown;
struct IDirect3DVertexShader9;
struct IDirect3DPixelShader9;

namespace Render
{
class CD3D9GPUDriver;

class CD3D9Shader: public CShader
{
	__DeclareClassNoFactory;

protected:

	IUnknown* _pShader = nullptr;

public:

	CD3D9Shader(IDirect3DVertexShader9* pShader, PShaderParamTable Params = nullptr);
	CD3D9Shader(IDirect3DPixelShader9* pShader, PShaderParamTable Params = nullptr);
	virtual ~CD3D9Shader();

	virtual bool            IsValid() const override { return !!_pShader && _Type != ShaderType_Invalid; }

	IDirect3DVertexShader9* GetD3DVertexShader() const;
	IDirect3DPixelShader9*  GetD3DPixelShader() const;
};

typedef Ptr<CD3D9Shader> PD3D9Shader;

}
