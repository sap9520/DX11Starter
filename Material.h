#pragma once

#include "SimpleShader.h"
#include <DirectXMath.h>
#include <memory>

class Material
{
public:
	Material(
		DirectX::XMFLOAT4 _tint, 
		std::shared_ptr<SimpleVertexShader> _vs, 
		std::shared_ptr<SimplePixelShader> _ps,
		float _roughness);

	DirectX::XMFLOAT4 GetColorTint();
	float GetRoughness();
	std::shared_ptr<SimpleVertexShader> GetVertexShader();
	std::shared_ptr<SimplePixelShader> GetPixelShader();

	void SetColorTint(DirectX::XMFLOAT4 _tint);
	void SetVertexShader(std::shared_ptr<SimpleVertexShader> _vs);
	void SetPixelShader(std::shared_ptr<SimplePixelShader> _ps);

private:
	DirectX::XMFLOAT4 tint;
	std::shared_ptr<SimpleVertexShader> vs;
	std::shared_ptr<SimplePixelShader> ps;
	float roughness;
};

