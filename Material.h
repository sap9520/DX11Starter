#pragma once

#include <DirectXMath.h>
#include <memory>
#include <unordered_map>
#include <wrl/client.h>
#include <d3d12.h>

class Material
{
public:
	Material(
		Microsoft::WRL::ComPtr<ID3D12PipelineState> _pipelineState,
		DirectX::XMFLOAT3 _colorTint,
		DirectX::XMFLOAT2 _uvScale,
		DirectX::XMFLOAT2 _uvOffset);

	float GetRoughness();
	DirectX::XMFLOAT3 GetColorTint();
	DirectX::XMFLOAT2 GetUVScale();
	DirectX::XMFLOAT2 GetUVOffset();
	Microsoft::WRL::ComPtr<ID3D12PipelineState> GetPipelineState();
	D3D12_GPU_DESCRIPTOR_HANDLE GetFinalGPUHandle();

	void SetColorTint(DirectX::XMFLOAT3 _colorTint);
	void SetUVScale(DirectX::XMFLOAT2 _uvScale);
	void GetUVOffset(DirectX::XMFLOAT2 _uvOffset);
	void SetPipelineState(Microsoft::WRL::ComPtr<ID3D12PipelineState> _pipelineState);

	void AddTexture(D3D12_CPU_DESCRIPTOR_HANDLE srv, int slot);
	void FinalizeMaterial();

private:
	float roughness;

	DirectX::XMFLOAT3 colorTint;
	DirectX::XMFLOAT2 uvScale;
	DirectX::XMFLOAT2 uvOffset;
	bool finalized;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState; // replaces indiv vertex and pixel shaders
	D3D12_CPU_DESCRIPTOR_HANDLE textureSRVsBySlot[4];
	D3D12_GPU_DESCRIPTOR_HANDLE finalGPUHandleForSRVs;
};