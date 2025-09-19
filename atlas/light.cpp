/*
 light.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Lighting helper functions
 Copyright (c) 2025 maxvdec
*/

#include "atlas/light.h"
#include "atlas/core/shader.h"
#include "atlas/object.h"
#include "atlas/window.h"

CoreObject Light::createDebugObject() {
    CoreObject cube = createBox({0.1f, 0.1f, 0.1f}, this->color);
    cube.setPosition(this->position);
    FragmentShader shader =
        FragmentShader::fromDefaultShader(AtlasFragmentShader::Color);
    shader.compile();
    VertexShader vShader =
        VertexShader::fromDefaultShader(AtlasVertexShader::Color);
    vShader.compile();
    ShaderProgram program = ShaderProgram(vShader, shader);
    program.compile();

    cube.attachProgram(program);
    return cube;
}
