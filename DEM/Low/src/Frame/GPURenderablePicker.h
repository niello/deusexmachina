#pragma once
#include <Render/RenderFwd.h>
#include <Data/StringID.h>
#include <Data/Regions.h>
#include <map>

// Renders geometry batches, instanced when possible. Uses sorting, lights.
// Batches are designed to minimize shader state switches.

namespace Frame
{
using PView = std::unique_ptr<class CView>;

class CGPURenderablePicker
{
protected:

	Render::PRenderTarget                 _RT;
	Render::PDepthStencilBuffer           _DS;
	std::map<Render::EEffectType, CStrID> _GPUPickEffects;

public:

	struct CPickInfo
	{
		Render::IRenderable* pRenderable = nullptr;
		UPTR                 UserValue = INVALID_INDEX;
		vector3              Position;
		vector3              Normal;
		vector2              TexCoord;
	};

	class CPickRequest
	{
	protected:

		Render::PTexture  CPUReadableTexture;
		Render::PGPUFence CopyFence;

	public:

		friend class CGPURenderablePicker;

		const CView* pView = nullptr;
		Data::CRectF RelRect;
		std::vector<std::pair<Render::IRenderable*, UPTR>> Objects; // Renderable + UserValue

		bool IsValid() const;
		bool IsReady() const;
		void Wait() const;
		void Get(CPickInfo& Out);
	};

	CGPURenderablePicker(Render::CGPUDriver& GPU, std::map<Render::EEffectType, CStrID>&& GPUPickEffects);
	~CGPURenderablePicker();

	bool        Init(); //???!!!to the constructor?!
	bool        PickAsync(CPickRequest& AsyncRequest);

	const auto& GetEffects() const { return _GPUPickEffects; }
};

}
