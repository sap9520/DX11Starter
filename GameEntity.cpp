#include "GameEntity.h"
#include "BufferStructs.h"
using namespace DirectX;

GameEntity::GameEntity(
	std::shared_ptr<Mesh> _mesh, 
	std::shared_ptr<Material> _material)
	:
	mesh(_mesh),
	material(_material)
{

}

Transform* GameEntity::GetTransform()
{
	return &transform;
}

std::shared_ptr<Mesh> GameEntity::GetMesh()
{
	return mesh;
}

std::shared_ptr<Material> GameEntity::GetMaterial() {
	return material;
}

void GameEntity::SetMaterial(Material _material) {
	material = std::make_shared<Material>(_material);
}

void GameEntity::Draw(
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, 
	Camera* camera) 
{
	// Set shader data
	{
		material->GetVertexShader()->SetShader();
		material->GetPixelShader()->SetShader();

		std::shared_ptr<SimpleVertexShader> vs = material->GetVertexShader();
		vs->SetMatrix4x4("world", transform.GetWorldMatrix());
		vs->SetMatrix4x4("worldInvTranspose", transform.GetWorldInverseTransposeMatrix());
		vs->SetMatrix4x4("view", camera->GetViewMatrix());
		vs->SetMatrix4x4("projection", camera->GetProjectionMatrix());

		vs->CopyAllBufferData();

		std::shared_ptr<SimplePixelShader> ps = material->GetPixelShader();
		ps->SetFloat4("colorTint", material->GetColorTint());
		ps->SetFloat3("cameraPosition", camera->GetTransform().GetPosition());
		ps->SetFloat("roughness", material->GetRoughness());

		ps->CopyAllBufferData();
	}

	// DRAW geometry
	// - These steps are generally repeated for EACH object you draw
	// - Other Direct3D calls will also be necessary to do more complex things
	{
		mesh->Draw(context);
	}
}