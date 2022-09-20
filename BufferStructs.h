#pragma once

#include <DirectXMath.h>

class BufferStructs
{
};

struct VertexShaderExternalData
{
	DirectX::XMFLOAT4 colorTint;
	DirectX::XMFLOAT4X4 worldMatrix;
};