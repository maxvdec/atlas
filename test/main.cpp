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
    CoreObject object;
    Camera camera;
    Model model;
    void init() override {
        Workspace workspace(TEST_PATH);
        Resource textureResource = workspace.loadResource("container.png");
        Texture texture;
        texture.fromImage(textureResource, TextureType::Color);

        Texture specular;
        specular.fromImage(workspace.loadResource("specular.png"),
                           TextureType::Specular);

        camera.position = Position3d(0.0f, 0.0f, -3.0f);
        camera.useCamera();

        light = new PointLight(Position3d(0.0f, 0.0f, 1.0f),
                               Color(1.0f, 0.83f, 0.5f));
        light->debugLight();
        light->intensity = 1.5f;

        model = Model(workspace.loadResource("backpack/backpack.obj"));
        model.initialize();
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
