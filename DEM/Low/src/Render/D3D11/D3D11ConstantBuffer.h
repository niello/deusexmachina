#pragma once
#include <Render/ConstantBuffer.h>
#include <Data/Flags.h>

// A Direct3D11 implementation of a shader constant buffer.
// Buffer can be created as a VRAM-only or with a RAM copy. Buffers with
// a RAM copy support partial updates in all cases. VRAM-only buffers
// save memory, but non-mappable ones don't support BeginChanges() +
// Set...() + EndChanges() and, as of DX11, can't be updated partially.
// Use WriteCommitToVRAM() to update non-mappable VRAM-only buffers.

struct ID3D11Buffer;
struct ID3D11ShaderResourceView;
struct ID3D11DeviceContext;
enum D3D11_USAGE;

namespace Render
{
enum EUSMBufferType;

class CD3D11ConstantBuffer: public CConstantBuffer
{
	__DeclareClassNoFactory;

protected:

	enum
	{
		CB11_UsesRAMCopy	= 0x01,
		CB11_Dirty			= 0x02,
		CB11_InWriteMode	= 0x04,
		CB11_Temporary		= 0x08
	};

	ID3D11Buffer*				pBuffer = nullptr;
	ID3D11ShaderResourceView*	pSRView = nullptr;
	char*						pMapped = nullptr;
	EUSMBufferType				Type;
	D3D11_USAGE					D3DUsage;
	UPTR						SizeInBytes;
	Data::CFlags				Flags;

public:

	CD3D11ConstantBuffer(ID3D11Buffer* pCB, ID3D11ShaderResourceView* pSRV, bool Temporary = false);
	virtual ~CD3D11ConstantBuffer() override;

	virtual bool				IsValid() const override { return !!pBuffer; }
	virtual bool				IsInWriteMode() const override { return Flags.Is(CB11_InWriteMode); }
	virtual bool                IsDirty() const override { return Flags.Is(CB11_Dirty); }
	virtual bool				IsTemporary() const override { return Flags.Is(CB11_Temporary); }
	virtual U8                  GetAccessFlags() const override;

	bool						CreateRAMCopy();
	void						ResetRAMCopy(const void* pVRAMData);
	void						DestroyRAMCopy();

	void						WriteData(UPTR Offset, const void* pData, UPTR Size);

	ID3D11Buffer*				GetD3DBuffer() const { return pBuffer; }
	ID3D11ShaderResourceView*	GetD3DSRView() const { return pSRView; }
	char*						GetMappedVRAM() { return Flags.Is(CB11_UsesRAMCopy) ? nullptr : pMapped; }
	char*						GetRAMCopy() { return Flags.Is(CB11_UsesRAMCopy) ? pMapped : nullptr; }
	const char*					GetRAMCopy() const { return Flags.Is(CB11_UsesRAMCopy) ? pMapped : nullptr; }
	D3D11_USAGE					GetD3DUsage() const { return D3DUsage; }
	UPTR						GetSizeInBytes() const { return SizeInBytes; }
	EUSMBufferType				GetType() const { return Type; }
	bool						UsesRAMCopy() const { return Flags.Is(CB11_UsesRAMCopy); }

	void						OnBegin(void* pMappedVRAM = nullptr);	// For internal use by the GPUDriver
	void						OnCommit();							// For internal use by the GPUDriver
};

typedef Ptr<CD3D11ConstantBuffer> PD3D11ConstantBuffer;

inline void CD3D11ConstantBuffer::WriteData(UPTR Offset, const void* pData, UPTR Size)
{
	n_assert_dbg(Flags.Is(CB11_InWriteMode) && pData && Size && pMapped && (Offset + Size <= SizeInBytes));
	memcpy(pMapped + Offset, pData, Size);
	Flags.Set(CB11_Dirty);
}
//---------------------------------------------------------------------

}
