//
// data.h
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Different data types and sending types
// Copyright (c) 2025 Max Van den Eynde
//

#ifndef TRACER_DATA_H
#define TRACER_DATA_H

#include <string>

// Graphics debugging

enum class DrawCallType { Draw = 1, Indexed = 2, Patch = 3 };

struct DrawCallInfo {
    std::string callerObject;
    DrawCallType type;
    unsigned int frameNumber;

    void send();
};

struct FrameDrawInfo {
    unsigned int frameNumber;
    unsigned int drawCallCount;
    float frameTimeMs;
    float fps;

    void send();
};

#endif // TRACER_DATA_H