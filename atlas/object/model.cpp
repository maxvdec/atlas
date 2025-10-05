//
// model.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Model object implementation
// Copyright (c) 2025 Max Van den Eynde
//
#include <any>
#include <assimp/scene.h>
#include "atlas/core/shader.h"
#include "atlas/object.h"
#include "atlas/texture.h"
#include "atlas/units.h"
#include "atlas/window.h"
#include "atlas/workspace.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

void Model::fromResource(Resource resource) { loadModel(resource); }

void Model::loadModel(Resource resource) {
    Assimp::Importer importer;
    if (resource.type != ResourceType::Model)
        return;
    const aiScene *scene = importer.ReadFile(
        resource.path.string(),
        aiProcess_Triangulate | aiProcess_MakeLeftHanded |
            aiProcess_FlipWindingOrder | aiProcess_CalcTangentSpace);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
        !scene->mRootNode) {
        throw std::runtime_error("Assimp error: " +
                                 std::string(importer.GetErrorString()));
        // Error handling
        return;
    }
    directory = resource.path.parent_path().string();

    processNode(scene->mRootNode, scene);

    std::cout << "Created model from resource: " << resource.name << " with "
              << objects.size() << " objects." << std::endl;
    std::cout << "Model path: " << resource.path << std::endl;
    std::cout << "Model directory: " << directory << std::endl;
    std::cout << "First model object has "
              << (objects.size() > 0 ? objects[0]->getVertices().size() : 0)
              << " vertices." << std::endl;
}

void Model::processNode(aiNode *node, const aiScene *scene,
                        glm::mat4 parentTransform) {
    aiMatrix4x4 aiTrans = node->mTransformation;
    glm::mat4 nodeTransform = glm::mat4(
        aiTrans.a1, aiTrans.b1, aiTrans.c1, aiTrans.d1, aiTrans.a2, aiTrans.b2,
        aiTrans.c2, aiTrans.d2, aiTrans.a3, aiTrans.b3, aiTrans.c3, aiTrans.d3,
        aiTrans.a4, aiTrans.b4, aiTrans.c4, aiTrans.d4);

    glm::mat4 globalTransform = parentTransform * nodeTransform;
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        auto obj = std::make_shared<CoreObject>(processMesh(mesh, scene));
        glm::vec3 position, scale, skew;
        glm::vec4 perspective;
        glm::quat rotation;

        glm::decompose(globalTransform, scale, rotation, position, skew,
                       perspective);

        position /= scale;
        rotation = glm::normalize(rotation);

        obj->setPosition({position.x, position.y, position.z});
        obj->setRotation(Rotation3d::fromGlmQuat(rotation));
        obj->setShader(ShaderProgram::defaultProgram());
        obj->initialize();
        this->objects.push_back(obj);
    }
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }
}

CoreObject Model::processMesh(aiMesh *mesh, const aiScene *scene) {
    CoreObject object;
    std::vector<CoreVertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        CoreVertex vertex;
        vertex.position = {mesh->mVertices[i].x, mesh->mVertices[i].y,
                           mesh->mVertices[i].z};
        if (mesh->mNormals) {
            vertex.normal = {mesh->mNormals[i].x, mesh->mNormals[i].y,
                             mesh->mNormals[i].z};
        } else {
            vertex.normal = {0.0f, 0.0f, 0.0f};
        }
        if (mesh->mTextureCoords[0]) {
            vertex.textureCoordinate = {mesh->mTextureCoords[0][i].x,
                                        mesh->mTextureCoords[0][i].y};
        } else {
            vertex.textureCoordinate = {0.0f, 0.0f};
        }
        if (mesh->mTangents) {
            vertex.tangent = {mesh->mTangents[i].x, mesh->mTangents[i].y,
                              mesh->mTangents[i].z};
        } else {
            vertex.tangent = {0.0f, 0.0f, 0.0f};
        }
        if (mesh->mBitangents) {
            vertex.bitangent = {mesh->mBitangents[i].x, mesh->mBitangents[i].y,
                                mesh->mBitangents[i].z};
        } else {
            vertex.bitangent = {0.0f, 0.0f, 0.0f};
        }
        vertex.color = {1.0f, 1.0f, 1.0f, 1.0f};
        vertices.push_back(vertex);
    }
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }
    if (mesh->mMaterialIndex >= 0) {
        aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
        std::vector<Texture> diffuseMaps = loadMaterialTextures(
            material, (std::any)aiTextureType_DIFFUSE, "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        std::vector<Texture> specularMaps = loadMaterialTextures(
            material, (std::any)aiTextureType_SPECULAR, "texture_specular");
        textures.insert(textures.end(), specularMaps.begin(),
                        specularMaps.end());
    }
    object.attachVertices(vertices);
    object.attachIndices(indices);
    for (const Texture &tex : textures) {
        object.attachTexture(tex);
    }
    return object;
}

std::vector<Texture> Model::loadMaterialTextures(aiMaterial *mat, std::any type,
                                                 const std::string &typeName) {
    aiMaterial *material = mat;
    aiTextureType textureType = std::any_cast<aiTextureType>(type);
    std::vector<Texture> textures;
    for (unsigned int i = 0; i < material->GetTextureCount(textureType); i++) {
        aiString str;
        material->GetTexture(textureType, i, &str);
        std::cout << "Loading texture: " << str.C_Str() << " of type "
                  << typeName << std::endl;
        std::string filename = std::string(str.C_Str());
        std::string fullPath = directory + "/" + filename;

        ResourceType resType = ResourceType::Image;
        if (typeName == "texture_specular") {
            resType = ResourceType::SpecularMap;
        }
        Resource resource =
            Workspace::get().createResource(fullPath, filename, resType);
        Texture texture(resource);
        TextureType textureType = TextureType::Color;
        if (resType == ResourceType::SpecularMap)
            textureType = TextureType::Specular;

        texture.type = textureType;
        textures.push_back(texture);
    }
    return textures;
}
