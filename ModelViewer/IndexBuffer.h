#pragma once

#include "VertexBuffer.h"

class IndexBuffer
{
public:
	ComPtr<ID3D12Resource> buffer;
	ComPtr<ID3D12Resource> uploadBuffer;
	D3D12_INDEX_BUFFER_VIEW descriptor;
	UINT indexCount;
	UINT startLocation;
	DXGI_FORMAT indexFormat;
	std::shared_ptr<VertexBuffer> vertexBuffer;

public:
	IndexBuffer(
		ComPtr<ID3D12Device4> device,
		ComPtr<ID3D12GraphicsCommandList> cmdList,
		void* inidData,
		UINT indexCount, DXGI_FORMAT indexFormat,
		UINT startLocation = 0
	) : indexCount(indexCount), indexFormat(indexFormat), startLocation(startLocation)
	{
		buffer = DirectXHelp::CreateDefaultBuffer(
			device,
			cmdList,
			inidData,
			(indexFormat == DXGI_FORMAT_R16_UINT ? 2 : 4) * indexCount,
			uploadBuffer
		);

		descriptor.BufferLocation = buffer->GetGPUVirtualAddress();
		descriptor.Format = indexFormat;
		descriptor.SizeInBytes = (indexFormat == DXGI_FORMAT_R16_UINT ? 2 : 4) * indexCount;
	} 

	void SetVertexBuffer(std::shared_ptr<VertexBuffer> buffer)
	{
		vertexBuffer = buffer;
	}

	void Bind(ComPtr<ID3D12GraphicsCommandList> cmdList)
	{
		cmdList->IASetIndexBuffer(&descriptor);
	}
	void Draw(ComPtr<ID3D12GraphicsCommandList> cmdList)
	{
		Bind(cmdList);
		cmdList->DrawIndexedInstanced(indexCount, 1, 0, startLocation, 0);
	}
	UINT GetStartLocation() { return startLocation; }
};