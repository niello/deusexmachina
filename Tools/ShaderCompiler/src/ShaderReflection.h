#pragma once

// Shader reflection enums

// Don't change existing values, they are saved to file
enum EShaderModel
{
	ShaderModel_30	= 0,
	ShaderModel_USM	= 1
};

enum EShaderConstType
{
	ShaderConst_Bool	= 0,
	ShaderConst_Int,
	ShaderConst_Float,

	ShaderConst_Struct,

	ShaderConst_Invalid
};

// Hardware capability level relative to D3D features.
// This is a hardware attribute, it doesn't depend on API used.
// Never change values, they are used in a file format.
enum EGPUFeatureLevel
{
	GPU_Level_D3D9_1	= 0x9100,
	GPU_Level_D3D9_2	= 0x9200,
	GPU_Level_D3D9_3	= 0x9300,   // OpenGL 3.2 (?)
	GPU_Level_D3D10_0	= 0xa000,   // OpenGL 3.3
	GPU_Level_D3D10_1	= 0xa100,
	GPU_Level_D3D11_0	= 0xb000,   // OpenGL 4.0 (compute shaders from 4.3?)
	GPU_Level_D3D11_1	= 0xb100,
	GPU_Level_D3D12_0	= 0xc000,   // Vulkan
	GPU_Level_D3D12_1	= 0xc100
};
