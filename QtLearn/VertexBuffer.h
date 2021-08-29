#pragma once

#include "DirectX-std.h"
#include "DirectXHelp.h"

class VertexBuffer
{
public:
	ComPtr<ID3D12Resource> buffer;
	ComPtr<ID3D12Resource> uploadBuffer;
	D3D12_VERTEX_BUFFER_VIEW descriptor;
	UINT vertexSize;
	UINT vertexCount;

public:
	VertexBuffer(
		ComPtr<ID3D12Device4> device,
		ComPtr<ID3D12GraphicsCommandList> cmdList,
		void* initData,
		UINT vertexSize, UINT vertexCount
	) : vertexSize(vertexSize), vertexCount(vertexCount)
	{
		buffer = DirectXHelp::CreateDefaultBuffer(device, cmdList, initData, vertexCount * vertexSize, uploadBuffer);
		descriptor.BufferLocation = buffer->GetGPUVirtualAddress();
		descriptor.SizeInBytes = vertexSize * vertexCount;
		descriptor.StrideInBytes = vertexSize;
	}

	void Bind(ComPtr<ID3D12GraphicsCommandList> cmdList)
	{
		cmdList->IASetVertexBuffers(0, 1, &descriptor);
	}

	void Draw(ComPtr<ID3D12GraphicsCommandList> cmdList)
	{
		Bind(cmdList);
		cmdList->DrawInstanced(vertexCount, 1, 0, 0);
	}
};