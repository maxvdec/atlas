/*
 main.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: The main entry point for the Atlas Test
 Copyright (c) 2025 maxvdec
*/

#include <atlas/core/rendering.hpp>
#include <atlas/window.hpp>
#include <iostream>

int main() {
    Window mywin = Window("Atlas Test", Frame(1500, 800));
    auto object = CoreObject({{0.0f, 0.5f, 0.0f, Color(1.0f, 0.0f, 0.0f)},
                              {-0.5f, -0.5f, 0.0f, Color(0.0f, 1.0f, 0.0f)},
                              {0.5f, -0.5f, 0.0f, Color(0.0f, 0.0f, 1.0f)}});
    mywin.run();
    return 0;
}
