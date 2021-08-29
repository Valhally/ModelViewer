#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <tchar.h>
#include <wrl.h>
#include <memory>
#include <vector>

#include <dxgi1_6.h>
#include <d3d12.h>
#include <DirectXMath.h>
#include <d3d12Shader.h>
#include <d3dcompiler.h>

#ifdef _DEBUG
#include <dxgidebug.h>
#endif

#include "d3dx12.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;

class COMException
{
public:
	COMException(HRESULT hr) : m_hrError(hr) {}
	HRESULT Error() const { return m_hrError; }
private:
	const HRESULT m_hrError;
};

inline void THROW_IF_FAILED(HRESULT hr) {
	if (FAILED(hr)) throw COMException(hr);
}

struct Vertex
{
	XMFLOAT3 position;
	XMFLOAT3 normal;
	XMFLOAT3 color;
};