#pragma once

#include "Lights.h"
#include <DirectXMath.h>

struct VertexShaderExternalData
{
	DirectX::XMFLOAT4X4 world;
	DirectX::XMFLOAT4X4 worldInvTranspose;
	DirectX::XMFLOAT4X4 view;
	DirectX::XMFLOAT4X4 projection;
};

struct PixelShaderExternalData
{
	DirectX::XMFLOAT2 uvScale;
	DirectX::XMFLOAT2 uvOffset;
	DirectX::XMFLOAT3 cameraPos;
	int lightCount;
	Light lights[NUM_LIGHTS];
};