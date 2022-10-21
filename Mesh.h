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
		Microsoft::WRL::ComPtr<ID3D11Device> bufferCreator,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> bufferActivator);
	Mesh(const wchar_t* filename, 
		Microsoft::WRL::ComPtr<ID3D11Device> bufferCreator,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context);
	~Mesh();

	Microsoft::WRL::ComPtr<ID3D11Buffer> GetVertexBuffer();
	Microsoft::WRL::ComPtr<ID3D11Buffer> GetIndexBuffer();
	int GetIndexCount();
	void Draw();

private:
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext>	context;
	int numIndices;

	void SetBufferData(Vertex* objArray,
		int numVertices, 
		unsigned int* indices,
		int numIndices,
		Microsoft::WRL::ComPtr<ID3D11Device> bufferCreator);
};

