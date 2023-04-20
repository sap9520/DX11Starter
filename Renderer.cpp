#include "Renderer.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"

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
}

Renderer::~Renderer()
{

}

// Release back buffer and depth buffer references so swap chain can resize them
void Renderer::PreResize()
{
	backBufferRTV.Reset();
	depthBufferDSV.Reset();
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
}

void Renderer::FrameStart()
{
	const float bgColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f }; // Black
	context->ClearRenderTargetView(backBufferRTV.Get(), bgColor);

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