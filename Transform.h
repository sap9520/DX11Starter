#pragma once

#include <DirectXMath.h>

class Transform
{
public:
	Transform(); // constructor

	// Offset methods - change/offset the data
	void MoveAbsolute(float x, float y, float z);
	void Rotate(float p, float y, float r);
	void Scale(float x, float y, float z);

	// Setters - overwrite the existing data
	void SetPosition(float x, float y, float z);
	void SetPitchYawRoll(float p, float y, float r);
	void SetScale(float x, float y, float z);

	// Getters - return existing data
	DirectX::XMFLOAT3 GetPosition();
	DirectX::XMFLOAT3 GetPitchYawRoll();
	DirectX::XMFLOAT3 GeScale();

	DirectX::XMFLOAT4X4 GetWorldMatrix();

private:
	// Raw transformation data
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 pitchYawRoll; // not a quaternion, just 3 different rotations
	DirectX::XMFLOAT3 scale;

	DirectX::XMFLOAT4X4 worldMatrix;
	DirectX::XMFLOAT4X4 worldInverseTransposeMatrix;

	bool matrixDirty;
};

