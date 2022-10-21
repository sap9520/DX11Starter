#include "Game.h"
#include "Vertex.h"
#include "Input.h"
#include "Helpers.h"
#include "Material.h"

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"

// Needed for a helper function to load pre-compiled shader files
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>
#include "BufferStructs.h"

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

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

// --------------------------------------------------------
// Called once per program, after Direct3D and the window
// are initialized but before the game loop.
// --------------------------------------------------------
void Game::Init()
{
	// Initialize ImGui itself & platform/renderer backends
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplWin32_Init(hWnd);
	ImGui_ImplDX11_Init(device.Get(), context.Get());

	ImGui::StyleColorsDark();

	// Helper methods for loading shaders, creating some basic
	// geometry to draw and some simple camera matrices.
	//  - You'll be expanding and/or replacing these later
	InitLights();
	LoadShaders();

	ambientColor = XMFLOAT3(0.1f, 0.1f, 0.25f);
	customMat = std::make_shared<Material>(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), vertexShader, pixelShader, 1.0f);

	CreateGeometry();
	
	// Set initial graphics API state
	//  - These settings persist until we change them
	//  - Some of these, like the primitive topology & input layout, probably won't change
	//  - Others, like setting shaders, will need to be moved elsewhere later
	{
		// Tell the input assembler (IA) stage of the pipeline what kind of
		// geometric primitives (points, lines or triangles) we want to draw.  
		// Essentially: "What kind of shape should the GPU draw with our vertices?"
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}

	camera = Camera();
}

void Game::InitLights() {
	lights = std::vector<Light>();
	Light light1 = {};
	lights.push_back(light1);
	lights[0] = {};
	lights[0].Type = LIGHT_TYPE_DIRECTIONAL;
	lights[0].Direction = XMFLOAT3(1, 0, 0);
	lights[0].Color = XMFLOAT3(0.5f, 0.0f, 0.0f);
	lights[0].Intensity = 1.0f;

	Light light2 = {};
	lights.push_back(light2);
	lights[1] = {};
	lights[1].Type = LIGHT_TYPE_DIRECTIONAL;
	lights[1].Direction = XMFLOAT3(-1, 0, 0);
	lights[1].Color = XMFLOAT3(0.0f, 0.5f, 0.0f);
	lights[1].Intensity = 1.0f;

	Light light3 = {};
	lights.push_back(light3);
	lights[2] = {};
	lights[2].Type = LIGHT_TYPE_DIRECTIONAL;
	lights[2].Direction = XMFLOAT3(0, 1, 0);
	lights[2].Color = XMFLOAT3(0.0f, 0.0f, 0.5f);
	lights[2].Intensity = 1.0f;

	Light light4 = {};
	lights.push_back(light4);
	lights[3] = {};
	lights[3].Type = LIGHT_TYPE_POINT;
	lights[3].Direction = XMFLOAT3(0.5f, 0.5f, 0);
	lights[3].Color = XMFLOAT3(0.2f, 0.0f, 0.2f);
	lights[3].Range = D3D11_FLOAT32_MAX;
	lights[3].Intensity = 0.2f;

	Light light5 = {};
	lights.push_back(light5);
	lights[4] = {};
	lights[4].Type = LIGHT_TYPE_POINT;
	lights[4].Direction = XMFLOAT3(-0.5f, -0.5f, 0);
	lights[4].Color = XMFLOAT3(0.2f, 0.2f, 0.0f);
	lights[4].Range = D3D11_FLOAT32_MAX;
	lights[4].Intensity = 0.2f;
}

// --------------------------------------------------------
// Loads shaders from compiled shader object (.cso) files
// and also created the Input Layout that describes our 
// vertex data to the rendering pipeline. 
// - Input Layout creation is done here because it must 
//    be verified against vertex shader byte code
// - We'll have that byte code already loaded below
// --------------------------------------------------------
void Game::LoadShaders()
{
	vertexShader = std::make_shared<SimpleVertexShader>(device, context, FixPath(L"VertexShader.cso").c_str());
	pixelShader = std::make_shared<SimplePixelShader>(device, context, FixPath(L"PixelShader.cso").c_str());
	customPixelShader = std::make_shared<SimplePixelShader>(device, context, FixPath(L"CustomPS.cso").c_str());
}



// --------------------------------------------------------
// Creates the geometry we're going to draw - a single triangle for now
// --------------------------------------------------------
void Game::CreateGeometry()
{
	cubeMesh = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/cube.obj").c_str(), device, context);
	cylMesh = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/cylinder.obj").c_str(), device, context);
	helixMesh = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/helix.obj").c_str(), device, context);
	quadMesh = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/quad.obj").c_str(), device, context);
	doubleSidedQuadMesh = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/quad_double_sided.obj").c_str(), device, context);
	sphereMesh = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/sphere.obj").c_str(), device, context);
	torusMesh = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/torus.obj").c_str(), device, context);

	entities = std::vector<GameEntity>();
	entities.push_back(GameEntity(cubeMesh, customMat));
	entities.push_back(GameEntity(cylMesh, customMat));
	entities.push_back(GameEntity(helixMesh, customMat));
	entities.push_back(GameEntity(quadMesh, customMat));
	entities.push_back(GameEntity(doubleSidedQuadMesh, customMat));
	entities.push_back(GameEntity(sphereMesh, customMat));
	entities.push_back(GameEntity(torusMesh, customMat));

	entities[0].GetTransform()->SetPosition(-8.0f, +0.0f, +10.0f);
	entities[1].GetTransform()->SetPosition(-5.0f, +0.0f, +10.0f);
	entities[2].GetTransform()->SetPosition(-2.0f, +0.0f, +10.0f);
	entities[3].GetTransform()->SetPosition(+0.0f, +0.0f, +10.0f);
	entities[4].GetTransform()->SetPosition(+2.0f, +0.0f, +10.0f);
	entities[5].GetTransform()->SetPosition(+5.0f, +0.0f, +10.0f);
	entities[6].GetTransform()->SetPosition(+8.0f, +0.0f, +10.0f);
}


// --------------------------------------------------------
// Handle resizing to match the new window size.
//  - DXCore needs to resize the back buffer
//  - Eventually, we'll want to update our 3D camera
// --------------------------------------------------------
void Game::OnResize()
{
	// Handle base-level DX resize stuff
	DXCore::OnResize();

	camera.UpdateProjectionMatrix((float)this->windowWidth / this->windowHeight);
}

// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{
	// Get a reference to our custom input manager
	Input& input = Input::GetInstance();
	// Reset input manager's gui state so we don’t
	// taint our own input (you’ll uncomment later)
	input.SetKeyboardCapture(false);
	input.SetMouseCapture(false);
	// Feed fresh input data to ImGui
	ImGuiIO& io = ImGui::GetIO();
	io.DeltaTime = deltaTime;
	io.DisplaySize.x = (float)this->windowWidth;
	io.DisplaySize.y = (float)this->windowHeight;
	io.KeyCtrl = input.KeyDown(VK_CONTROL);
	io.KeyShift = input.KeyDown(VK_SHIFT);
	io.KeyAlt = input.KeyDown(VK_MENU);
	io.MousePos.x = (float)input.GetMouseX();
	io.MousePos.y = (float)input.GetMouseY();
	io.MouseDown[0] = input.MouseLeftDown();
	io.MouseDown[1] = input.MouseRightDown();
	io.MouseDown[2] = input.MouseMiddleDown();
	io.MouseWheel = input.GetMouseWheel();
	input.GetKeyArray(io.KeysDown, 256);
	// Reset the frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	// Determine new input capture (you’ll uncomment later)
	input.SetKeyboardCapture(io.WantCaptureKeyboard);
	input.SetMouseCapture(io.WantCaptureMouse);
	// Show the demo window
	ImGui::ShowDemoWindow();

	// ImGui Windows
	{
		ImGui::Begin("Data");
		ImGui::Text("Current FPS: %f", io.Framerate);
		ImGui::End();

		ImGui::Begin("Object Inspector");
		XMFLOAT3 currentPos;
		for (int i = 0; i < entities.size(); i++) {
			ImGui::Text("Entity %i Movement Controls", i);

			currentPos = entities[i].GetTransform()->GetPosition();
			ImGui::PushID("ent" + i);
			if (ImGui::DragFloat3("##", &currentPos.x, 0.1f, -1.0f, 1.0f)) {
				entities[i].GetTransform()->SetPosition(currentPos.x, currentPos.y, currentPos.z);
			}
			ImGui::PopID();
		}
		ImGui::End();

		ImGui::Begin("Light Inspector");
		XMFLOAT3 currentColor;
		for (int i = 0; i < lights.size(); i++) {
			ImGui::Text("Light %i Color Controls", i);
			currentColor = lights[i].Color;
			ImGui::PushID("light" + i);
			if (ImGui::DragFloat3("##", &currentColor.x, 0.1f, -1.0f, 1.0f)) {
				lights[i].Color = XMFLOAT3(currentColor.x, currentColor.y, currentColor.z);
			}
			ImGui::PopID();
		}
		ImGui::End();
	}

	// Example input checking: Quit if the escape key is pressed
	if (Input::GetInstance().KeyDown(VK_ESCAPE))
		Quit();

	camera.Update(deltaTime);
}

// --------------------------------------------------------
// Clear the screen, redraw everything, present to the user
// --------------------------------------------------------
void Game::Draw(float deltaTime, float totalTime)
{
	// Frame START
	// - These things should happen ONCE PER FRAME
	// - At the beginning of Game::Draw() before drawing *anything*
	{
		// Clear the back buffer (erases what's on the screen)
		const float bgColor[4] = { 0.4f, 0.6f, 0.75f, 1.0f }; // Cornflower Blue
		context->ClearRenderTargetView(backBufferRTV.Get(), bgColor);

		// Clear the depth buffer (resets per-pixel occlusion information)
		context->ClearDepthStencilView(depthBufferDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	}

	for(GameEntity ge : entities) {
		ge.GetMaterial()->GetPixelShader()->SetFloat3("ambient", ambientColor);
		ge.GetMaterial()->GetPixelShader()->SetData("lights", &lights[0], sizeof(Light) * (int)lights.size());
		ge.Draw(context, &camera);
	}

	// Frame END
	// - These should happen exactly ONCE PER FRAME
	// - At the very end of the frame (after drawing *everything*)
	{
		// Draw ImGui
		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		// Present the back buffer to the user
		//  - Puts the results of what we've drawn onto the window
		//  - Without this, the user never sees anything
		swapChain->Present(vsync ? 1 : 0, 0);

		// Must re-bind buffers after presenting, as they become unbound
		context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthBufferDSV.Get());
	}
}