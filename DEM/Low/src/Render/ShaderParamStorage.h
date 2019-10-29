#pragma once
#include <Render/RenderFwd.h>

// Shader parameter storage stores values in a form ready to be committed to the GPU.
// In a combination with shader parameter table this class simplifies the process of
// setting shader parameters from the code.
// Indices are stable and can be used as handles for fast parameter setting.
// Constant buffers are managed automatically. User can set the buffer explicitly
// or a temporary one will be created at the first time it is required.

//???!!!offset indices / add type bits to avoid using an index for parameter of another class?!

namespace Render
{

class CShaderParamStorage final
{
protected:

	PShaderParamTable            _Table;
	PGPUDriver                   _GPU;

	//???!!!for the fast access when setting separate constants?!
	//???std::vector<CConstantBuffer*> _ConstantBufferPerConstant;

	std::vector<PConstantBuffer> _ConstantBuffers;
	std::vector<PTexture>        _Resources;
	std::vector<PSampler>        _Samplers;

public:

	CShaderParamStorage(CShaderParamTable& Table, CGPUDriver& GPU);
	CShaderParamStorage(CShaderParamStorage&& Other);
	~CShaderParamStorage();

	bool                     SetConstantBuffer(CStrID ID, CConstantBuffer* pBuffer);
	bool                     SetConstantBuffer(size_t Index, CConstantBuffer* pBuffer);
	CConstantBuffer*         GetBuffer(size_t Index, bool Create = true);

	bool                     SetRawConstant(CStrID ID, void* pData, UPTR Size);
	bool                     SetRawConstant(size_t Index, void* pData, UPTR Size);

	bool                     SetResource(CStrID ID, CTexture* pTexture);
	bool                     SetResource(size_t Index, CTexture* pTexture);

	bool                     SetSampler(CStrID ID, CSampler* pSampler);
	bool                     SetSampler(size_t Index, CSampler* pSampler);

	bool                     Apply() const;

	const CShaderParamTable& GetParamTable() const { return *_Table; }
};

}
