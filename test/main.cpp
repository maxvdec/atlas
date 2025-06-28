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
#include <iostream>

int main() {
    Window mywin = Window("Atlas Test", Frame(1500, 800));

    Workspace workspace(TEST_PATH);
    Resource textureResource = workspace.loadResource("wooden.jng");
    Texture texture;

    auto object = CoreObject({{1.0f, 1.f, 0.0f, Color(1.0f, 0.0f, 0.0f)},
                              {1.0f, -1.f, 0.0f, Color(0.0f, 1.0f, 0.0f)},
                              {-1.0f, -1.0f, 0.0f, Color(0.0f, 0.0f, 1.0f)},
                              {-1.0f, 1.0f, 0.0f, Color(1.0f, 1.0f, 0.0f)}});
    unsigned int indices[] = {0, 1, 2, 2, 3, 0};
    object.provideIndexedDrawing(
        std::vector<unsigned int>(indices, indices + 6));
    object.initialize();
    mywin.run();
    return 0;
}
