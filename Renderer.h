#pragma once

#include "GameEntity.h"
#include "SimpleShader.h"
#include "Camera.h"
#include "Lights.h"
#include "Sky.h"

#include <wrl/client.h>
#include <d3d11.h>
#include <vector>
#include <memory>

class Renderer
{
#pragma region Singleton
public:
	// Gets the one and only instance of this class
	static Renderer& GetInstance()
	{
		if (!instance)
		{
			instance = new Renderer();
		}

		return *instance;
	}

	// Remove these functions (C++ 11 version)
	Renderer(Renderer const&) = delete;
	void operator=(Renderer const&) = delete;

private:
	static Renderer* instance;
	Renderer() :
		device(0),
		context(0),
		swapChain(0),
		backBufferRTV(0),
		depthBufferDSV(0),
		windowWidth(0),
		windowHeight(0)
	{};
#pragma endregion

public:
	~Renderer();

	void Initialize(
		Microsoft::WRL::ComPtr<ID3D11Device> _device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> _context,
		Microsoft::WRL::ComPtr<IDXGISwapChain> _swapChain,
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> _backBufferRTV,
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView> _depthBufferDSV,
		unsigned int _windowWidth,
		unsigned int _windowHeight);

	void PreResize();
	void PostResize(
		unsigned int _windowWidth,
		unsigned int _windowHeight,
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> _backBufferRTV,
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView> _depthBufferDSV);

	void FrameStart();
	void FrameEnd(bool vsync);

	void RenderScene(
		std::vector<std::shared_ptr<GameEntity>> entities,
		std::shared_ptr<Sky> sky,
		std::vector<Light> lights,
		int lightCount,
		std::shared_ptr<Camera> camera);

private:
	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
	Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV;
	unsigned int windowWidth;
	unsigned int windowHeight;
};

