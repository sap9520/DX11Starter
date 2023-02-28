#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <string>

#include "Vertex.h"

struct MeshRaytracingData
{
	D3D12_GPU_DESCRIPTOR_HANDLE IndexbufferSRV {};
	D3D12_GPU_DESCRIPTOR_HANDLE VertexBufferSRV {};
	Microsoft::WRL::ComPtr<ID3D12Resource> BLAS;
	unsigned int HitGroupIndex = 0;
};

class Mesh
{
public:
	Mesh(Vertex* vertArray, int numVerts, unsigned int* indexArray, int numIndices);
	Mesh(const wchar_t* objFile);
	~Mesh();

	// Getters for mesh data
	D3D12_VERTEX_BUFFER_VIEW GetVBView() { return vbView; }
	D3D12_INDEX_BUFFER_VIEW GetIBView() { return ibView; }

	Microsoft::WRL::ComPtr<ID3D12Resource> GetVBResource() { return vertexBuffer; }
	Microsoft::WRL::ComPtr<ID3D12Resource> GetIBResource() { return indexBuffer; }

	MeshRaytracingData GetRaytracingData() { return raytracingData; }

	unsigned int GetIndexCount();
	unsigned int GetVertexCount();


private:
	// D3D buffers
	D3D12_VERTEX_BUFFER_VIEW vbView;
	D3D12_INDEX_BUFFER_VIEW ibView;

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer;

	MeshRaytracingData raytracingData;

	// Total indices in this mesh
	unsigned int numIndices;
	unsigned int numVertices;

	// Helper for creating buffers (in the event we add more constructor overloads)
	void CreateBuffers(Vertex* vertArray, size_t numVerts, unsigned int* indexArray, size_t numIndices);
	void CalculateTangents(Vertex* verts, size_t numVerts, unsigned int* indices, size_t numIndices);
};