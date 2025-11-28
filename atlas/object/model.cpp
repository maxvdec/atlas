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
#include "opal/opal.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/epsilon.hpp>

void Model::fromResource(Resource resource) { loadModel(resource); }

void Model::loadModel(Resource resource) {
    Assimp::Importer importer;
    if (resource.type != ResourceType::Model)
        return;
    const aiScene *scene = importer.ReadFile(
        resource.path.string(),
        aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_FlipUVs |
            aiProcess_JoinIdenticalVertices | aiProcess_ImproveCacheLocality |
            aiProcess_SortByPType | aiProcess_GenSmoothNormals);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
        !scene->mRootNode) {
        throw std::runtime_error("Assimp error: " +
                                 std::string(importer.GetErrorString()));
        // Error handling
        return;
    }
    directory = resource.path.parent_path().string();

    // Create a single shared pipeline for all meshes in this model
    auto sharedPipeline = opal::Pipeline::create();
    sharedPipeline =
        ShaderProgram::defaultProgram().requestPipeline(sharedPipeline);

    // Texture cache to avoid loading the same texture multiple times
    std::unordered_map<std::string, Texture> textureCache;

    processNode(scene->mRootNode, scene, glm::mat4(1.0f), sharedPipeline,
                textureCache);

    std::cout << "Created model from resource: " << resource.name << " with "
              << objects.size() << " objects." << std::endl;
    std::cout << "Model path: " << resource.path << std::endl;
    std::cout << "Model directory: " << directory << std::endl;
    std::cout << "First model object has "
              << (objects.size() > 0 ? objects[0]->getVertices().size() : 0)
              << " vertices." << std::endl;

    std::cout << "Model loaded successfully." << std::endl;
    std::cout << "Meshes: " << scene->mNumMeshes << std::endl;
    std::cout << "Unique textures loaded: " << textureCache.size() << std::endl;

    size_t totalVertices = 0;
    size_t totalTriangles = 0;
    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
        aiMesh *mesh = scene->mMeshes[i];
        totalVertices += mesh->mNumVertices;
        totalTriangles +=
            mesh->mNumFaces; // each face is a triangle (after Triangulate)
    }

    std::cout << "Total Vertices: " << totalVertices << std::endl;
    std::cout << "Total Triangles: " << totalTriangles << std::endl;
}

void Model::processNode(
    aiNode *node, const aiScene *scene, glm::mat4 parentTransform,
    std::shared_ptr<opal::Pipeline> sharedPipeline,
    std::unordered_map<std::string, Texture> &textureCache) {
    glm::mat4 nodeTransform =
        parentTransform * glm::make_mat4(&node->mTransformation.a1);

    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        auto obj = std::make_shared<CoreObject>(
            processMesh(mesh, scene, nodeTransform, textureCache));
        // Reuse the shared pipeline instead of creating a new one per mesh
        obj->setPipeline(sharedPipeline);
        obj->initialize();
        this->objects.push_back(obj);
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene, nodeTransform, sharedPipeline,
                    textureCache);
    }
}

CoreObject
Model::processMesh(aiMesh *mesh, const aiScene *scene,
                   const glm::mat4 &transform,
                   std::unordered_map<std::string, Texture> &textureCache) {
    CoreObject object;
    std::vector<CoreVertex> vertices;
    std::vector<unsigned int> indices;

    vertices.reserve(mesh->mNumVertices);

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        glm::vec4 pos =
            transform * glm::vec4(mesh->mVertices[i].x, mesh->mVertices[i].y,
                                  mesh->mVertices[i].z, 1.0f);

        CoreVertex vertex;
        vertex.position = Position3d::fromGlm(glm::vec3(pos));

        // ---------- Normals ----------
        if (mesh->mNormals) {
            glm::vec3 normal =
                glm::mat3(transform) * glm::vec3(mesh->mNormals[i].x,
                                                 mesh->mNormals[i].y,
                                                 mesh->mNormals[i].z);
            if (glm::length(normal) < 1e-6f) {
                normal = glm::vec3(0.0f, 1.0f, 0.0f);
            } else {
                normal = glm::normalize(normal);
            }
            vertex.normal = Normal3d::fromGlm(normal);
        } else {
            vertex.normal = Normal3d::fromGlm(glm::vec3(0.0f, 1.0f, 0.0f));
        }

        // ---------- Tangents ----------
        if (mesh->mTangents) {
            glm::vec3 tangent =
                glm::mat3(transform) * glm::vec3(mesh->mTangents[i].x,
                                                 mesh->mTangents[i].y,
                                                 mesh->mTangents[i].z);
            if (glm::length(tangent) < 1e-6f)
                tangent = glm::vec3(1.0f, 0.0f, 0.0f);
            vertex.tangent = Normal3d::fromGlm(glm::normalize(tangent));
        }

        // ---------- Texture Coordinates ----------
        if (mesh->mTextureCoords[0]) {
            glm::vec2 uv = glm::vec2(mesh->mTextureCoords[0][i].x,
                                     mesh->mTextureCoords[0][i].y);
            if (glm::all(glm::isnan(uv)) || glm::any(glm::isinf(uv)))
                uv = glm::vec2(0.0f, 0.0f);
            vertex.textureCoordinate = TextureCoordinate{uv.x, uv.y};
        } else {
            vertex.textureCoordinate = TextureCoordinate{0.0f, 0.0f};
        }

        // ---------- Vertex Color ----------
        if (mesh->mColors[0]) {
            glm::vec4 col =
                glm::vec4(mesh->mColors[0][i].r, mesh->mColors[0][i].g,
                          mesh->mColors[0][i].b, mesh->mColors[0][i].a);
            if (glm::all(glm::isnan(col)) || glm::any(glm::isinf(col)))
                col = glm::vec4(1.0f);
            vertex.color = Color{col.r, col.g, col.b, col.a};
        } else {
            vertex.color = Color{1.0f, 1.0f, 1.0f, 1.0f};
        }

        vertices.push_back(vertex);
    }

    // ---------- Indices ----------
    // Pre-calculate total indices for reservation
    size_t totalIndices = 0;
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        totalIndices += mesh->mFaces[i].mNumIndices;
    }
    indices.reserve(totalIndices);

    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        const aiFace &face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    std::vector<Texture> textures;
    if (mesh->mMaterialIndex >= 0) {
        aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];

        auto diffuseMaps =
            loadMaterialTextures(material, std::any(aiTextureType_DIFFUSE),
                                 "texture_diffuse", textureCache);
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

        auto specularMaps =
            loadMaterialTextures(material, std::any(aiTextureType_SPECULAR),
                                 "texture_specular", textureCache);
        textures.insert(textures.end(), specularMaps.begin(),
                        specularMaps.end());

        auto normalMaps =
            loadMaterialTextures(material, std::any(aiTextureType_NORMALS),
                                 "texture_normal", textureCache);
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

        auto heightMaps =
            loadMaterialTextures(material, std::any(aiTextureType_HEIGHT),
                                 "texture_height", textureCache);
        textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());
    }

    if (!textures.empty()) {
        for (const auto &texture : textures) {
            object.attachTexture(texture);
        }
    }

    object.attachVertices(vertices);
    object.attachIndices(indices);
    return object;
}

std::vector<Texture> Model::loadMaterialTextures(
    aiMaterial *mat, std::any type, const std::string &typeName,
    std::unordered_map<std::string, Texture> &textureCache) {
    aiMaterial *material = mat;
    aiTextureType textureType = std::any_cast<aiTextureType>(type);
    std::vector<Texture> textures;
    for (unsigned int i = 0; i < material->GetTextureCount(textureType); i++) {
        aiString str;
        material->GetTexture(textureType, i, &str);
        std::string filename = std::string(str.C_Str());
        std::string fullPath = directory + "/" + filename;

        // Check if texture is already cached
        auto cacheIt = textureCache.find(fullPath);
        if (cacheIt != textureCache.end()) {
            textures.push_back(cacheIt->second);
            continue;
        }

        std::cout << "Loading texture: " << str.C_Str() << " of type "
                  << typeName << std::endl;

        ResourceType resType = ResourceType::Image;
        if (typeName == "texture_specular") {
            resType = ResourceType::SpecularMap;
        }
        Resource resource =
            Workspace::get().createResource(fullPath, filename, resType);
        TextureType texType = TextureType::Color;
        if (typeName == "texture_specular") {
            texType = TextureType::Specular;
        } else if (typeName == "texture_normal") {
            texType = TextureType::Normal;
        } else if (typeName == "texture_height") {
            texType = TextureType::Parallax;
        }

        try {
            Texture loadedTexture = Texture::fromResource(resource, texType);
            textureCache[fullPath] = loadedTexture;
            textures.push_back(loadedTexture);
        } catch (const std::exception &ex) {
            std::cerr << "Failed to load texture '" << filename
                      << "': " << ex.what() << std::endl;
        }
    }
    return textures;
}
