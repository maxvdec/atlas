//
// model.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Model object implementation
// Copyright (c) 2025 Max Van den Eynde
//

#include "atlas/object.h"
#include "atlas/workspace.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

void Model::fromResource(Resource resource) { loadModel(resource); }

void Model::loadModel(Resource resource) {
    Assimp::Importer importer;
    if (resource.type != ResourceType::Model)
        return;
    const aiScene *scene = importer.ReadFile(
        resource.path.string(), aiProcess_Triangulate | aiProcess_FlipUVs);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
        !scene->mRootNode) {
        // Error handling
        return;
    }
    directory = resource.path.parent_path().string();

    if (this->object == nullptr)
        this->object = std::make_shared<CompoundObject>();

    processNode(scene->mRootNode, scene);
}