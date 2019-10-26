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
	__DeclareClass(CD3D9Shader);

protected:

	union
	{
		void*					pD3DShader = nullptr;
		IDirect3DVertexShader9*	pD3DVertexShader;
		IDirect3DPixelShader9*	pD3DPixelShader;
	};

	void							InternalDestroy();

	friend class CD3D9GPUDriver; // for creation

public:

	virtual ~CD3D9Shader();

	bool							Create(IDirect3DVertexShader9* pShader);
	bool							Create(IDirect3DPixelShader9* pShader);
	virtual void					Destroy() { InternalDestroy(); }

	virtual bool					IsValid() const override { return !!pD3DShader; }

	IDirect3DVertexShader9*			GetD3DVertexShader() const { n_assert_dbg(Type == ShaderType_Vertex); return pD3DVertexShader; }
	IDirect3DPixelShader9*			GetD3DPixelShader() const { n_assert_dbg(Type == ShaderType_Pixel); return pD3DPixelShader; }
};

typedef Ptr<CD3D9Shader> PD3D9Shader;

}
