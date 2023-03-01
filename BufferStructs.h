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

struct RaytracingSceneData
{
	DirectX::XMFLOAT4X4 inverseViewProjection;
	DirectX::XMFLOAT3 cameraPosition;
};

#define MAX_INSTANCES_PER_BLAS 100
struct RaytracingEntityData
{
	DirectX::XMFLOAT4 color[MAX_INSTANCES_PER_BLAS];
};