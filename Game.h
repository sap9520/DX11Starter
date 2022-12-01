#pragma once

#include "DXCore.h"
#include "Mesh.h"
#include "GameEntity.h"
#include "Camera.h"
#include "SimpleShader.h"
#include "Lights.h"
#include "Sky.h"

#include <DirectXMath.h>
#include <wrl/client.h> // Used for ComPtr - a smart pointer for COM objects
#include <memory>
#include <vector>

class Game 
	: public DXCore
{

public:
	Game(HINSTANCE hInstance);
	~Game();

	// Overridden setup and game loop methods, which
	// will be called automatically
	void Init();
	void OnResize();
	void Update(float deltaTime, float totalTime);
	void Draw(float deltaTime, float totalTime);

private:

	// Initialization helper methods - feel free to customize, combine, remove, etc.
	void InitLights();
	void LoadShaders();
	void LoadTextures();
	void CreateGeometry();
	void CreateShadowResources();
	void RenderShadowMap();

	// Note the usage of ComPtr below
	//  - This is a smart pointer for objects that abide by the
	//     Component Object Model, which DirectX objects do
	//  - More info here: https://github.com/Microsoft/DirectXTK/wiki/ComPtr

	// Buffers to hold actual geometry data
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
	
	// Shaders and shader-related constructs
	std::shared_ptr<SimplePixelShader> pixelShader;
	std::shared_ptr<SimplePixelShader> customPixelShader;
	std::shared_ptr<SimpleVertexShader> vertexShader;

	std::vector<Light> lights;
	std::shared_ptr<Material> metalMat;
	std::shared_ptr<Material> tileMat;
	std::shared_ptr<Material> bronzeMat;

	std::shared_ptr<Mesh> cubeMesh;
	std::shared_ptr<Mesh> cylMesh;
	std::shared_ptr<Mesh> helixMesh;
	std::shared_ptr<Mesh> quadMesh;
	std::shared_ptr<Mesh> doubleSidedQuadMesh;
	std::shared_ptr<Mesh> sphereMesh;
	std::shared_ptr<Mesh> torusMesh;

	std::vector<GameEntity> entities;

	// Shadow mapping variables
	UINT shadowMapRes;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> shadowTexture;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> shadowDSV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shadowSRV;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> shadowRTV;

	Camera camera;
	std::shared_ptr<Sky> sky;
};

