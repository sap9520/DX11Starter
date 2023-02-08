#include "Material.h"
#include "DX12Helper.h"

Material::Material(
	Microsoft::WRL::ComPtr<ID3D12PipelineState> _pipelineState,
	DirectX::XMFLOAT3 _colorTint,
	DirectX::XMFLOAT2 _uvScale,
	DirectX::XMFLOAT2 _uvOffset)
	:
	pipelineState(_pipelineState),
	colorTint(_colorTint),
	uvScale(_uvScale),
	uvOffset(_uvOffset),
	finalized(false)
{
	finalGPUHandleForSRVs = {};

}

// Getters
float Material::GetRoughness() { return roughness; }
DirectX::XMFLOAT3 Material::GetColorTint() { return colorTint; }
DirectX::XMFLOAT2 Material::GetUVScale() { return uvScale; }
DirectX::XMFLOAT2 Material::GetUVOffset() { return uvOffset; }
Microsoft::WRL::ComPtr<ID3D12PipelineState> Material::GetPipelineState() { return pipelineState; }
D3D12_GPU_DESCRIPTOR_HANDLE Material::GetFinalGPUHandle() { return finalGPUHandleForSRVs; }

// Setters
void Material::SetColorTint(DirectX::XMFLOAT3 _colorTint)
{
	colorTint = _colorTint;
}
void Material::SetUVScale(DirectX::XMFLOAT2 _uvScale)
{
	uvScale = _uvScale;
}
void Material::GetUVOffset(DirectX::XMFLOAT2 _uvOffset)
{
	uvOffset = _uvOffset;
}
void Material::SetPipelineState(Microsoft::WRL::ComPtr<ID3D12PipelineState> _pipelineState)
{
	pipelineState = _pipelineState;
}

void Material::AddTexture(D3D12_CPU_DESCRIPTOR_HANDLE srv, int slot)
{
	if (finalized || slot < 4)
		return;

	textureSRVsBySlot[slot] = srv;
}

void Material::FinalizeMaterial()
{
	if (finalized)
		return;

	DX12Helper& dx12Helper = DX12Helper::GetInstance();
	D3D12_GPU_DESCRIPTOR_HANDLE descHandle;

	for (int i = 0; i < 4; i++) {
		descHandle = dx12Helper.CopySRVsToDescriptorHeapAndGetGPUDescriptorHandle(textureSRVsBySlot[i], 1);

		if (i == 0)
			finalGPUHandleForSRVs = descHandle;
	}
}
