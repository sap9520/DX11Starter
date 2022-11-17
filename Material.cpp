#include "Material.h"

Material::Material(
	DirectX::XMFLOAT4 _tint,
	std::shared_ptr<SimpleVertexShader> _vs,
	std::shared_ptr<SimplePixelShader> _ps,
	float _roughness)
	:
	tint(_tint),
	vs(_vs),
	ps(_ps),
	roughness(_roughness)
{

}

DirectX::XMFLOAT4 Material::GetColorTint() { return tint; }

float Material::GetRoughness() { return roughness; }

std::shared_ptr<SimpleVertexShader> Material::GetVertexShader() { return vs; }

std::shared_ptr<SimplePixelShader> Material::GetPixelShader() { return ps; }

void Material::SetColorTint(DirectX::XMFLOAT4 _tint)
{
	tint = _tint;
}

void Material::SetVertexShader(std::shared_ptr<SimpleVertexShader> _vs)
{
	vs = _vs;
}

void Material::SetPixelShader(std::shared_ptr<SimplePixelShader> _ps)
{
	ps = _ps;
}

void Material::PrepareMaterial() {
	for (auto& t : textureSRVs) { ps->SetShaderResourceView(t.first.c_str(), t.second); }
	for (auto& s : samplers) { ps->SetSamplerState(s.first.c_str(), s.second); }
}

void Material::AddTextureSRV(std::string shaderName, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv) {
	textureSRVs.insert({ shaderName, srv });
}

void Material::AddSampler(std::string samplerName, Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler) {
	samplers.insert({ samplerName, sampler });
}
