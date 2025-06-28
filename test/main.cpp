/*
 main.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: The main entry point for the Atlas Test
 Copyright (c) 2025 maxvdec
*/

#include "atlas/texture.hpp"
#include "atlas/workspace.hpp"
#include <atlas/core/rendering.hpp>
#include <atlas/window.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

int main() {
    Window mywin = Window("Atlas Test", Frame(1500, 800));
    mywin.backgroundColor = Color(0.2f, 0.2f, 0.2f, 1.0f);

    Workspace workspace(TEST_PATH);
    Resource textureResource = workspace.loadResource("wooden.jpg");
    Texture texture;
    texture.fromImage(textureResource);

    auto object = CoreObject({{0.5f, 0.5f, 0.0f, Color(1.0f, 0.0f, 0.0f)},
                              {0.5f, -0.5f, 0.0f, Color(0.0f, 1.0f, 0.0f)},
                              {-0.5f, -0.5f, 0.0f, Color(0.0f, 0.0f, 1.0f)},
                              {-0.5f, 0.5f, 0.0f, Color(1.0f, 1.0f, 0.0f)}});

    object.provideTextureCoords({Size2d(1.0f, 1.0f), Size2d(1.0f, 0.0f),
                                 Size2d(0.0f, 0.0f), Size2d(0.0f, 1.0f)});
    object.setTexture(texture);
    object.rotate(-55.0f, Axis::X);
    object.viewMatrix =
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -3.0f));
    unsigned int indices[] = {0, 1, 2, 2, 3, 0};
    object.provideIndexedDrawing(
        std::vector<unsigned int>(indices, indices + 6));
    object.initialize();
    mywin.run();
    return 0;
}
