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
enum D3D_FEATURE_LEVEL;

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

	// Don't change values
	enum EConstType
	{
		D3D11Const_Bool	= 0,
		D3D11Const_Int,
		D3D11Const_Float,

		D3D11Const_Struct,

		D3D11Const_Invalid
	};

	// Don't change values
	enum ERsrcType
	{
		SM40Rsrc_Texture1D			= 0,
		SM40Rsrc_Texture1DArray,
		SM40Rsrc_Texture2D,
		SM40Rsrc_Texture2DArray,
		SM40Rsrc_Texture2DMS,
		SM40Rsrc_Texture2DMSArray,
		SM40Rsrc_Texture3D,
		SM40Rsrc_TextureCUBE,
		SM40Rsrc_TextureCUBEArray,

		SM40Rsrc_Unknown
	};

	struct CBufferMeta
	{
		CStrID		Name;
		EBufferType	Type;
		U32			Register;
		U32			Size;		// Element size for structured buffers
		HHandle		Handle;
	};

	struct CConstMeta
	{
		HHandle		BufferHandle;
		CStrID		Name;
		EConstType	Type;
		U32			Offset;
		U32			ElementSize;
		U32			ElementCount;
		HHandle		Handle;
	};

	struct CRsrcMeta
	{
		CStrID		Name;
		ERsrcType	Type;
		U32			RegisterStart;
		U32			RegisterCount;
		HHandle		Handle;
	};

	struct CSamplerMeta
	{
		CStrID		Name;
		U32			RegisterStart;
		U32			RegisterCount;
		HHandle		Handle;
	};

	//!!!must be sorted by name! //???sort in tool?
	CFixedArray<CConstMeta>		Consts;
	CFixedArray<CBufferMeta>	Buffers;
	CFixedArray<CRsrcMeta>		Resources;
	CFixedArray<CSamplerMeta>	Samplers;

	UPTR						InputSignatureID;
	D3D_FEATURE_LEVEL			MinFeatureLevel;
	U64							RequiresFlags;

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
	virtual HConstBuffer	GetConstBufferHandle(HConst hConst) const;
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
