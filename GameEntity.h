#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <memory>
#include "Mesh.h"
#include "Transform.h"

class GameEntity
{
public:
	GameEntity(std::shared_ptr<Mesh> _mesh);

	Transform* GetTransform();
	std::shared_ptr<Mesh> GetMesh();
	void SetMesh(std::shared_ptr<Mesh> _mesh);

private:
	std::shared_ptr<Mesh> mesh;
	Transform transform;
};

