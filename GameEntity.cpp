#include "GameEntity.h"
#include "BufferStructs.h"
using namespace DirectX;

GameEntity::GameEntity(std::shared_ptr<Mesh> _mesh)
{
	mesh = _mesh;
}

Transform* GameEntity::GetTransform()
{
	return &transform;
}

std::shared_ptr<Mesh> GameEntity::GetMesh()
{
	return mesh;
}

void GameEntity::Draw(
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, 
	Microsoft::WRL::ComPtr<ID3D11Buffer> vsConstantBuffer,
	Camera* camera) 
{
	// Set shader data
	{
		VertexShaderExternalData vsData;
		vsData.colorTint = XMFLOAT4(1.0f, 0.5f, 0.5f, 1.0f);
		vsData.worldMatrix = transform.GetWorldMatrix();
		vsData.viewMatrix = camera->GetViewMatrix();
		vsData.projectionMatrix = camera->GetProjectionMatrix();

		D3D11_MAPPED_SUBRESOURCE mappedBuffer = {};
		context->Map(vsConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedBuffer);

		memcpy(mappedBuffer.pData, &vsData, sizeof(vsData));

		context->Unmap(vsConstantBuffer.Get(), 0);

		context->VSSetConstantBuffers(
			0,		// Which slot (register) to bind the buffer to
			1,		// How many are we activating? Can do multiple at once
			vsConstantBuffer.GetAddressOf()); // Array of buffers (or the address of one)
	}

	// DRAW geometry
	// - These steps are generally repeated for EACH object you draw
	// - Other Direct3D calls will also be necessary to do more complex things
	{
		mesh->Draw();
	}
}