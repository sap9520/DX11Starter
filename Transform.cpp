#include "Transform.h"
using namespace DirectX;

Transform::Transform() :
	position(0,0,0),
	pitchYawRoll(0,0,0),
	scale(1,1,1)
{
	XMStoreFloat4x4(&worldMatrix, XMMatrixIdentity());
	matrixDirty = false;
}

void Transform::MoveAbsolute(float x, float y, float z) {
	// Simple way
	//position.x += x;
	//position.y += y;
	//position.z += z;

	// Math library way
	XMVECTOR pos = XMLoadFloat3(&position);
	XMVECTOR offset = XMVectorSet(x, y, z, 0);
	XMStoreFloat3(&position, pos + offset);

	matrixDirty = true;
}

void Transform::Rotate(float p, float y, float r) {
	pitchYawRoll.x += p;
	pitchYawRoll.y += y;
	pitchYawRoll.z += r;

	matrixDirty = true;
}

void Transform::Scale(float x, float y, float z) {
	scale.x *= x;
	scale.y *= y;
	scale.z *= z;

	matrixDirty = true;
}

void Transform::SetPosition(float x, float y, float z) {
	position.x = x;
	position.y = y;
	position.z = z;

	matrixDirty = true;
}

void Transform::SetPitchYawRoll(float p, float y, float r) {
	pitchYawRoll.x = p;
	pitchYawRoll.y = y;
	pitchYawRoll.z = r;

	matrixDirty = true;
}

void Transform::Scale(float x, float y, float z) {
	scale.x = x;
	scale.y = y;
	scale.z = z;

	matrixDirty = true;
}

DirectX::XMFLOAT3 Transform::GetPosition() { return position; }
DirectX::XMFLOAT3 Transform::GetPitchYawRoll() { return pitchYawRoll; }
DirectX::XMFLOAT3 Transform::GeScale() { return scale; }

DirectX::XMFLOAT4X4 Transform::GetWorldMatrix() { 
	// Check if world matrix needs to be regenerated
	if (matrixDirty) {
		// Build the matrices representing each type of transform
		XMMATRIX trans = XMMatrixTranslation(position.x, position.y, position.z);
		// can also do XMMATRIX trans = XMMatrixTranslationFromVector(XMLoadFloat3(&position));
		XMMATRIX rot = XMMatrixRotationRollPitchYaw(pitchYawRoll.x, pitchYawRoll.y, pitchYawRoll.z);
		XMMATRIX sc = XMMatrixScaling(scale.x, scale.y, scale.z);

		// Combine them - use SRT
		XMMATRIX worldMat = sc * rot * trans;

		// Store as a storage type and return
		XMStoreFloat4x4(&worldMatrix, worldMat);
		// invert scale, but keep everything else the same (for normals and lighting)
		XMStoreFloat4x4(
			&worldInverseTransposeMatrix,
			XMMatrixInverse(0, XMMatrixTranspose(worldMat))
		);
		matrixDirty = false;
	}

	return worldMatrix;
}
