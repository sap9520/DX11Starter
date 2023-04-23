#include "Renderer.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"

using namespace DirectX;

Renderer* Renderer::instance;

void Renderer::Initialize(
	Microsoft::WRL::ComPtr<ID3D11Device> _device,
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> _context,
	Microsoft::WRL::ComPtr<IDXGISwapChain> _swapChain,
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> _backBufferRTV,
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> _depthBufferDSV,
	unsigned int _windowWidth,
	unsigned int _windowHeight)
{
	device = _device;
	context = _context;
	swapChain = _swapChain;
	backBufferRTV = _backBufferRTV;
	depthBufferDSV = _depthBufferDSV;
	windowWidth = _windowWidth;
	windowHeight = _windowHeight;

	// Create render targets
	CreateRenderTarget(windowWidth, windowHeight, sceneColorsRTV, sceneColorsSRV);
	CreateRenderTarget(windowWidth, windowHeight, ambientColorsRTV, ambientColorsSRV);
	CreateRenderTarget(windowWidth, windowHeight, depthsRTV, depthsSRV);
	CreateRenderTarget(windowWidth, windowHeight, normalsRTV, normalsSRV);

	CreateRenderTarget(windowWidth, windowHeight, ssaoOutputRTV, ssaoOutputSRV);
	CreateRenderTarget(windowWidth, windowHeight, ssaoBlurredRTV, ssaoBlurredSRV);

	// Create SSAO offset vectors
	for (int i = 0; i < 64; i++) {
		// Offsets are in a hemisphere range of ([-1, 1], [-1, 1], [0, 1])
		ssaoOffsets[i] = XMFLOAT4(
			(float)rand() / RAND_MAX * 2 - 1,	// -1 to 1
			(float)rand() / RAND_MAX * 2 - 1,	// -1 to 1
			(float)rand() / RAND_MAX,			// 0 to 1
			0);
		XMVECTOR offset = XMVector3Normalize(XMLoadFloat4(&ssaoOffsets[i]));

		// Scale over array to weigh values closer to the minimum
		float scale = (float)i / 64;
		XMVECTOR acceleratedScale = XMVectorLerp(
			XMVectorSet(0.1f, 0.1f, 0.1f, 1),
			XMVectorSet(1, 1, 1, 1),
			scale * scale);
		XMStoreFloat4(&ssaoOffsets[i], offset * acceleratedScale);
	}
}

Renderer::~Renderer()
{

}

// Release back buffer and depth buffer references so swap chain can resize them
void Renderer::PreResize()
{
	backBufferRTV.Reset();
	depthBufferDSV.Reset();

	sceneColorsRTV.Reset();
	sceneColorsSRV.Reset();

	ambientColorsRTV.Reset();
	ambientColorsSRV.Reset();

	depthsRTV.Reset();
	depthsSRV.Reset();

	normalsRTV.Reset();
	normalsSRV.Reset();

	ssaoOutputRTV.Reset();
	ssaoOutputSRV.Reset();

	ssaoBlurredRTV.Reset();
	ssaoBlurredSRV.Reset();
}

// Update variables to match new screen size
void Renderer::PostResize(
	unsigned int _windowWidth,
	unsigned int _windowHeight,
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> _backBufferRTV,
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> _depthBufferDSV)
{
	windowWidth = _windowWidth;
	windowHeight = _windowHeight;
	backBufferRTV = _backBufferRTV;
	depthBufferDSV = _depthBufferDSV;

	// Rereate render targets
	CreateRenderTarget(windowWidth, windowHeight, sceneColorsRTV, sceneColorsSRV);
	CreateRenderTarget(windowWidth, windowHeight, ambientColorsRTV, ambientColorsSRV);
	CreateRenderTarget(windowWidth, windowHeight, depthsRTV, depthsSRV);
	CreateRenderTarget(windowWidth, windowHeight, normalsRTV, normalsSRV);

	CreateRenderTarget(windowWidth, windowHeight, ssaoOutputRTV, ssaoOutputSRV);
	CreateRenderTarget(windowWidth, windowHeight, ssaoBlurredRTV, ssaoBlurredSRV);
}

void Renderer::FrameStart()
{
	const float bgColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f }; // Black
	context->ClearRenderTargetView(backBufferRTV.Get(), bgColor);

	context->ClearRenderTargetView(sceneColorsRTV.Get(), bgColor);
	context->ClearRenderTargetView(ambientColorsRTV.Get(), bgColor);
	context->ClearRenderTargetView(depthsRTV.Get(), bgColor);
	context->ClearRenderTargetView(normalsRTV.Get(), bgColor);
	context->ClearRenderTargetView(ssaoOutputRTV.Get(), bgColor);
	context->ClearRenderTargetView(ssaoBlurredRTV.Get(), bgColor);

	// Clear the depth buffer (resets per-pixel occlusion information)
	context->ClearDepthStencilView(depthBufferDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void Renderer::FrameEnd(bool vsync)
{
	// Draw the UI after everything else
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	// Present the back buffer to the user
	//  - Puts the results of what we've drawn onto the window
	//  - Without this, the user never sees anything
	swapChain->Present(
		vsync ? 1 : 0,
		vsync ? 0 : DXGI_PRESENT_ALLOW_TEARING);

	// Must re-bind buffers after presenting, as they become unbound
	context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthBufferDSV.Get());
}

void Renderer::RenderScene(
	std::vector<std::shared_ptr<GameEntity>> entities,
	std::shared_ptr<Sky> sky,
	std::vector<Light> lights,
	int lightCount,
	std::shared_ptr<Camera> camera)
{
	ID3D11RenderTargetView* targets[4] = {};
	targets[0] = sceneColorsRTV.Get();
	targets[1] = ambientColorsRTV.Get();
	targets[2] = normalsRTV.Get();
	targets[3] = depthsRTV.Get();
	context->OMSetRenderTargets(4, targets, depthBufferDSV.Get());

	// Draw all of the entities
	for (auto& ge : entities)
	{
		// Set the "per frame" data
		// Note that this should literally be set once PER FRAME, before
		// the draw loop, but we're currently setting it per entity since 
		// we are just using whichever shader the current entity has.  
		// Inefficient!!!
		std::shared_ptr<SimplePixelShader> ps = ge->GetMaterial()->GetPixelShader();

		ps->SetShaderResourceView("BrdfLookUpMap", sky->GetBRDFLookUpTexture());
		ps->SetShaderResourceView("IrradianceIBLMap", sky->GetIrradianceMap());
		ps->SetShaderResourceView("SpecularIBLMap", sky->GetConvolvedSpecularMap());

		ps->SetData("lights", (void*)(&lights[0]), sizeof(Light) * lightCount);
		ps->SetInt("lightCount", lightCount);
		ps->SetFloat3("cameraPosition", camera->GetTransform()->GetPosition());
		ps->SetInt("specIBLTotalMipLevels", sky->GetNumSpecMipLevels());
		ps->CopyBufferData("perFrame");

		// Draw the entity
		ge->Draw(context, camera);
	}

	// Draw the sky
	sky->Draw(camera);
}

void Renderer::CreateRenderTarget(unsigned int width,
	unsigned int height,
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> rtv,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv)
{
	// Create texture
	Microsoft::WRL::ComPtr<ID3D11Texture2D> rtTexture;

	D3D11_TEXTURE2D_DESC texDesc = {};
	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.ArraySize = 1;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.MipLevels = 1;
	texDesc.MiscFlags = 0;
	texDesc.SampleDesc.Count = 1;
	
	device->CreateTexture2D(&texDesc, 0, rtTexture.GetAddressOf());

	// Create render target view
	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.Format = texDesc.Format;

	device->CreateRenderTargetView(rtTexture.Get(), &rtvDesc, rtv.GetAddressOf());

	// Create shader resource view
	device->CreateShaderResourceView(rtTexture.Get(), 0, srv.GetAddressOf());
}