#include "Renderer.h"
#include "SimpleShader.h"
#include "Helpers.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"

using namespace DirectX;

#define RandomRange(min, max) (float)rand() / RAND_MAX * (max - min) + min
#define LoadShader(type, file) std::make_shared<type>(device.Get(), context.Get(), FixPath(file).c_str())

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

	CreateSamplers();

	// Create render targets
	CreateRenderTarget(windowWidth, windowHeight, sceneColorsRTV, sceneColorsSRV);
	CreateRenderTarget(windowWidth, windowHeight, ambientColorsRTV, ambientColorsSRV);
	CreateRenderTarget(windowWidth, windowHeight, depthsRTV, depthsSRV);
	CreateRenderTarget(windowWidth, windowHeight, normalsRTV, normalsSRV);

	CreateRenderTarget(windowWidth, windowHeight, ssaoOutputRTV, ssaoOutputSRV);
	CreateRenderTarget(windowWidth, windowHeight, ssaoBlurredRTV, ssaoBlurredSRV);

	// Create SSAO offset vectors
	for (int i = 0; i < 64; i++)
	{
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

	// Create random texture for SSAO
	CreateRandomTexture();
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

	// Unbind all SRVs
	ID3D11ShaderResourceView* nullSRVs[6] = {};
	context->PSSetShaderResources(0, 6, nullSRVs);
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

		ps->SetSamplerState("BasicSampler", basicSampler);
		ps->SetSamplerState("ClampSampler", clampSampler);

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

	// Get SSAO results
	targets[0] = ssaoOutputRTV.Get();
	targets[1] = 0;
	targets[2] = 0;
	targets[3] = 0;
	context->OMSetRenderTargets(4, targets, depthBufferDSV.Get());

	std::shared_ptr<SimpleVertexShader> vs = LoadShader(SimpleVertexShader, L"FullscreenVS.cso");
	std::shared_ptr<SimplePixelShader> ps = LoadShader(SimplePixelShader, L"SsaoPS.cso");
	vs->SetShader();
	ps->SetShader();

	ps->SetShaderResourceView("Normals", normalsSRV);
	ps->SetShaderResourceView("Depths", depthsSRV);
	ps->SetShaderResourceView("Random", randomTextureSRV);

	ps->SetSamplerState("BasicSampler", basicSampler);
	ps->SetSamplerState("ClampSampler", clampSampler);

	XMFLOAT4X4 inverseProj, proj = camera->GetProjection();
	XMStoreFloat4x4(&inverseProj, XMMatrixInverse(0, XMLoadFloat4x4(&proj)));
	ps->SetMatrix4x4("viewMatrix", camera->GetView());
	ps->SetMatrix4x4("projMatrix", proj);
	ps->SetMatrix4x4("inverseProjMatrix", inverseProj);
	ps->SetData("offsets", ssaoOffsets, sizeof(XMFLOAT4) * ARRAYSIZE(ssaoOffsets));
	ps->SetFloat("ssaoRadius", 10);
	ps->SetInt("ssaoSamples", 16);
	ps->SetFloat2("randomTextureSceenScale", XMFLOAT2(windowWidth / 4.0f, windowHeight / 4.0f));
	ps->CopyAllBufferData();

	context->Draw(3, 0);

	// Get SSAO Blur
	targets[0] = ssaoBlurredRTV.Get();
	context->OMSetRenderTargets(1, targets, 0);

	ps = LoadShader(SimplePixelShader, L"SsaoBlurPS.cso");
	ps->SetShader();

	ps->SetShaderResourceView("SSAO", ssaoOutputSRV);
	ps->SetSamplerState("ClampSampler", clampSampler);

	ps->SetFloat2("pixelSize", XMFLOAT2(1.0f / windowWidth, 1.0f / windowHeight));
	ps->CopyAllBufferData();

	context->Draw(3, 0);

	// Combine
	targets[0] = backBufferRTV.Get();
	context->OMSetRenderTargets(1, targets, 0);

	ps = LoadShader(SimplePixelShader, L"SsaoCombinePS.cso");
	ps->SetShader();

	ps->SetShaderResourceView("SceneColorsNoAmbient", sceneColorsSRV);
	ps->SetShaderResourceView("Ambient", ambientColorsSRV);
	ps->SetShaderResourceView("SSAOBlur", ssaoBlurredSRV);

	ps->SetSamplerState("BasicSampler", basicSampler);
	ps->CopyAllBufferData();

	context->Draw(3, 0);
}

void Renderer::CreateRenderTarget(unsigned int width,
	unsigned int height,
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView>& rtv,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srv)
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

void Renderer::CreateRandomTexture()
{
	const int textureSize = 4;
	const int totalPixels = textureSize * textureSize;
	XMFLOAT4 randomPixels[totalPixels] = {};
	for (int i = 0; i < totalPixels; i++)
	{
		XMVECTOR randomVec = XMVectorSet(RandomRange(-1, 1), RandomRange(-1, 1), 0, 0);
		XMStoreFloat4(&randomPixels[i], XMVector3Normalize(randomVec));
	}

	D3D11_SUBRESOURCE_DATA data = {};
	data.pSysMem = randomPixels;
	data.SysMemPitch = sizeof(int) * 16;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> randomTexture;
	D3D11_TEXTURE2D_DESC texDesc = {};
	texDesc.Width = 4;
	texDesc.Height = 4;
	texDesc.ArraySize = 1;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.MipLevels = 1;
	texDesc.MiscFlags = 0;
	texDesc.SampleDesc.Count = 1;

	device->CreateTexture2D(&texDesc, &data, randomTexture.GetAddressOf());
	device->CreateShaderResourceView(randomTexture.Get(), 0, randomTextureSRV.GetAddressOf());
}

void Renderer::CreateSamplers()
{
	D3D11_SAMPLER_DESC sampDesc = {};
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	sampDesc.MaxAnisotropy = 16;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	device->CreateSamplerState(&sampDesc, basicSampler.GetAddressOf());

	D3D11_SAMPLER_DESC clampSampDesc = {};
	clampSampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	clampSampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	clampSampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	clampSampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	clampSampDesc.MaxAnisotropy = 16;
	clampSampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	device->CreateSamplerState(&clampSampDesc, clampSampler.GetAddressOf());
}