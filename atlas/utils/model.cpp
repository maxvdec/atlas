/*
 model.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Model functions implementation
 Copyright (c) 2025 maxvdec
*/

#include "atlas/model.hpp"
#include "atlas/core/rendering.hpp"
#include "atlas/workspace.hpp"
#include <iostream>
#include <string>
#include <vector>

Model::Model(Resource resc) {
    this->resource = resc;
    this->directory = fs::path(resc.path).parent_path().string();
    loadModel(resc);
}

Model::Model() {}

void Model::loadModel(Resource resc) {
    Assimp::Importer importer;
    const aiScene *scene =
        importer.ReadFile(resc.path, aiProcess_Triangulate | aiProcess_FlipUVs |
                                         aiProcess_CalcTangentSpace |
                                         aiProcess_JoinIdenticalVertices);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
        !scene->mRootNode) {
        throw std::runtime_error("Failed to load model: " + resc.path);
    }

    processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode *node, const aiScene *scene) {
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        objects.push_back(processMesh(mesh, scene));
    }
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }
}

CoreObject Model::processMesh(aiMesh *mesh, const aiScene *scene) {
    CoreObject coreObject;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        CoreVertex vertex;
        vertex.x = mesh->mVertices[i].x;
        vertex.y = mesh->mVertices[i].y;
        vertex.z = mesh->mVertices[i].z;

        vertex.color = Color(0.79f, 0.79f, 0.79f, 1.f);

        vertex.textCoords.width = mesh->mTextureCoords[0][i].x;
        vertex.textCoords.height = mesh->mTextureCoords[0][i].y;

        vertex.normal.width = mesh->mNormals[i].x;
        vertex.normal.height = mesh->mNormals[i].y;
        vertex.normal.depth = mesh->mNormals[i].z;

        coreObject.vertices.push_back(vertex);
    }

    std::vector<unsigned int> indices;
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    coreObject.provideIndexedDrawing(indices);

    for (unsigned int i = 0; i < mesh->mMaterialIndex; i++) {
        aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
        std::vector<Texture> textures = loadMaterialTextures(
            material, aiTextureType_DIFFUSE, TextureType::Color);
        for (const auto &texture : textures) {
            coreObject.addTexture(texture);
        }
        std::vector<Texture> specularTextures = loadMaterialTextures(
            material, aiTextureType_SPECULAR, TextureType::Specular);
        for (const auto &texture : specularTextures) {
            coreObject.addTexture(texture);
        }
    }

    return coreObject;
}

std::vector<Texture> Model::loadMaterialTextures(aiMaterial *mat,
                                                 aiTextureType type,
                                                 TextureType textType) {
    std::vector<Texture> textures;
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
        aiString str;
        mat->GetTexture(type, i, &str);
        std::string texturePath = directory + "/" + str.C_Str();
        bool skip = false;
        for (unsigned int j = 0; j < loadedTextures.size(); j++) {
            if (loadedTextures[j].image.path == texturePath) {
                textures.push_back(loadedTextures[j]);
                skip = true;
                break;
            }
        }
        if (skip) {
            continue;
        }

        Texture texture;
        texture.fromImage(Resource(texturePath), textType);
        textures.push_back(texture);
        loadedTextures.push_back(texture);
    }
    return textures;
}
