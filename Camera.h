#pragma once

#include <DirectXMath.h>
#include "Input.h"
#include "Transform.h"

class Camera
{
public:
	Camera();
	Camera(float aspectRatio, DirectX::XMFLOAT3 initialPos);

	DirectX::XMFLOAT4X4 GetViewMatrix();
	DirectX::XMFLOAT4X4 GetProjectionMatrix();
	Transform GetTransform();

	void UpdateProjectionMatrix(float aspectRatio);
	void UpdateViewMatrix();
	void Update(float dt);


private:
	Transform transform;
	DirectX::XMFLOAT4X4 viewMat;
	DirectX::XMFLOAT4X4 projectionMat;

	float fov;
	float nearClipDist;
	float farClipDist;
	float movementSpeed;
	float mouseLookSpeed;
	bool isPerspective;
};

