#pragma once

#include "Vertex.h"
#include <d3d11.h>
#include <wrl/client.h>
class Mesh
{
public:
	Mesh(
		Vertex* objArray,
		int numVertices,
		unsigned int* indices,
		int numIndices,
		Microsoft::WRL::ComPtr<ID3D11Device> bufferCreator);
	Mesh(const wchar_t* filename, 
		Microsoft::WRL::ComPtr<ID3D11Device> bufferCreator);
	~Mesh();

	Microsoft::WRL::ComPtr<ID3D11Buffer> GetVertexBuffer();
	Microsoft::WRL::ComPtr<ID3D11Buffer> GetIndexBuffer();
	int GetIndexCount();
	void Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context);
	void CalculateTangents(Vertex* verts, int numVerts, unsigned int* indices, int numIndices);

private:
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
	int numIndices;

	void SetBufferData(Vertex* objArray,
		int numVertices, 
		unsigned int* indices,
		int numIndices,
		Microsoft::WRL::ComPtr<ID3D11Device> bufferCreator);
};

