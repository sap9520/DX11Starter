#include "GameEntity.h"

GameEntity::GameEntity(std::shared_ptr<Mesh> _mesh, std::shared_ptr<Material> _material) :
	mesh(_mesh),
	material(_material)
{
}

Transform* GameEntity::GetTransform() { return &transform; }
std::shared_ptr<Mesh> GameEntity::GetMesh() { return mesh; }
std::shared_ptr<Material> GameEntity::GetMaterial() { return material; }

void GameEntity::SetMesh(std::shared_ptr<Mesh> _mesh)
{
	mesh = _mesh;
}
void GameEntity::SetMaterial(std::shared_ptr<Material> _material)
{
	material = _material;
}

