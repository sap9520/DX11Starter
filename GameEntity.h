#pragma once

#include "Transform.h"
#include "Mesh.h"
#include <memory>

class GameEntity
{
public:
	GameEntity(std::shared_ptr<Mesh> _mesh);

	Transform* GetTransform();
	std::shared_ptr<Mesh> GetMesh();

	void Draw(
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
		Microsoft::WRL::ComPtr<ID3D11Buffer> vsConstantBuffer);

private:
	Transform transform;
	std::shared_ptr<Mesh> mesh;
};

