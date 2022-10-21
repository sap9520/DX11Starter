#include "Transform.h"
using namespace DirectX;

Transform::Transform() :
	position(0, 0, 0),
	pitchYawRoll(0, 0, 0),
	scale(1, 1, 1)
{
	XMStoreFloat4x4(&world, XMMatrixIdentity());
	XMStoreFloat4x4(&worldInverseTranspose, XMMatrixIdentity());
}

void Transform::MoveAbsolute(float x, float y, float z) {
	position.x += x;
	position.y += y;
	position.z += z;
}

void Transform::MoveRelative(float x, float y, float z) {
	XMVECTOR targetPos = XMVectorSet(x, y, z, 0.0f);
	XMVECTOR currentRotation = XMQuaternionRotationRollPitchYaw(pitchYawRoll.x, pitchYawRoll.y, pitchYawRoll.z);
	XMVECTOR move = XMVector3Rotate(targetPos, currentRotation);

	XMStoreFloat3(&position, XMLoadFloat3(&position) + move);
}

void Transform::Rotate(float p, float y, float r) {
	pitchYawRoll.x += p;
	pitchYawRoll.y += y;
	pitchYawRoll.z += r;
}

void Transform::Scale(float x, float y, float z) {
	scale.x *= x;
	scale.y *= y;
	scale.z *= z;
}

void Transform::SetPosition(float x, float y, float z) {
	position.x = x;
	position.y = y;
	position.z = z;
}

void Transform::SetPitchYawRoll(float p, float y, float r) {
	pitchYawRoll.x = p;
	pitchYawRoll.y = y;
	pitchYawRoll.z = r;
}

void Transform::SetScale(float x, float y, float z) {
	scale.x = x;
	scale.y = y;
	scale.z = z;
}

DirectX::XMFLOAT3 Transform::GetPosition() { return position; }
DirectX::XMFLOAT3 Transform::GetPitchYawRoll() { return pitchYawRoll; }
DirectX::XMFLOAT3 Transform::GeScale() { return scale; }

DirectX::XMFLOAT3 Transform::GetRight() {
	XMVECTOR currentRotation = XMQuaternionRotationRollPitchYaw(pitchYawRoll.x, pitchYawRoll.y, pitchYawRoll.z);
	XMVECTOR rot = XMVector3Rotate(XMVectorSet(1, 0, 0, 0), currentRotation);

	XMFLOAT3 right;
	XMStoreFloat3(&right, rot);

	return right;
}

DirectX::XMFLOAT3 Transform::GetUp() {
	XMVECTOR currentRotation = XMQuaternionRotationRollPitchYaw(pitchYawRoll.x, pitchYawRoll.y, pitchYawRoll.z);
	XMVECTOR rot = XMVector3Rotate(XMVectorSet(0, 1, 0, 0), currentRotation);

	XMFLOAT3 up;
	XMStoreFloat3(&up, rot);

	return up;
}

DirectX::XMFLOAT3 Transform::GetForward() {
	XMVECTOR currentRotation = XMQuaternionRotationRollPitchYaw(pitchYawRoll.x, pitchYawRoll.y, pitchYawRoll.z);
	XMVECTOR rot = XMVector3Rotate(XMVectorSet(0, 0, 1, 0), currentRotation);

	XMFLOAT3 fwd;
	XMStoreFloat3(&fwd, rot);

	return fwd;
}

DirectX::XMFLOAT4X4 Transform::GetWorldMatrix() { 
	UpdateMatrices();

	return world;
}

DirectX::XMFLOAT4X4 Transform::GetWorldInverseTransposeMatrix() {
	UpdateMatrices();

	return worldInverseTranspose;
}

void Transform::UpdateMatrices() {
	XMMATRIX trans = XMMatrixTranslation(position.x, position.y, position.z);
	XMMATRIX rot = XMMatrixRotationRollPitchYaw(pitchYawRoll.x, pitchYawRoll.y, pitchYawRoll.z);
	XMMATRIX sc = XMMatrixScaling(scale.x, scale.y, scale.z);

	XMMATRIX worldMat = sc * rot * trans;

	XMStoreFloat4x4(&world, worldMat);
	XMStoreFloat4x4(&worldInverseTranspose, XMMatrixInverse(0, XMMatrixTranspose(worldMat)));

}
