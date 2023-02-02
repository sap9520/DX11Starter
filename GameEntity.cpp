#include "GameEntity.h"

GameEntity::GameEntity(std::shared_ptr<Mesh> _mesh) :
	mesh(mesh)
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

void GameEntity::SetMesh(std::shared_ptr<Mesh> _mesh)
{
	mesh = _mesh;
}

