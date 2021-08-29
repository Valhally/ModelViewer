#pragma once

#include "DirectX-std.h"
#include "FrameResource.h"
#include "IndexBuffer.h"
#include "Model.h"
#include "Camera.h"
#include "Nullable.h"
#include <QFileDialog>
#include <ctime>
#include "QLabel"

#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3dcompiler.lib")

class Renderer {
private:
	static const int FrameBackBufferCount = 3;
	static const int MultiSampleNum = 4;
	static const auto BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	static const auto DepthStencilFormat = DXGI_FORMAT_D32_FLOAT;
	static const auto FeatureLevel = D3D_FEATURE_LEVEL_11_0;

	static constexpr int gridLength = 100;
	static const int gridCount = 1000;

	static constexpr XMFLOAT3 screenClearColor{55.0 / 255, 56.0 / 255, 59.0/ 255};

	static constexpr XMFLOAT3 gridColor {62.0 / 255, 63.0 / 255, 66.0 / 255};
	static constexpr XMFLOAT3 gridBigColor{0.4, 0.4, 0.4};
	static constexpr XMFLOAT3 xaxisColor{232.0 / 255, 95.0 / 255, 55.0 / 255};
	static constexpr XMFLOAT3 yaxisColor{118.0 / 255, 248.0 / 255, 39.0 / 255};
	static constexpr XMFLOAT3 zaxisColor{37.0 / 255, 191.0 / 255, 250.0 / 255};

	std::string initModel = std::string("models/demo.fbx");

	std::shared_ptr<FrameResource> frameResources[FrameBackBufferCount];

	HWND windowHandle;
	int width;
	int height;
	int curFrameIndex;

	UINT64 fenceValue;
	HANDLE fenceEvent;

	CD3DX12_VIEWPORT viewport;
	CD3DX12_RECT scissor;

	ComPtr<IDXGIFactory5> factory;
	ComPtr<IDXGIAdapter1> adapter;
	ComPtr<ID3D12Device4> device;

	ComPtr<IDXGISwapChain3> swapChain;

	ComPtr<ID3D12CommandQueue> commandQueue;
	ComPtr<ID3D12CommandAllocator> commandAlloc;
	ComPtr<ID3D12GraphicsCommandList> commandList;

	ComPtr<ID3D12DescriptorHeap> rtvHeap;
	UINT rtvDescriptorSize;
	ComPtr<ID3D12Resource> renderTargets[FrameBackBufferCount];
	ComPtr<ID3D12DescriptorHeap> dsvHeap;
	ComPtr<ID3D12Resource> depthStencil;

	ComPtr<ID3D12DescriptorHeap> cbvHeap;
	UINT cbvDescriptorSize;

	ComPtr<ID3D12RootSignature> rootSignature;
	ComPtr<ID3D12PipelineState> pso;

	ComPtr<ID3D12Fence> fence;

	std::shared_ptr<VertexBuffer> vertexBuffer;
	std::shared_ptr<IndexBuffer> indexBuffer;

	ComPtr<ID3D10Blob> vertexShader;
	ComPtr<ID3D10Blob> fragmentShader;

	std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescs;

	ComPtr<ID3D12Resource> offsetScreenRenderTarget;

	std::shared_ptr<Model> model[2];
	UINT curModel;

	std::shared_ptr<Model> grid;
	std::vector<Vertex> gridVertices;
	std::vector<UINT32> gridIndices;
	ComPtr<ID3D12PipelineState> gridPso;

	Nullable<std::string> task;
	int switchFrame;

	std::shared_ptr<Camera> camera;
	
	static constexpr double controlTime = 0.15;

	

public:
	Renderer(HWND handle, int width, int height);
	void Draw();
	double AspectRatio();
	std::shared_ptr<Camera> GetCamera();

	bool flag = false;
	double lastMove = 0;
	double lastResize = -1;
	int lightIntensity = 2500000;
	XMINT2 newSize;
	XMFLOAT3 axisFlag{1, 1, 1};
	D3D12_PRIMITIVE_TOPOLOGY primitiveType = D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	void Rotate(double xoffset, double yoffset);
	void Transform(double xoffset, double yoffset);

	QLabel* infoLabel;

	void SwitchModel();
	void SaveModel();
	void SwitchPoint();
	void SwitchLine();
	void SwitchFace();
	void SwitchSolid();
	void SwitchUp();
	void SwitchDown();
	void SwitchLeft();
	void SwitchRight();
	void SwitchFront();
	void SwitchBack();

	void ChangeLightIntensity(int intensity);

private:
	inline ComPtr<ID3D12Resource> CurRenderTarget();
	inline std::shared_ptr<FrameResource> CurFrameResource();
	inline ComPtr<ID3D12Resource> DepthStencil();
	inline D3D12_CPU_DESCRIPTOR_HANDLE CurRenderTargetView();
	inline D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView();
	void CreateFactory();
	void EnumAdapters();
	void CreateDevice();
	void CreateSwapChain(int width, int height);
	void ResizeSwapChain();
	void CreateSizeResource();
	void CreateCommandObjects();
	void CreateFence();
	void CreateVertexBuffer();
	void CreateShaders();
	void CreateRootSignature();
	void CreatePso();
	void FlushCommandQueue(UINT64 waitValue = 0);
	void Update();
	void GenGrid();
};