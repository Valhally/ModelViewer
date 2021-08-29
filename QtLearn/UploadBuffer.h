#pragma once

#include "DirectX-std.h"

template<class T>
class UploadBuffer
{
private:
	ComPtr<ID3D12Resource> uploadBuffer;
	byte* mappedData;
	UINT elementByteSize;
	bool isConstantBuffer;

public:
	UploadBuffer(ComPtr<ID3D12Device4> device, UINT elementCount, bool isConstantBuffer) : isConstantBuffer(isConstantBuffer)
	{
		elementByteSize = sizeof(T);
		if(isConstantBuffer) elementByteSize = CalcConstantBufferByteSize(elementByteSize);
		THROW_IF_FAILED(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(elementByteSize * elementCount),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&uploadBuffer)
		));

		THROW_IF_FAILED(uploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedData)));
	}
	UploadBuffer(const UploadBuffer& rhs) = delete;
	UploadBuffer& operator=(const UploadBuffer& rhs) = delete;
	~UploadBuffer()
	{
		if(mappedData != nullptr) uploadBuffer->Unmap(0, nullptr);
		mappedData = nullptr;
	}

	ID3D12Resource* GetResource() const
	{
		return uploadBuffer.Get();
	}

	void CopyData(UINT elementIndex, const T& data)
	{
		memcpy(mappedData + elementByteSize * elementIndex, &data, sizeof(T));
	}

	inline UINT GetElementSize() { return elementByteSize; }

	static UINT CalcConstantBufferByteSize(UINT size)
	{
		return (size + 255) & ~255;
	}
}; 