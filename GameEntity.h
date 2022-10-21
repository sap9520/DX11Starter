#pragma once

#include "Transform.h"
#include "Mesh.h"
#include "Camera.h"
#include "Material.h"
#include <memory>

class GameEntity
{
public:
	GameEntity(std::shared_ptr<Mesh> _mesh, std::shared_ptr<Material> _material);

	Transform* GetTransform();
	std::shared_ptr<Mesh> GetMesh();
	std::shared_ptr<Material> GetMaterial();
	void SetMaterial(Material _material);

	void Draw(
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
		Camera* camera);

private:
	Transform transform;
	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<Material> material;
};

