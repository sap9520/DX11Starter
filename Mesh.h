#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <string>

#include "Vertex.h"


class Mesh
{
public:
	Mesh(Vertex* vertArray, int numVerts, unsigned int* indexArray, int numIndices);
	Mesh(const wchar_t* objFile);
	~Mesh();

	// Getters for mesh data
	D3D12_VERTEX_BUFFER_VIEW GetVertexBuffer() { return vbView; }
	D3D12_INDEX_BUFFER_VIEW GetIndexBuffer() { return ibView; }
	unsigned int GetIndexCount();

private:
	// D3D buffers
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW vbView;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer;
	D3D12_INDEX_BUFFER_VIEW ibView;

	// Total indices in this mesh
	unsigned int numIndices;

	// Helper for creating buffers (in the event we add more constructor overloads)
	void CreateBuffers(Vertex* vertArray, size_t numVerts, unsigned int* indexArray, size_t numIndices);
	void CalculateTangents(Vertex* verts, size_t numVerts, unsigned int* indices, size_t numIndices);
};