/*
 model.hpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Model functions and implementations
 Copyright (c) 2025 maxvdec
*/

#ifndef ATLAS_MODEL_HPP
#define ATLAS_MODEL_HPP

#include "atlas/core/rendering.hpp"
#include "atlas/texture.hpp"
#include "atlas/workspace.hpp"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <string>
#include <vector>

class Model {
  public:
    Resource resource;
    std::vector<CoreObject> objects;

    Model(Resource resc);
    Model();

    inline void initialize() {
        for (auto &object : objects) {
            object.initialize();
        }
    }

  private:
    std::string directory;

    void loadModel(Resource resc);
    void processNode(aiNode *node, const aiScene *scene);
    CoreObject processMesh(aiMesh *mesh, const aiScene *scene);
    std::vector<Texture> loadMaterialTextures(aiMaterial *mat,
                                              aiTextureType type,
                                              TextureType textType);
    std::vector<Texture> loadedTextures;
};

#endif // ATLAS_MODEL_HPP
