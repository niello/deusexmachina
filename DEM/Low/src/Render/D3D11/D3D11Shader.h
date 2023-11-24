#pragma once
#include <Render/Shader.h>

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
	RTTI_CLASS_DECL(Render::CD3D11Shader, Render::CShader);

protected:

	ID3D11DeviceChild*	_pShader = nullptr;
	U32					_InputSignatureID = 0;

public:

	CD3D11Shader(ID3D11VertexShader* pShader, U32 InputSignatureID, PShaderParamTable Params = nullptr);
	CD3D11Shader(ID3D11PixelShader* pShader, PShaderParamTable Params = nullptr);
	CD3D11Shader(ID3D11GeometryShader* pShader, U32 InputSignatureID, PShaderParamTable Params = nullptr);
	CD3D11Shader(ID3D11HullShader* pShader, PShaderParamTable Params = nullptr);
	CD3D11Shader(ID3D11DomainShader* pShader, PShaderParamTable Params = nullptr);

	virtual ~CD3D11Shader() override;

	virtual bool          IsValid() const override { return !!_pShader && _Type != ShaderType_Invalid; }
	virtual void          SetDebugName(std::string_view Name) override;

	U32                   GetInputSignatureID() const { return _InputSignatureID; }
	ID3D11DeviceChild*    GetD3DShader() const { return _pShader; }
	ID3D11VertexShader*   GetD3DVertexShader() const;
	ID3D11HullShader*     GetD3DHullShader() const;
	ID3D11DomainShader*   GetD3DDomainShader() const;
	ID3D11GeometryShader* GetD3DGeometryShader() const;
	ID3D11PixelShader*    GetD3DPixelShader() const;
};

typedef Ptr<CD3D11Shader> PD3D11Shader;

}
