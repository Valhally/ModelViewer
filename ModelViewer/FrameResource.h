#pragma once

#include "DirectX-std.h"
#include "UploadBuffer.h"
#include "MathHelper.h"
#include "DirectXHelp.h"

using namespace DirectX;

struct PassConstants
{
    XMFLOAT4X4 model;
    XMFLOAT4X4 view;
    XMFLOAT4X4 projection;

    XMFLOAT3 albedo;
    float metallic;
    float roughness;
    float ao;

    int lightCount;
	float unused;

    XMFLOAT4 lightPos[8];

    XMFLOAT4 lightColor[8];

    XMFLOAT3 camearaPos;
};

class FrameResource
{
private:
	std::shared_ptr<UploadBuffer<PassConstants>> passCB = nullptr;
	ComPtr<ID3D12CommandAllocator> cmdAlloc;

public:
	UINT64 FenceValue = 0;

	FrameResource(ComPtr<ID3D12Device4> device)
	{
		THROW_IF_FAILED(device->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT, 
			IID_PPV_ARGS(&cmdAlloc)
		));
		passCB = std::make_shared<UploadBuffer<PassConstants>>(device, 1, true);
	}

	inline ComPtr<ID3D12CommandAllocator> GetCmdAlloc() { return cmdAlloc; }
	inline std::shared_ptr<UploadBuffer<PassConstants>> GetPassConstants() { return passCB; }
};