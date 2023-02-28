#include "Game.h"
#include "Vertex.h"
#include "Input.h"
#include "Helpers.h"
#include "Mesh.h"
#include "BufferStructs.h"
#include "RaytracingHelper.h"

// Needed for a helper function to load pre-compiled shader files
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>
#include "DX12Helper.h"

// For the DirectX Math library
using namespace DirectX;

// --------------------------------------------------------
// Constructor
//
// DXCore (base class) constructor will set up underlying fields.
// Direct3D itself, and our window, are not ready at this point!
//
// hInstance - the application's OS-level handle (unique ID)
// --------------------------------------------------------
Game::Game(HINSTANCE hInstance)
	: DXCore(
		hInstance,			// The application's handle
		L"DirectX Game",	// Text for the window's title bar (as a wide-character string)
		1280,				// Width of the window's client area
		720,				// Height of the window's client area
		false,				// Sync the framerate to the monitor refresh? (lock framerate)
		true)				// Show extra stats (fps) in title bar?
{
#if defined(DEBUG) || defined(_DEBUG)
	// Do we want a console window?  Probably only in debug mode
	CreateConsoleWindow(500, 120, 32, 120);
	printf("Console window created successfully.  Feel free to printf() here.\n");
#endif

	vbView = {};
	ibView = {};
}

// --------------------------------------------------------
// Destructor - Clean up anything our game has created:
//  - Delete all objects manually created within this class
//  - Release() all Direct3D objects created within this class
// --------------------------------------------------------
Game::~Game()
{
	// Call delete or delete[] on any objects or arrays you've
	// created using new or new[] within this class
	// - Note: this is unnecessary if using smart pointers

	// Call Release() on any Direct3D objects made within this class
	// - Note: this is unnecessary for D3D objects stored in ComPtrs

	DX12Helper::GetInstance().WaitForGPU();
	delete& RaytracingHelper::GetInstance();
}

// --------------------------------------------------------
// Called once per program, after Direct3D and the window
// are initialized but before the game loop.
// --------------------------------------------------------
void Game::Init()
{
	RaytracingHelper::GetInstance().Initialize(
		windowWidth,
		windowHeight,
		device,
		commandQueue,
		commandList,
		FixPath(L"Raytracing.cso"));

	// Helper methods for loading shaders, creating some basic
	// geometry to draw and some simple camera matrices.
	//  - You'll be expanding and/or replacing these later
	CreateRootSigAndPipelineState();
	LoadTextures();
	CreateGeometry();

	camera = std::make_shared<Camera>(
		XMFLOAT3(0.0f, 0.0f, -5.0f),		// Position
		5.0f,								// Move speed
		0.002f,								// Look speed
		XM_PIDIV4,							// Field of view
		windowWidth / (float)windowHeight);	// Aspect ratio

	lights = std::vector<Light>();
	Light dirLight = {};
	dirLight.Type = LIGHT_TYPE_DIRECTIONAL;
	dirLight.Direction = XMFLOAT3(1.0f, -1.0f, 1.0f);
	dirLight.Color = XMFLOAT3(1.0f, 1.0f, 1.0f);
	dirLight.Intensity = 1.0f;
	lights.push_back(dirLight);
}

// --------------------------------------------------------
// Loads the two basic shaders, then creates the root signature
// and pipeline state object for our very basic demo.
// --------------------------------------------------------
void Game::CreateRootSigAndPipelineState()
{
	// Blobs to hold raw shader byte code
	Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderByteCode;
	Microsoft::WRL::ComPtr<ID3DBlob> pixelShaderByteCode;

	// Load shaders
	{
		// Readed compiled vertex shader code into a blob
		D3DReadFileToBlob(FixPath(L"VertexShader.cso").c_str(), vertexShaderByteCode.GetAddressOf());
		D3DReadFileToBlob(FixPath(L"PixelShader.cso").c_str(), pixelShaderByteCode.GetAddressOf());
	}

	// Input layout
	const unsigned int inputElementCount = 4;
	D3D12_INPUT_ELEMENT_DESC inputElements[inputElementCount] = {};
	{
		inputElements[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		inputElements[0].Format = DXGI_FORMAT_R32G32B32_FLOAT; // R32 G32 B32 = float3
		inputElements[0].SemanticName = "POSITION"; // Name must match semantic in shader
		inputElements[0].SemanticIndex = 0; // This is the first POSITION semantic

		inputElements[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		inputElements[1].Format = DXGI_FORMAT_R32G32_FLOAT; // R32 G32 = float2
		inputElements[1].SemanticName = "TEXCOORD";
		inputElements[1].SemanticIndex = 0; // This is the first TEXCOORD semantic

		inputElements[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		inputElements[2].Format = DXGI_FORMAT_R32G32B32_FLOAT; // R32 G32 B32 = float3
		inputElements[2].SemanticName = "NORMAL";
		inputElements[2].SemanticIndex = 0; // This is the first NORMAL semantic

		inputElements[3].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		inputElements[3].Format = DXGI_FORMAT_R32G32B32_FLOAT; // R32 G32 B32 = float3
		inputElements[3].SemanticName = "TANGENT";
		inputElements[3].SemanticIndex = 0; // This is the first TANGENT semantic
	}

	// Root Signature
	{
		// Describe the range of CBVs needed for the vertex shader
		D3D12_DESCRIPTOR_RANGE cbvRangeVS = {};
		cbvRangeVS.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		cbvRangeVS.NumDescriptors = 1;
		cbvRangeVS.BaseShaderRegister = 0;
		cbvRangeVS.RegisterSpace = 0;
		cbvRangeVS.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		// Describe the range of CBVs needed for the pixel shader
		D3D12_DESCRIPTOR_RANGE cbvRangePS = {};
		cbvRangePS.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		cbvRangePS.NumDescriptors = 1;
		cbvRangePS.BaseShaderRegister = 0;
		cbvRangePS.RegisterSpace = 0;
		cbvRangePS.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		// Create a range of SRV's for textures
		D3D12_DESCRIPTOR_RANGE srvRange = {};
		srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		srvRange.NumDescriptors = 4; // Set to max number of textures at once (match pixel shader)
		srvRange.BaseShaderRegister = 0; // Starts at s0 (match pixel shader)
		srvRange.RegisterSpace = 0;
		srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		// Create the root parameters
		D3D12_ROOT_PARAMETER rootParams[3] = {};

		// CBV table param for vertex shader
		rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		rootParams[0].DescriptorTable.NumDescriptorRanges = 1;
		rootParams[0].DescriptorTable.pDescriptorRanges = &cbvRangeVS;

		// CBV table param for pixel shader
		rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
		rootParams[1].DescriptorTable.pDescriptorRanges = &cbvRangePS;

		// SRV table param
		rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParams[2].DescriptorTable.NumDescriptorRanges = 1;
		rootParams[2].DescriptorTable.pDescriptorRanges = &srvRange;

		// Create a single static sampler (available to all pixel shaders at the same slot)
		D3D12_STATIC_SAMPLER_DESC anisoWrap = {};
		anisoWrap.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		anisoWrap.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		anisoWrap.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		anisoWrap.Filter = D3D12_FILTER_ANISOTROPIC;
		anisoWrap.MaxAnisotropy = 16;
		anisoWrap.MaxLOD = D3D12_FLOAT32_MAX;
		anisoWrap.ShaderRegister = 0; // register(s0)
		anisoWrap.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		D3D12_STATIC_SAMPLER_DESC samplers[] = { anisoWrap };

		// Describe and serialize the root signature
		D3D12_ROOT_SIGNATURE_DESC rootSig = {};
		rootSig.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		rootSig.NumParameters = ARRAYSIZE(rootParams);
		rootSig.pParameters = rootParams;
		rootSig.NumStaticSamplers = ARRAYSIZE(samplers);
		rootSig.pStaticSamplers = samplers;

		ID3DBlob* serializedRootSig = 0;
		ID3DBlob* errors = 0;
		D3D12SerializeRootSignature(
			&rootSig,
			D3D_ROOT_SIGNATURE_VERSION_1,
			&serializedRootSig,
			&errors);

		// Check for errors during serialization
		if (errors != 0)
		{
			OutputDebugString((wchar_t*)errors->GetBufferPointer());
		}

		// Create the root signature
		device->CreateRootSignature(
			0,
			serializedRootSig->GetBufferPointer(),
			serializedRootSig->GetBufferSize(),
			IID_PPV_ARGS(rootSignature.GetAddressOf()));
	}

	// Pipeline state
	{
		// Describe the pipeline state
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		
		// Input assembler related
		psoDesc.InputLayout.NumElements = inputElementCount;
		psoDesc.InputLayout.pInputElementDescs = inputElements;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		psoDesc.pRootSignature = rootSignature.Get();

		// Vertex and Pixel shader related
		psoDesc.VS.pShaderBytecode = vertexShaderByteCode->GetBufferPointer();
		psoDesc.VS.BytecodeLength = vertexShaderByteCode->GetBufferSize();
		psoDesc.PS.pShaderBytecode = pixelShaderByteCode->GetBufferPointer();
		psoDesc.PS.BytecodeLength = pixelShaderByteCode->GetBufferSize();

		// Render targets
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		psoDesc.SampleDesc.Count = 1;
		psoDesc.SampleDesc.Quality = 0;

		// States
		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
		psoDesc.RasterizerState.DepthClipEnable = true;
		psoDesc.DepthStencilState.DepthEnable = true;
		psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
		psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
		psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		psoDesc.SampleMask = 0xffffffff;

		// Create the pipe state object
		device->CreateGraphicsPipelineState(
			&psoDesc,
			IID_PPV_ARGS(pipelineState.GetAddressOf()));
	}
}

void Game::LoadTextures()
{
	DX12Helper& dx12Helper = DX12Helper::GetInstance();

	bronzeMat = std::make_shared<Material>(pipelineState, XMFLOAT3(1, 1, 1), XMFLOAT2(1, 1), XMFLOAT2(0, 0));
	D3D12_CPU_DESCRIPTOR_HANDLE bronzeAlbedo = dx12Helper.LoadTexture(FixPath(L"../../Assets/Textures/bronze_albedo.png").c_str());
	D3D12_CPU_DESCRIPTOR_HANDLE bronzeNormals = dx12Helper.LoadTexture(FixPath(L"../../Assets/Textures/bronze_normals.png").c_str());
	D3D12_CPU_DESCRIPTOR_HANDLE bronzeRoughness = dx12Helper.LoadTexture(FixPath(L"../../Assets/Textures/bronze_roughness.png").c_str());
	D3D12_CPU_DESCRIPTOR_HANDLE bronzeMetal = dx12Helper.LoadTexture(FixPath(L"../../Assets/Textures/bronze_metal.png").c_str());

	bronzeMat->AddTexture(bronzeAlbedo, 0);
	bronzeMat->AddTexture(bronzeNormals, 1);
	bronzeMat->AddTexture(bronzeRoughness, 2);
	bronzeMat->AddTexture(bronzeMetal, 3);

	bronzeMat->FinalizeMaterial();

	cobbleMat = std::make_shared<Material>(pipelineState, XMFLOAT3(1, 1, 1), XMFLOAT2(1, 1), XMFLOAT2(0, 0));
	D3D12_CPU_DESCRIPTOR_HANDLE cobbleAlbedo = dx12Helper.LoadTexture(FixPath(L"../../Assets/Textures/cobblestone_albedo.png").c_str());
	D3D12_CPU_DESCRIPTOR_HANDLE cobbleNormals = dx12Helper.LoadTexture(FixPath(L"../../Assets/Textures/cobblestone_normals.png").c_str());
	D3D12_CPU_DESCRIPTOR_HANDLE cobbleRoughness = dx12Helper.LoadTexture(FixPath(L"../../Assets/Textures/cobblestone_roughness.png").c_str());
	D3D12_CPU_DESCRIPTOR_HANDLE cobbleMetal = dx12Helper.LoadTexture(FixPath(L"../../Assets/Textures/cobblestone_metal.png").c_str());

	cobbleMat->AddTexture(cobbleAlbedo, 0);
	cobbleMat->AddTexture(cobbleNormals, 1);
	cobbleMat->AddTexture(cobbleRoughness, 2);
	cobbleMat->AddTexture(cobbleMetal, 3);

	cobbleMat->FinalizeMaterial();
}

// --------------------------------------------------------
// Creates the geometry we're going to draw - a single triangle for now
// --------------------------------------------------------
void Game::CreateGeometry()
{
	std::shared_ptr<Mesh> cube = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/cube.obj").c_str());
	std::shared_ptr<Mesh> cylinder = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/cylinder.obj").c_str());
	std::shared_ptr<Mesh> helix = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/helix.obj").c_str());
	std::shared_ptr<Mesh> quad = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/quad.obj").c_str());
	std::shared_ptr<Mesh> quadDoubleSided = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/quad_double_sided.obj").c_str());
	std::shared_ptr<Mesh> sphere = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/sphere.obj").c_str());
	std::shared_ptr<Mesh> torus = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/torus.obj").c_str());

	std::shared_ptr<GameEntity> cubeEntity = std::make_shared<GameEntity>(cube, bronzeMat);
	std::shared_ptr<GameEntity> cylinderEntity = std::make_shared<GameEntity>(cylinder, cobbleMat);
	std::shared_ptr<GameEntity> helixEntity = std::make_shared<GameEntity>(helix, bronzeMat);
	std::shared_ptr<GameEntity> quadEntity = std::make_shared<GameEntity>(quad, cobbleMat);
	std::shared_ptr<GameEntity> quadDoubleSidedEntity = std::make_shared<GameEntity>(quadDoubleSided, bronzeMat);
	std::shared_ptr<GameEntity> sphereEntity = std::make_shared<GameEntity>(sphere, cobbleMat);
	std::shared_ptr<GameEntity> torusEntity = std::make_shared<GameEntity>(torus, bronzeMat);

	cubeEntity->GetTransform()->SetPosition(-9, 0, 0);
	cylinderEntity->GetTransform()->SetPosition(-6, 0, 0);
	helixEntity->GetTransform()->SetPosition(-3, 0, 0);
	quadEntity->GetTransform()->SetPosition(0, 0, 0);
	quadDoubleSidedEntity->GetTransform()->SetPosition(3, 0, 0);
	sphereEntity->GetTransform()->SetPosition(6, 0, 0);
	torusEntity->GetTransform()->SetPosition(9, 0, 0);

	entities.push_back(cubeEntity);
	entities.push_back(cylinderEntity);
	entities.push_back(helixEntity);
	entities.push_back(quadEntity);
	entities.push_back(quadDoubleSidedEntity);
	entities.push_back(sphereEntity);
	entities.push_back(torusEntity);

	// Create TLAS
	RaytracingHelper::GetInstance().CreateTopLevelAccelerationStructureForScene(entities);
}


// --------------------------------------------------------
// Handle resizing to match the new window size.
//  - DXCore needs to resize the back buffer
//  - Eventually, we'll want to update our 3D camera
// --------------------------------------------------------
void Game::OnResize()
{
	camera->UpdateProjectionMatrix((float)windowWidth / windowHeight);
	RaytracingHelper::GetInstance().ResizeOutputUAV(windowWidth, windowHeight);

	// Handle base-level DX resize stuff
	DXCore::OnResize();
}

// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{
	camera->Update(deltaTime);

	for (auto& e : entities) {
		e->GetTransform()->Rotate(0, deltaTime, 0);
		// e->GetTransform()->MoveAbsolute(0, sin(totalTime) * 0.1f, 0);
	}

	// Example input checking: Quit if the escape key is pressed
	if (Input::GetInstance().KeyDown(VK_ESCAPE))
		Quit();
}

// --------------------------------------------------------
// Clear the screen, redraw everything, present to the user
// --------------------------------------------------------
void Game::Draw(float deltaTime, float totalTime)
{
	// Grab the current back buffer for this frame
	Microsoft::WRL::ComPtr<ID3D12Resource> currentBackBuffer = backBuffers[currentSwapBuffer];

	// Clear the render target
	//{
	//	// Transition the back buffer from present to render target
	//	D3D12_RESOURCE_BARRIER rb = {};
	//	rb.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	//	rb.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	//	rb.Transition.pResource = currentBackBuffer.Get();
	//	rb.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	//	rb.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	//	rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	//	commandList->ResourceBarrier(1, &rb);

	//	// Background color for clearing
	//	float color[] = { 0.4f, 0.6f, 0.75f, 1.0f };

	//	// Clear the RTV
	//	commandList->ClearRenderTargetView(
	//		rtvHandles[currentSwapBuffer],
	//		color,
	//		0, 0); // No scissor rectangles

	//	// Clear the depth buffer
	//	commandList->ClearDepthStencilView(
	//		dsvHandle,
	//		D3D12_CLEAR_FLAG_DEPTH,
	//		1.0f, // Max depth = 1.0f
	//		0,
	//		0, 0); // No scissor rectangles
	//}

	// Render
	//{
	//	// Set overall pipeline state
	//	commandList->SetPipelineState(pipelineState.Get());

	//	// Set root signature
	//	commandList->SetGraphicsRootSignature(rootSignature.Get());

	//	// Set up other commands for rendering
	//	commandList->OMSetRenderTargets(1, &rtvHandles[currentSwapBuffer], true, &dsvHandle);
	//	commandList->RSSetViewports(1, &viewport);
	//	commandList->RSSetScissorRects(1, &scissorRect);
	//	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap = DX12Helper::GetInstance().GetCBVSRVDescriptorHeap();
	//	commandList->SetDescriptorHeaps(1, descriptorHeap.GetAddressOf());

	//	for (auto& ge : entities) {
	//		std::shared_ptr<Material> mat = ge->GetMaterial();
	//		commandList->SetPipelineState(mat->GetPipelineState().Get());

	//		// Vertex shader
	//		VertexShaderExternalData vsData = {};
	//		vsData.world = ge->GetTransform()->GetWorldMatrix();
	//		vsData.worldInvTranspose = ge->GetTransform()->GetWorldInverseTransposeMatrix();
	//		vsData.view = camera->GetView();
	//		vsData.projection = camera->GetProjection();

	//		D3D12_GPU_DESCRIPTOR_HANDLE handleVS =
	//			DX12Helper::GetInstance().FillNextConstantBufferAndGetGPUDescriptorHandle((void*)(&vsData), sizeof(VertexShaderExternalData));
	//		commandList->SetGraphicsRootDescriptorTable(0, handleVS);

	//		// Pixel shader
	//		PixelShaderExternalData psData = {};
	//		psData.uvScale = mat->GetUVScale();
	//		psData.uvOffset = mat->GetUVOffset();
	//		psData.cameraPos = camera->GetTransform()->GetPosition();
	//		psData.lightCount = 1;
	//		memcpy(psData.lights, &lights[0], sizeof(Light) * NUM_LIGHTS);

	//		D3D12_GPU_DESCRIPTOR_HANDLE handlePS =
	//			DX12Helper::GetInstance().FillNextConstantBufferAndGetGPUDescriptorHandle((void*)(&psData), sizeof(PixelShaderExternalData));
	//		commandList->SetGraphicsRootDescriptorTable(1, handlePS);

	//		commandList->SetGraphicsRootDescriptorTable(2, mat->GetFinalGPUHandle());

	//		std::shared_ptr<Mesh> mesh = ge->GetMesh();
	//		D3D12_VERTEX_BUFFER_VIEW vbView = mesh->GetVBView();
	//		D3D12_INDEX_BUFFER_VIEW ibView = mesh->GetIBView();
	//		commandList->IASetVertexBuffers(0, 1, &vbView);
	//		commandList->IASetIndexBuffer(&ibView);

	//		// Draw
	//		commandList->DrawIndexedInstanced(mesh->GetIndexCount(), 1, 0, 0, 0);
	//	}
	//}

	// Present
	{
		// Transition back to present
		//D3D12_RESOURCE_BARRIER rb = {};
		//rb.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		//rb.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		//rb.Transition.pResource = currentBackBuffer.Get();
		//rb.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		//rb.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		//rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		//commandList->ResourceBarrier(1, &rb);

		//// Must occur before present
		//DX12Helper::GetInstance().CloseExecuteAndResetCommandList();

		commandAllocator->Reset();
		commandList->Reset(commandAllocator.Get(), 0);

		// Update raytracing acceleration structure
		RaytracingHelper::GetInstance().CreateTopLevelAccelerationStructureForScene(entities);

		// Perform raytrace, execute command list
		RaytracingHelper::GetInstance().Raytrace(camera, backBuffers[currentSwapBuffer]);

		// Present the current back buffer
		bool vsyncNecessary = vsync || !deviceSupportsTearing || isFullscreen;
		swapChain->Present(
			vsyncNecessary ? 1 : 0,
			vsyncNecessary ? 0 : DXGI_PRESENT_ALLOW_TEARING);

		// Figure out which buffer is next
		currentSwapBuffer++;
		if (currentSwapBuffer >= numBackBuffers)
			currentSwapBuffer = 0;
	}
}