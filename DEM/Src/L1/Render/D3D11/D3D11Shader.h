#pragma once
#ifndef __DEM_L1_RENDER_D3D11_SHADER_H__
#define __DEM_L1_RENDER_D3D11_SHADER_H__

#include <Render/Shader.h>
#include <Data/FixedArray.h>

// Direct3D11 shader object implementation

struct ID3D11DeviceChild;
struct ID3D11VertexShader;
struct ID3D11HullShader;
struct ID3D11DomainShader;
struct ID3D11GeometryShader;
struct ID3D11PixelShader;

namespace Render
{

class CD3D11Shader: public CShader
{
	__DeclareClass(CD3D11Shader);

protected:

	ID3D11DeviceChild*		pD3DShader;

	void					InternalDestroy();

public:

	enum EBufferType
	{
		ConstantBuffer		= 0,
		TextureBuffer		= 1,
		StructuredBuffer	= 2
	};

	struct CConstMeta
	{
		CStrID	Name;
		HHandle	Handle;
		HHandle	BufferHandle;
		U32		Offset;
		U32		Size;	//!!!arrays  vectors may need ElementCount & ElementSize!
	};

	struct CBufferMeta
	{
		CStrID		Name;
		HHandle		Handle;
		U32			Register;
		U32			ElementSize;
		U32			ElementCount;	// 1 for CB
		EBufferType	Type;
	};

	struct CRsrcMeta
	{
		CStrID	Name;
		HHandle	Handle;
		U32		Register;
	};

	//!!!must be sorted by name! //???sort in tool?
	CFixedArray<CConstMeta>		Consts;
	CFixedArray<CBufferMeta>	Buffers;
	CFixedArray<CRsrcMeta>		Resources;
	CFixedArray<CRsrcMeta>		Samplers;

	UPTR						InputSignatureID;

	CD3D11Shader(): pD3DShader(NULL), InputSignatureID(0) {}
	virtual ~CD3D11Shader() { InternalDestroy(); }

	bool					Create(ID3D11DeviceChild* pShader); 
	bool					Create(ID3D11VertexShader* pShader);
	bool					Create(ID3D11HullShader* pShader);
	bool					Create(ID3D11DomainShader* pShader);
	bool					Create(ID3D11GeometryShader* pShader);
	bool					Create(ID3D11PixelShader* pShader);
	virtual void			Destroy() { InternalDestroy(); }

	virtual bool			IsResourceValid() const { return !!pD3DShader; }

	virtual HConst			GetConstHandle(CStrID ID) const;
	virtual HConstBuffer	GetConstBufferHandle(CStrID ID) const;
	virtual HResource		GetResourceHandle(CStrID ID) const;
	virtual HSampler		GetSamplerHandle(CStrID ID) const;

	ID3D11DeviceChild*		GetD3DShader() const { return pD3DShader; }
	ID3D11VertexShader*		GetD3DVertexShader() const { n_assert_dbg(Type == ShaderType_Vertex); return (ID3D11VertexShader*)pD3DShader; }
	ID3D11HullShader*		GetD3DHullShader() const { n_assert_dbg(Type == ShaderType_Hull); return (ID3D11HullShader*)pD3DShader; }
	ID3D11DomainShader*		GetD3DDomainShader() const { n_assert_dbg(Type == ShaderType_Domain); return (ID3D11DomainShader*)pD3DShader; }
	ID3D11GeometryShader*	GetD3DGeometryShader() const { n_assert_dbg(Type == ShaderType_Geometry); return (ID3D11GeometryShader*)pD3DShader; }
	ID3D11PixelShader*		GetD3DPixelShader() const { n_assert_dbg(Type == ShaderType_Pixel); return (ID3D11PixelShader*)pD3DShader; }
};

typedef Ptr<CD3D11Shader> PD3D11Shader;

}

#endif
