/*
 main.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: The main entry point for the Atlas Test
 Copyright (c) 2025 maxvdec
*/

#include "atlas/camera.hpp"
#include "atlas/light.hpp"
#include "atlas/material.hpp"
#include "atlas/scene.hpp"
#include "atlas/texture.hpp"
#include "atlas/units.hpp"
#include "atlas/workspace.hpp"
#include <atlas/core/rendering.hpp>
#include <atlas/window.hpp>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>

class MainScene : public Scene {
  public:
    PointLight light;
    CoreObject object;
    Camera camera;
    void init() override {
        Workspace workspace(TEST_PATH);
        Resource textureResource = workspace.loadResource("container.png");
        Texture texture;
        texture.fromImage(textureResource);

        Texture specular;
        specular.fromImage(workspace.loadResource("specular.png"));

        camera.position = Position3d(0.0f, 0.0f, -3.0f);
        camera.useCamera();

        light =
            PointLight(Position3d(-0.3f, -2.0f, 0.2f), Color(1.0f, 1.0f, 1.0f));
        light.debugLight();
        light.intensity = 1.0f;

        object = generateCubeObject(Position3d(0.0f, 0.0f, 0.0f),
                                    Size3d(1.0f, 1.0f, 1.0f));

        std::vector<Size2d> faceUVs = {
            {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};

        std::vector<Size2d> allUVs;
        for (int i = 0; i < 6; ++i) {
            allUVs.insert(allUVs.end(), faceUVs.begin(), faceUVs.end());
        }

        object.provideTextureCoords(allUVs);

        std::vector<Size3d> normals = {
            // Front face
            {0.0f, 0.0f, 1.0f},
            {0.0f, 0.0f, 1.0f},
            {0.0f, 0.0f, 1.0f},
            {0.0f, 0.0f, 1.0f},
            // Back face
            {0.0f, 0.0f, -1.0f},
            {0.0f, 0.0f, -1.0f},
            {0.0f, 0.0f, -1.0f},
            {0.0f, 0.0f, -1.0f},
            // Left face
            {-1.0f, 0.0f, 0.0f},
            {-1.0f, 0.0f, 0.0f},
            {-1.0f, 0.0f, 0.0f},
            {-1.0f, 0.0f, 0.0f},
            // Right face
            {1.0f, 0.0f, 0.0f},
            {1.0f, 0.0f, 0.0f},
            {1.0f, 0.0f, 0.0f},
            {1.0f, 0.0f, 0.0f},
            // Top face
            {0.0f, 1.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
            // Bottom face
            {0.0f, -1.0f, 0.0f},
            {0.0f, -1.0f, 0.0f},
            {0.0f, -1.0f, 0.0f},
            {0.0f, -1.0f, 0.0f},
        };

        object.provideNormals(normals);

        object.setTexture(texture);
        object.setSpecularMap(specular);

        std::vector<uint32_t> indices;
        for (int i = 0; i < 6; ++i) {
            uint32_t start = i * 4;
            indices.push_back(start + 0);
            indices.push_back(start + 1);
            indices.push_back(start + 2);
            indices.push_back(start + 2);
            indices.push_back(start + 3);
            indices.push_back(start + 0);
        }
        object.provideIndexedDrawing(indices);
        object.show();

        CoreObject copy = object.copy();
        object.initialize();
    }
    void update(float deltaTime) override {}
};

int main() {
    Window mywin = Window("Atlas Test", Frame(1500, 800));
    mywin.backgroundColor = Color(0.2f, 0.2f, 0.2f, 1.0f);
    MainScene *mainScene = new MainScene();
    mywin.currentScene = mainScene;

    mywin.run();
    return 0;
}
