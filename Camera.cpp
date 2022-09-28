#include "Camera.h"
using namespace DirectX;

Camera::Camera() :
	fov(1.0472f),
	nearClipDist(0.1f),
	farClipDist(800),
	movementSpeed(1.0f),
	mouseLookSpeed(0.1f)
{
	transform = Transform();

	UpdateViewMatrix();
	UpdateProjectionMatrix(1.77f);
}

Camera::Camera(float aspectRatio, DirectX::XMFLOAT3 initialPos) :
	fov(1.0472f),
	nearClipDist(0.1f),
	farClipDist(100),
	movementSpeed(1.0f),
	mouseLookSpeed(0.1f)
{
	transform = Transform();
	transform.SetPosition(initialPos.x, initialPos.y, initialPos.z);

	UpdateViewMatrix();
	UpdateProjectionMatrix(aspectRatio);
}

XMFLOAT4X4 Camera::GetViewMatrix() { return viewMat; }
XMFLOAT4X4 Camera::GetProjectionMatrix() { return projectionMat; }

void Camera::UpdateProjectionMatrix(float aspectRatio) {
	XMStoreFloat4x4(&projectionMat, XMMatrixPerspectiveFovLH(fov, aspectRatio, nearClipDist, farClipDist));
}

void Camera::UpdateViewMatrix() {
	XMVECTOR pos = XMVectorSet(transform.GetPosition().x, transform.GetPosition().y, transform.GetPosition().z, 0);
	XMVECTOR direction = XMVectorSet(transform.GetForward().x, transform.GetForward().y, transform.GetForward().z, 0);

	XMStoreFloat4x4(&viewMat, XMMatrixLookToLH(pos, direction, XMVectorSet(0, 1, 0, 0)));
}

void Camera::Update(float dt) {
	Input& input = Input::GetInstance();

	// keyboard inputs
	{
		// move forward and backward
		if (input.KeyDown('W')) {
			transform.MoveRelative(0, 0, movementSpeed * dt);
		}
		if (input.KeyDown('S')) {
			transform.MoveRelative(0, 0, -movementSpeed * dt);
		}

		// move left and right
		if (input.KeyDown('A')) {
			transform.MoveRelative(-movementSpeed * dt, 0, 0);
		}
		if (input.KeyDown('D')) {
			transform.MoveRelative(movementSpeed * dt, 0, 0);
		}

		// move up and down y axis
		if (input.KeyDown(VK_SPACE)) {
			transform.MoveAbsolute(0, movementSpeed * dt, 0);
		}
		if (input.KeyDown('X')) {
			transform.MoveAbsolute(0, -movementSpeed * dt, 0);
		}
	}

	// mouse inputs
	{
		if (input.MouseLeftDown()) {
			float cursorMovementX = input.GetMouseXDelta() * mouseLookSpeed * dt;
			float cursorMovementY = input.GetMouseYDelta() * mouseLookSpeed * dt;
			transform.Rotate(cursorMovementY, cursorMovementX, 0);

			XMFLOAT3 currentRot = transform.GetPitchYawRoll();
			if (currentRot.x > XM_PIDIV2) {
				currentRot.x = XM_PIDIV2;
			}
			else if (currentRot.x < -XM_PIDIV2) {
				currentRot.x = -XM_PIDIV2;
			}

			transform.SetPitchYawRoll(currentRot.x, currentRot.y, currentRot.z);
		}
	}

	UpdateViewMatrix();
}