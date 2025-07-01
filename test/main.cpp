/*
 main.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: The main entry point for the Atlas Test
 Copyright (c) 2025 maxvdec
*/

#include "atlas/camera.hpp"
#include "atlas/input.hpp"
#include "atlas/light.hpp"
#include "atlas/material.hpp"
#include "atlas/model.hpp"
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
    PointLight *light = nullptr;
    DirectionalLight *sun = nullptr;
    CoreObject object;
    Camera camera;
    Model model;
    RenderTarget renderTarget;
    Skybox skybox;
    void init() override {
        Workspace workspace(TEST_PATH);

        Texture texture;
        texture.fromImage(workspace.loadResource("container.jpg"),
                          TextureType::Color);

        CubemapPacket cubemapPacket;
        cubemapPacket.right = workspace.loadResource("skybox/right.jpg");
        cubemapPacket.left = workspace.loadResource("skybox/left.jpg");
        cubemapPacket.top = workspace.loadResource("skybox/top.jpg");
        cubemapPacket.bottom = workspace.loadResource("skybox/bottom.jpg");
        cubemapPacket.front = workspace.loadResource("skybox/front.jpg");
        cubemapPacket.back = workspace.loadResource("skybox/back.jpg");
        Cubemap cubemap;
        cubemap.fromImages(cubemapPacket, TextureType::Cubemap);
        skybox.addCubemap(cubemap);
        skybox.useSkybox();

        camera.position = Position3d(0.0f, 0.0f, -3.0f);
        camera.useCamera();

        sun = new DirectionalLight(Position3d(0.0f, 0.0f, 1.0f),
                                   Color(1.0f, 0.98f, 0.8f));
        sun->direction = Position3d(-1.0f, -1.0f, -1.0f);
        sun->intensity = 3.f;

        renderTarget = RenderTarget(Size2d(1500, 800), TextureType::Color);
        renderTarget.enable();
        renderTarget.renderToScreen();

        object = generateCubeObject(Position3d(0, 0, 0), Size3d(1.f, 1.f, 1.f));
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

        std::vector<Size2d> textureCoords = {// Front face
                                             {0.0f, 0.0f},
                                             {1.0f, 0.0f},
                                             {1.0f, 1.0f},
                                             {0.0f, 1.0f},
                                             // Back face
                                             {0.0f, 0.0f},
                                             {1.0f, 0.0f},
                                             {1.0f, 1.0f},
                                             {0.0f, 1.0f},
                                             // Left face
                                             {0.0f, 0.0f},
                                             {1.0f, 0.0f},
                                             {1.0f, 1.0f},
                                             {0.0f, 1.0f},
                                             // Right face
                                             {0.0f, 0.0f},
                                             {1.0f, 0.0f},
                                             {1.0f, 1.0f},
                                             {0.0f, 1.0f},
                                             // Top face
                                             {0.0f, 0.0f},
                                             {1.0f, 0.0f},
                                             {1.0f, 1.0f},
                                             {0.0f, 1.0f},
                                             // Bottom face
                                             {0.0f, 0.0f},
                                             {1.0f, 0.0f},
                                             {1.0f, 1.0f},
                                             {0.0f, 1.0f}};
        object.provideTextureCoords(textureCoords);
        object.addTexture(texture);

        object.initialize();
    }
    void update(float deltaTime) override {
        if (isKeyPressed(Key::Escape)) {
            Window::current_window->unlockCursor();
        }
    }
};

int main() {
    Window mywin = Window("Atlas Test", Frame(1500, 800));
    mywin.backgroundColor = Color(0.2f, 0.2f, 0.2f, 1.0f);
    MainScene *mainScene = new MainScene();
    mywin.currentScene = mainScene;

    mywin.run();
    return 0;
}
