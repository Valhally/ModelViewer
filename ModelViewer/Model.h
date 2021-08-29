#pragma once

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "assimp/Exporter.hpp"

#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include "IndexBuffer.h"

#pragma comment(lib, "assimp-vc140-mt.lib")

struct Point
{
	float min, max;
	Point() { min = 1e30; max = -1e30; }
	Point(float x, float y)
	{
		this->min = x;
		this->max = y;
	}
	inline float Middle()
	{
		return (min + max) * 0.5;
	}
};

const std::string modelTypeList[] = {
	"*.obj",
	"*.fbx",
	"*.3d",
	"*.blend",
	"*.dae",
	"*.glTF",
	"*.dxf",
	"*.m3d",
	"*.ogex",
	"*.raw",
	"*.x",
	"*.smd",
	"*.x3d"
};

class Model
{
public:
	std::shared_ptr<VertexBuffer> vertexBuffer;
	std::shared_ptr<VertexBuffer> solidVertexBuffer;
	std::vector<std::shared_ptr<IndexBuffer>> indexBuffers;
	std::vector<std::shared_ptr<IndexBuffer>> lineIndexBuffers;

	std::vector<Vertex> vertices;
	std::vector<Vertex> solidVertices;
	std::vector<UINT32> indices;
	std::vector<UINT32> lineIndices;

	ComPtr<ID3D12Device4> device;
	ComPtr<ID3D12GraphicsCommandList> cmdList;

	int faceCount;

public:
	Point lr[3];
	static constexpr double maxLength = 800;
	double scale = 1;
	std::string modelFileName;

	Model(
		std::shared_ptr<VertexBuffer> vb, 
		std::shared_ptr<IndexBuffer> ib, 
		ComPtr<ID3D12Device4> device, 
		ComPtr<ID3D12GraphicsCommandList> cmdList)
	{
		this->device = device;
		this->cmdList = cmdList;

		vertexBuffer = vb;
		lineIndexBuffers.push_back(ib);
	}

	Model(std::string fileName, ComPtr<ID3D12Device4> device, ComPtr<ID3D12GraphicsCommandList> cmdList)
		: device(device), cmdList(cmdList)
	{
		std::cout << "Model input:" << fileName << std::endl;
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(fileName, aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_ConvertToLeftHanded);
		modelFileName = fileName;

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
			return;
		}

		processNode(scene->mRootNode, scene);

		vertexBuffer = std::make_shared<VertexBuffer>(
			device, cmdList, vertices.data(), sizeof(Vertex), vertices.size() );

#pragma omp parallel for
		for(int i = 0; i < vertices.size(); ++i)
		{
			lr[0].min = min(lr[0].min, vertices[i].position.x);
			lr[0].max = max(lr[0].max, vertices[i].position.x);

			lr[1].min = min(lr[1].min, vertices[i].position.y);
			lr[1].max = max(lr[1].max, vertices[i].position.y);

			lr[2].min = min(lr[2].min, vertices[i].position.z);
			lr[2].max = max(lr[2].max, vertices[i].position.z);
		}

		scale = -1e30;
		for(int i = 0; i < 3; ++i) scale = max(scale, lr[i].max - lr[i].min);
		scale = maxLength / scale;

		for(int i = 2; i < solidVertices.size(); i += 3)
		{
			XMVECTOR p1{solidVertices[i - 2].position.x, solidVertices[i - 2].position.y, solidVertices[i - 2].position.z };
			XMVECTOR p2{solidVertices[i - 1].position.x, solidVertices[i - 1].position.y, solidVertices[i - 1].position.z };
			XMVECTOR p3{solidVertices[i - 0].position.x, solidVertices[i - 0].position.y, solidVertices[i - 0].position.z };

			XMVECTOR v1 = p3 - p1;
			XMVECTOR v2 = p2 - p1;
			XMVECTOR nor = XMVector3Cross(v2, v1);

			XMFLOAT3 normal;
			XMStoreFloat3(&normal, nor);
			solidVertices[i - 2].normal = normal;
			solidVertices[i - 1].normal = normal;
			solidVertices[i - 0].normal = normal;
		}
		solidVertexBuffer = std::make_shared<VertexBuffer>(
			device, cmdList, solidVertices.data(), sizeof(Vertex), solidVertices.size() );

		printf("Model:  %d vertices\n", vertices.size());
		faceCount = 0;
		for(auto& buffer : indexBuffers) faceCount += buffer->indexCount / 3;
	}

	XMMATRIX getModel()
	{
		return XMMatrixScaling(scale, scale, scale);
	}

	void Draw(D3D12_PRIMITIVE_TOPOLOGY primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST)
	{
		cmdList->IASetPrimitiveTopology(primitiveType);
		vertexBuffer->Bind(cmdList);
		if(primitiveType == D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST)
		{
			for(auto& indexBuffer : indexBuffers)
			{
				indexBuffer->Draw(cmdList);
			}
		}
		else if(primitiveType == D3D_PRIMITIVE_TOPOLOGY_LINELIST)
		{
			for(auto& indexBuffer : lineIndexBuffers)
			{
				indexBuffer->Draw(cmdList);
			}
		}
		else if(primitiveType == D3D_PRIMITIVE_TOPOLOGY_LINESTRIP)
		{
			cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			solidVertexBuffer->Draw(cmdList);
		}
		else
		{
			vertexBuffer->Draw(cmdList);
		}
		
	}

	void DrawGrid(XMFLOAT3 axisFlag)
	{
		cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
		vertexBuffer->Bind(cmdList);
		if(axisFlag.x > 0.5) lineIndexBuffers[0]->Draw(cmdList);
		if(axisFlag.y > 0.5) lineIndexBuffers[1]->Draw(cmdList);
		if(axisFlag.z > 0.5) lineIndexBuffers[2]->Draw(cmdList);
		lineIndexBuffers[3]->Draw(cmdList);
	}

	static std::string GetReadFileTypeList()
	{
		std::string res = "3D Model(" + modelTypeList[0];
		for(int i = 1; i < _countof(modelTypeList); ++i)
			res += " " + modelTypeList[i];
		res += ")";
		return res;
	}

	void saveModel(std::string fileName)
	{
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(modelFileName.c_str(), 0);
		Assimp::Exporter exporter;
		exporter.Export(scene, "obj", fileName);
	}

private:
	void processNode(aiNode* node, const aiScene* scene)
	{
		for(int i = 0; i < node->mNumMeshes; ++i) processMesh(scene->mMeshes[node->mMeshes[i]], scene);
		for(int i = 0; i < node->mNumChildren; ++i) processNode(node->mChildren[i], scene);
	}

	void processMesh(aiMesh* mesh, const aiScene* scene)
	{
		UINT startLocation = vertices.size();
		UINT indexOffset = indices.size();
		UINT lineIndexOffset = lineIndices.size();
		for(int i = 0; i < mesh->mNumVertices; ++i)
		{
			Vertex vertex{
				{
					mesh->mVertices[i].x,
					mesh->mVertices[i].y,
					mesh->mVertices[i].z
				},
				{
					mesh->mNormals[i].x,
					mesh->mNormals[i].y,
					mesh->mNormals[i].z
				}
			};
			if(mesh->mColors[0]) vertex.color = {mesh->mColors[i][0].r,mesh->mColors[i][0].g,mesh->mColors[i][0].b};
			vertices.push_back(vertex);
		}

		for(int i = 0; i < mesh->mNumFaces; ++i)
		{
			aiFace face = mesh->mFaces[i];
			for(int j = 0; j < face.mNumIndices; ++j) {
				indices.push_back(face.mIndices[j]);
				solidVertices.push_back(vertices[face.mIndices[j] + startLocation]);
			}

			for(int j = 0; j < face.mNumIndices; ++j)
				lineIndices.push_back(face.mIndices[j]),
				lineIndices.push_back(face.mIndices[(j + 1) % face.mNumIndices]);
		}

		indexBuffers.push_back(
			std::make_shared<IndexBuffer>(
				device, 
				cmdList, 
				indices.data() + indexOffset, 
				indices.size() - indexOffset, 
				DXGI_FORMAT_R32_UINT, 
				startLocation )
		);

		lineIndexBuffers.push_back(
			std::make_shared<IndexBuffer>(
				device,
				cmdList,
				lineIndices.data() + lineIndexOffset,
				lineIndices.size() - lineIndexOffset,
				DXGI_FORMAT_R32_UINT,
				startLocation )
		);
	}
};