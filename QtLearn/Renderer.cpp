#include "Renderer.h"

Renderer::Renderer(HWND handle, int width, int height) : windowHandle(handle)
{
	CreateFactory();
	EnumAdapters();
	CreateDevice();

	for(int i = 0; i < FrameBackBufferCount; ++i) 
		frameResources[i] = std::make_shared<FrameResource>(device);

	CreateCommandObjects();
	CreateSwapChain(width, height);
	CreateFence();
	CreateVertexBuffer();
	CreateShaders();
	CreateRootSignature();
	CreatePso();

	commandList->Close();
	ID3D12CommandList* cmdLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
	FlushCommandQueue();

	camera = std::make_shared<Camera>(AspectRatio());
	lastResize = -1;
}

void Renderer::CreateFactory() {
	int factoryFlags = 0;
#ifdef _DEBUG
	{
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
			debugController->EnableDebugLayer();
			factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;

		}
	}
#endif
	THROW_IF_FAILED(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&factory)));
}

void Renderer::EnumAdapters() {
	for (UINT i = 0; factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
		DXGI_ADAPTER_DESC1 adapterDesc = {};
		adapter->GetDesc1(&adapterDesc);

		if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
		if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), FeatureLevel, _uuidof(ID3D12Device), nullptr))) break;
	}
}

void Renderer::CreateDevice() {
	THROW_IF_FAILED(D3D12CreateDevice(adapter.Get(), FeatureLevel, IID_PPV_ARGS(&device)));
}

void Renderer::CreateSwapChain(int width, int height) {
	this->width = width; this->height = height;

	// CreateSwapChain
	{
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.BufferCount = FrameBackBufferCount;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.Width = width;
		swapChainDesc.Height = height;
		swapChainDesc.Format = BackBufferFormat;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.SampleDesc.Count = 1;

		ComPtr<IDXGISwapChain1> tmpSwapChain;
		THROW_IF_FAILED(factory->CreateSwapChainForHwnd(commandQueue.Get(), windowHandle, &swapChainDesc, nullptr, nullptr, &tmpSwapChain));
		THROW_IF_FAILED(tmpSwapChain.As(&swapChain));
	}

	// CreateRtvDescriptorHeap
	{
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		rtvHeapDesc.NumDescriptors = FrameBackBufferCount + 1;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

		THROW_IF_FAILED(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap)));

		rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}
	//CreateDsvDescriptorHeap
	{
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
		dsvHeapDesc.NodeMask = 0;
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		THROW_IF_FAILED(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap)));
	}
	

	// CreateCbvHeap
	{
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc{
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			FrameBackBufferCount,
			D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
		};
		THROW_IF_FAILED(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&cbvHeap)));
		cbvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		CD3DX12_CPU_DESCRIPTOR_HANDLE handle(cbvHeap->GetCPUDescriptorHandleForHeapStart());
		for(int i = 0; i < FrameBackBufferCount; ++i)
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{
				frameResources[i]->GetPassConstants()->GetResource()->GetGPUVirtualAddress(),
				frameResources[i]->GetPassConstants()->GetElementSize()
			};
			device->CreateConstantBufferView(&cbvDesc, handle);
			handle.Offset(cbvDescriptorSize);
		}
	}

	CreateSizeResource();
}

void Renderer::CreateCommandObjects() {
	D3D12_COMMAND_QUEUE_DESC queueDesc{};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	THROW_IF_FAILED(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue)));

	THROW_IF_FAILED(device->CreateCommandAllocator(queueDesc.Type, IID_PPV_ARGS(&commandAlloc)));

	THROW_IF_FAILED(device->CreateCommandList(0, queueDesc.Type, commandAlloc.Get(), nullptr, IID_PPV_ARGS(&commandList)));
}

void Renderer::CreateFence() {
	THROW_IF_FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
	fenceValue = 1;

	fenceEvent = CreateEvent(nullptr, false, false, nullptr);
	if (fenceEvent == nullptr) THROW_IF_FAILED(HRESULT_FROM_WIN32(GetLastError()));
}

void Renderer::CreateShaders() {
	UINT compileFlags = 0;
#ifdef _DEBUG
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif // _DEBUG
	ComPtr<ID3D10Blob> error;
	auto result = D3DCompileFromFile(
		L"shaders/pbr.hlsl", 
		nullptr, 
		nullptr, 
		"VSMain", 
		"vs_5_0", 
		compileFlags, 
		0, 
		&vertexShader, 
		&error );
	if(!SUCCEEDED(result))
	{
		std::string info;
		for(int i = 0; i < error->GetBufferSize(); ++i) info.push_back( ((char*)(error->GetBufferPointer()))[i] );
		std::cout << info << std::endl;
		THROW_IF_FAILED(result);
	}

	result = D3DCompileFromFile(
		L"shaders/pbr.hlsl", 
		nullptr, 
		nullptr, 
		"PSMain", 
		"ps_5_0", 
		compileFlags, 
		0, 
		&fragmentShader, 
		&error );
	if(!SUCCEEDED(result))
	{
		std::string info;
		for(int i = 0; i < error->GetBufferSize(); ++i) info.push_back( ((char*)(error->GetBufferPointer()))[i] );
		std::cout << info << std::endl;
		THROW_IF_FAILED(result);
	}
}

void Renderer::CreateVertexBuffer() {
	inputElementDescs.push_back(
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,
			0, offsetof(Vertex, position), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	);
	inputElementDescs.push_back(
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,
			0, offsetof(Vertex, normal), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	);
	inputElementDescs.push_back(
		{"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT,
			0, offsetof(Vertex, color), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	);

	model[0] = std::make_shared<Model>(initModel, device, commandList);
	curModel = 0;
	switchFrame = -1;
	task.Clear();

	GenGrid();
}

void Renderer::GenGrid()
{
	for(int i = -gridCount / 2 * gridLength, iLimit = gridCount / 2 * gridLength; i <= iLimit; i += gridLength)
		for(int j = -gridCount / 2 * gridLength, jLimit = gridCount / 2 * gridLength; j <= jLimit; j += gridLength)
		{
			Vertex vertex{{i * 1.f, 0, j * 1.f}, gridColor};
			gridVertices.push_back(vertex);
		}
			
	const float inf = 70000;
	gridVertices.push_back({{-inf, 0, 0}, xaxisColor});
	gridVertices.push_back({{ inf, 0, 0}, xaxisColor});
	gridVertices.push_back({{0, -inf, 0}, yaxisColor});
	gridVertices.push_back({{0,  inf, 0}, yaxisColor});
	gridVertices.push_back({{0, 0, -inf}, zaxisColor});
	gridVertices.push_back({{0, 0,  inf}, zaxisColor});

	for(int i = 0, offset = (gridCount + 1) * (gridCount + 1); i < 6; ++i)
		gridIndices.push_back(offset + i);

	for(int i = 0; i < gridCount; ++i)
		for(int j = 0; j < gridCount; ++j)
		{
			if(i != gridCount / 2)
				gridIndices.push_back(i * (gridCount + 1) + j),
				gridIndices.push_back(i * (gridCount + 1) + j + 1);

			if(j != gridCount / 2) 
				gridIndices.push_back(i * (gridCount + 1) + j),
				gridIndices.push_back((i + 1) * (gridCount + 1) + j);
		}

	grid = std::make_shared<Model>(
		std::make_shared<VertexBuffer>(device, commandList, gridVertices.data(), sizeof(Vertex), gridVertices.size()),
		std::make_shared<IndexBuffer>(device, commandList, gridIndices.data(), 2, DXGI_FORMAT_R32_UINT),
		device,
		commandList );
	grid->lineIndexBuffers.push_back(std::make_shared<IndexBuffer>(
		device, commandList, gridIndices.data() + 2, 2, DXGI_FORMAT_R32_UINT
		));
	grid->lineIndexBuffers.push_back(std::make_shared<IndexBuffer>(
		device, commandList, gridIndices.data() + 4, 2, DXGI_FORMAT_R32_UINT
		));
	grid->lineIndexBuffers.push_back(std::make_shared<IndexBuffer>(
		device, commandList, gridIndices.data() + 6, gridIndices.size() - 6, DXGI_FORMAT_R32_UINT
		));
}


void Renderer::CreateRootSignature() {
	CD3DX12_DESCRIPTOR_RANGE table;
	table.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);

	CD3DX12_ROOT_PARAMETER parameter;
	parameter.InitAsDescriptorTable(1, &table);

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(1, &parameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);


	ComPtr<ID3D10Blob> signature;
	ComPtr<ID3D10Blob> error;
	THROW_IF_FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
	THROW_IF_FAILED(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
}

void Renderer::CreatePso() {
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.InputLayout = { inputElementDescs.data(), (UINT)inputElementDescs.size() };
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(fragmentShader.Get());
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.MultisampleEnable = true;
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = BackBufferFormat;
	psoDesc.DSVFormat = DepthStencilFormat;
	psoDesc.SampleDesc.Count = MultiSampleNum;

	THROW_IF_FAILED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));

	ComPtr<ID3D10Blob> gridVertexShader;
	ComPtr<ID3D10Blob> gridFragmentShader;

	UINT compileFlags = 0;
#ifdef _DEBUG
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif // _DEBUG
	ComPtr<ID3D10Blob> error;
	auto result = D3DCompileFromFile(
		L"shaders/grid.hlsl", 
		nullptr, 
		nullptr, 
		"VSMain", 
		"vs_5_0", 
		compileFlags, 
		0, 
		&gridVertexShader, 
		&error );
	if(!SUCCEEDED(result))
	{
		std::string info;
		for(int i = 0; i < error->GetBufferSize(); ++i) info.push_back( ((char*)(error->GetBufferPointer()))[i] );
		std::cout << info << std::endl;
		THROW_IF_FAILED(result);
	}

	result = D3DCompileFromFile(
		L"shaders/grid.hlsl", 
		nullptr, 
		nullptr, 
		"PSMain", 
		"ps_5_0", 
		compileFlags, 
		0, 
		&gridFragmentShader, 
		&error );
	if(!SUCCEEDED(result))
	{
		std::string info;
		for(int i = 0; i < error->GetBufferSize(); ++i) info.push_back( ((char*)(error->GetBufferPointer()))[i] );
		std::cout << info << std::endl;
		THROW_IF_FAILED(result);
	}

	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(gridVertexShader.Get());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(gridFragmentShader.Get());
	THROW_IF_FAILED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&gridPso)));
}

void Renderer::FlushCommandQueue(UINT64 waitValue) {
	if(waitValue == 0)
	{
		waitValue = ++fenceValue;
		commandQueue->Signal(fence.Get(), waitValue);
	}
	if (fence->GetCompletedValue() < waitValue) {
		THROW_IF_FAILED(fence->SetEventOnCompletion(waitValue, fenceEvent));
		WaitForSingleObject(fenceEvent, INFINITE);
	}
}

inline ComPtr<ID3D12Resource> Renderer::CurRenderTarget()  { return renderTargets[curFrameIndex]; }

std::shared_ptr<FrameResource> Renderer::CurFrameResource() { return frameResources[curFrameIndex]; }

ComPtr<ID3D12Resource> Renderer::DepthStencil() { return depthStencil; }

D3D12_CPU_DESCRIPTOR_HANDLE Renderer::CurRenderTargetView()
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvHeap->GetCPUDescriptorHandleForHeapStart(), curFrameIndex, rtvDescriptorSize);
}

D3D12_CPU_DESCRIPTOR_HANDLE Renderer::DepthStencilView()
{
	return dsvHeap->GetCPUDescriptorHandleForHeapStart();
}

double Renderer::AspectRatio()
{
	return static_cast<double>(width) / height;
}

std::shared_ptr<Camera> Renderer::GetCamera()
{
	return camera;
}

void Renderer::SwitchModel()
{
	QString QfileName = QFileDialog::getOpenFileName(
	nullptr,
	"Select a model",
	nullptr,
	Model::GetReadFileTypeList().c_str()
	);
	std::string fileName = QfileName.toStdString();

	std::cout << fileName << std::endl << fileName.size() << std::endl;
	if(fileName.size() > 1) task.SetValue(fileName);
}

void Renderer::SaveModel()
{
	QString QfileName = QFileDialog::getSaveFileName(
		nullptr,
		"Save your model",
		nullptr,
		"Obj Model(*.obj)"
	);
	
	model[curModel]->saveModel(std::string(QfileName.toLocal8Bit()));
}

void Renderer::SwitchUp()
{
	axisFlag = XMFLOAT3{1, 0, 1};
	camera->origin = XMVECTOR{
		model[curModel]->lr[0].Middle(),
		model[curModel]->lr[1].Middle(),
		model[curModel]->lr[2].Middle()
	} * model[curModel]->scale;

	camera->phi = -0.5 * PI + 0.0000001;
	camera->theta = PI * 0.5;
	camera->radius = (model[curModel]->lr[1].max - model[curModel]->lr[1].Middle()) * 5 * model[curModel]->scale;
}

void Renderer::SwitchDown()
{
	axisFlag = XMFLOAT3{1, 0, 1};
	camera->origin = XMVECTOR{
		model[curModel]->lr[0].Middle(),
		model[curModel]->lr[1].Middle(),
		model[curModel]->lr[2].Middle()
	} * model[curModel]->scale;
	//printf("%f  %f  %f\n", model[curModel]->lr[0].Middle(), model[curModel]->lr[1].Middle(), model[curModel]->lr[2].Middle());

	camera->phi = 0.5 * PI - 0.0000001;
	camera->theta = PI * 0.5;
	camera->radius = (model[curModel]->lr[1].max - model[curModel]->lr[1].Middle()) * 5 * model[curModel]->scale;
}

void Renderer::SwitchLeft()
{
	axisFlag = XMFLOAT3{0, 1, 1};
	camera->origin = XMVECTOR{
		model[curModel]->lr[0].Middle(),
		model[curModel]->lr[1].Middle(),
		model[curModel]->lr[2].Middle()
	} * model[curModel]->scale;
	//printf("%f  %f  %f\n", model[curModel]->lr[0].Middle(), model[curModel]->lr[1].Middle(), model[curModel]->lr[2].Middle());

	camera->phi = 0;
	camera->theta = 0;
	camera->radius = (model[curModel]->lr[1].max - model[curModel]->lr[1].Middle()) * 5 * model[curModel]->scale;
}

void Renderer::SwitchRight()
{
	axisFlag = XMFLOAT3{0, 1, 1};
	camera->origin = XMVECTOR{
		model[curModel]->lr[0].Middle(),
		model[curModel]->lr[1].Middle(),
		model[curModel]->lr[2].Middle()
	} * model[curModel]->scale;
	//printf("%f  %f  %f\n", model[curModel]->lr[0].Middle(), model[curModel]->lr[1].Middle(), model[curModel]->lr[2].Middle());

	camera->phi = 0;
	camera->theta = PI;
	camera->radius = (model[curModel]->lr[1].max - model[curModel]->lr[1].Middle()) * 5 * model[curModel]->scale;
}

void Renderer::SwitchFront()
{
	axisFlag = XMFLOAT3{1, 1, 0};
	camera->origin = XMVECTOR{
		model[curModel]->lr[0].Middle(),
		model[curModel]->lr[1].Middle(),
		model[curModel]->lr[2].Middle()
	} * model[curModel]->scale;
	//printf("%f  %f  %f\n", model[curModel]->lr[0].Middle(), model[curModel]->lr[1].Middle(), model[curModel]->lr[2].Middle());

	camera->phi = 0;
	camera->theta = 0.5 * PI;
	camera->radius = (model[curModel]->lr[1].max - model[curModel]->lr[1].Middle()) * 5 * model[curModel]->scale;
}

void Renderer::SwitchBack()
{
	axisFlag = XMFLOAT3{1, 1, 0};
	camera->origin = XMVECTOR{
		model[curModel]->lr[0].Middle(),
		model[curModel]->lr[1].Middle(),
		model[curModel]->lr[2].Middle()
	} * model[curModel]->scale;
	//printf("%f  %f  %f\n", model[curModel]->lr[0].Middle(), model[curModel]->lr[1].Middle(), model[curModel]->lr[2].Middle());

	camera->phi = 0;
	camera->theta = -0.5 * PI;
	camera->radius = (model[curModel]->lr[1].max - model[curModel]->lr[1].Middle()) * 5 * model[curModel]->scale;
}

void Renderer::ResizeSwapChain()
{
	FlushCommandQueue();
	for(int i = 0; i < FrameBackBufferCount; ++i) renderTargets[i].Reset();
	depthStencil.Reset();
	offsetScreenRenderTarget.Reset();
	swapChain->ResizeBuffers(FrameBackBufferCount, newSize.x, newSize.y, BackBufferFormat, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
	width = newSize.x; height = newSize.y;
	CreateSizeResource();
	camera->aspectRatio = static_cast<float>(width) / height;
}

void Renderer::CreateSizeResource()
{
	viewport = CD3DX12_VIEWPORT(0.f, 0.f, static_cast<float>(width), static_cast<float>(height));
	scissor = CD3DX12_RECT(0, 0, width, height);

	// CreateRtvDescriptors
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE handle(rtvHeap->GetCPUDescriptorHandleForHeapStart());
		for (UINT i = 0; i < FrameBackBufferCount; ++i) {
			THROW_IF_FAILED(swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i])));
			device->CreateRenderTargetView(renderTargets[i].Get(), nullptr, handle);
			handle.Offset(1, rtvDescriptorSize);
		}
	}

	// Create offset screen resource
	{
		D3D12_RESOURCE_DESC msaaRTDesc = CD3DX12_RESOURCE_DESC::Tex2D(
			BackBufferFormat,
			width,
			height,
			1,
			1,
			MultiSampleNum
		);
		msaaRTDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		D3D12_CLEAR_VALUE msaaClearValue = {};
		msaaClearValue.Format = BackBufferFormat;
		msaaClearValue.Color[0] = screenClearColor.x;
		msaaClearValue.Color[1] = screenClearColor.y;
		msaaClearValue.Color[2] = screenClearColor.z;
		msaaClearValue.Color[3] = 1.0f;

		if(offsetScreenRenderTarget.Get()) offsetScreenRenderTarget->Release();

		THROW_IF_FAILED(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&msaaRTDesc,
			D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
			&msaaClearValue,
			IID_PPV_ARGS(&offsetScreenRenderTarget)
		));

		CD3DX12_CPU_DESCRIPTOR_HANDLE handle(rtvHeap->GetCPUDescriptorHandleForHeapStart(), FrameBackBufferCount, rtvDescriptorSize);
		device->CreateRenderTargetView(offsetScreenRenderTarget.Get(), nullptr, handle);

	}

	// Create DepthStencil Resource

	D3D12_RESOURCE_DESC depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DepthStencilFormat,
		width, height,
		1, 1, MultiSampleNum
	);
	depthDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE depthClearValue;
	depthClearValue.Format = DepthStencilFormat;
	depthClearValue.DepthStencil.Depth = 1.0f;
	depthClearValue.DepthStencil.Stencil = 0;

	if(depthStencil.Get()) depthStencil->Release();

	THROW_IF_FAILED(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&depthDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthClearValue,
		IID_PPV_ARGS(&depthStencil)
	));

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DepthStencilFormat;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
	device->CreateDepthStencilView(DepthStencil().Get(), &dsvDesc, dsvHeap->GetCPUDescriptorHandleForHeapStart());
}

void Renderer::Update()
{
	if(task.HasValue())
	{
		model[curModel ^ 1] = std::make_shared<Model>(task.GetValue(), device, commandList);
		task.Clear();
		switchFrame =curFrameIndex;
		std::cout << "Task:  " << task.GetValue() << std::endl;
	}
	else if(switchFrame == curFrameIndex)
	{
		curModel ^= 1;
		switchFrame = -1;
		std::cout << "It's time to switch Model!" << std::endl;
	}

	XMFLOAT4 lightPos[] = {
		{1, 1, -1, 1},
		{-1, 1, 1, 1},
		{-1, 1,-1, 1},
		{1, 1, 1, 1},
		{1, -1, -1, 1},
		{-1, -1, 1, 1},
		{-1, -1,-1, 1},
		{1, -1, 1, 1}

	};
	XMFLOAT4 lightColor[] = {
		{1.0, 1.0, 1.0, 1},
		{1.0, 1.0, 1.0, 1},
		{1.0, 1.0, 1.0, 1},
		{1.0, 1.0, 1.0, 1},
		{1.0, 1.0, 1.0, 1},
		{1.0, 1.0, 1.0, 1},
		{1.0, 1.0, 1.0, 1},
		{1.0, 1.0, 1.0, 1}
	};

	float tmp = 800;
	float lightInten = lightIntensity;

	for(int i = 0; i < _countof(lightPos); ++i)
	{
		lightPos[i].x *= tmp;
		lightPos[i].y *= tmp;
		lightPos[i].z *= tmp;
	}
	for(int i = 0; i < _countof(lightColor); ++i)
	{
		lightColor[i].x *= lightInten;
		lightColor[i].y *= lightInten;
		lightColor[i].z *= lightInten;
	}

	PassConstants passCB;
	XMStoreFloat4x4(&passCB.model, XMMatrixTranspose(model[curModel]->getModel()));
	XMStoreFloat4x4(&passCB.view, XMMatrixTranspose(camera->getViewMatrix()));
	XMStoreFloat4x4(&passCB.projection, XMMatrixTranspose(camera->getProjectMatrix()));
	passCB.albedo = {0.8, 0.8, 0.8};
	passCB.metallic = 0.0;
	passCB.roughness = 0.6;
	passCB.ao = 1.0;

	passCB.lightCount = _countof(lightPos);
	for(int i = 0; i < _countof(lightPos); ++i)
		passCB.lightPos[i] = lightPos[i],
		passCB.lightColor[i] = lightColor[i];

	XMStoreFloat3(&passCB.camearaPos, camera->getCameraPos());

	CurFrameResource()->GetPassConstants()->CopyData(0, passCB);
}


void Renderer::Draw() {
	//if(flag) return;
	if(clock() - lastMove < controlTime * CLOCKS_PER_SEC) return;
	if(lastResize > 0 && clock() - lastResize > controlTime * CLOCKS_PER_SEC)
	{
		ResizeSwapChain();
		lastResize = -1;
	}

	if(infoLabel)
	{
		char buffer[128];
		sprintf(buffer, " 模型:%s | 点:%d | 三角面:%d  ", model[curModel]->modelFileName.c_str(), model[curModel]->vertexBuffer->vertexCount, model[curModel]->faceCount);
		infoLabel->setText(QString::fromLocal8Bit(buffer, strlen(buffer)));
	}

	// PopulateCommandList
	{
		curFrameIndex = swapChain->GetCurrentBackBufferIndex();
		FlushCommandQueue(CurFrameResource()->FenceValue);
		THROW_IF_FAILED(commandList->Reset(CurFrameResource()->GetCmdAlloc().Get(), pso.Get()));

		Update();

		//Set command list state
		commandList->SetGraphicsRootSignature(rootSignature.Get());
		commandList->RSSetViewports(1, &viewport);
		commandList->RSSetScissorRects(1, &scissor);
		
		ID3D12DescriptorHeap* heaps[] = { cbvHeap.Get() };
		commandList->SetDescriptorHeaps(_countof(heaps), heaps);

		CD3DX12_GPU_DESCRIPTOR_HANDLE handle(cbvHeap->GetGPUDescriptorHandleForHeapStart(), curFrameIndex, cbvDescriptorSize);
		commandList->SetGraphicsRootDescriptorTable(0, handle);

		//Transition resource for render
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
			offsetScreenRenderTarget.Get(),
			D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET
		));

		//Clear and set render target
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart(), FrameBackBufferCount, rtvDescriptorSize);
		static const float clearColor[4] = { screenClearColor.x, screenClearColor.y, screenClearColor.z, 1.0f };
		if(curFrameIndex != 0)
		commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
		commandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
		commandList->OMSetRenderTargets(1, &rtvHandle, true, &DepthStencilView());

		//Draw

		if(curFrameIndex != 0)
		{
			model[curModel]->Draw(primitiveType);
			commandList->SetPipelineState(gridPso.Get());
			grid->DrawGrid(axisFlag);
		}
		

		//Transiton for present
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
			offsetScreenRenderTarget.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_RESOLVE_SOURCE ));

		commandList->ResourceBarrier(1,
			&CD3DX12_RESOURCE_BARRIER::Transition(
				CurRenderTarget().Get(),
				D3D12_RESOURCE_STATE_PRESENT,
				D3D12_RESOURCE_STATE_RESOLVE_DEST ));

		commandList->ResolveSubresource(
			CurRenderTarget().Get(), 
			0, 
			offsetScreenRenderTarget.Get(), 
			0, 
			BackBufferFormat
		);
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
			CurRenderTarget().Get(),
			D3D12_RESOURCE_STATE_RESOLVE_DEST,
			D3D12_RESOURCE_STATE_PRESENT
		));

		//Close command list
		commandList->Close();
	}


	ID3D12CommandList* cmdLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	swapChain->Present(1, 0);

	CurFrameResource()->FenceValue = ++fenceValue;
	commandQueue->Signal(fence.Get(), fenceValue);
}

void Renderer::SwitchPoint()
{
	primitiveType = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
}
void Renderer::SwitchLine()
{
	primitiveType = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
}
void Renderer::SwitchFace()
{
	primitiveType = D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
}
void Renderer::SwitchSolid()
{
	primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
}
void Renderer::Rotate(double xoffset, double yoffset)
{
	axisFlag = XMFLOAT3{1, 1, 1};
	camera->Rotate(xoffset, yoffset);
}
void Renderer::Transform(double xoffset, double yoffset)
{
	axisFlag = XMFLOAT3{1, 1, 1};
	camera->Transform(xoffset, yoffset);
}

void Renderer::ChangeLightIntensity(int intensity)
{
	lightIntensity = intensity;
}


