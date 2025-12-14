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
#include <vector>

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

// Resources Event

enum class DebugResourceType {
    Texture = 1,
    Buffer = 2,
    Shader = 3,
    Mesh = 4,
};

enum class DebugResourceOperation {
    Created = 1,
    Loaded = 2,
    Unloaded = 3,
};

struct ResourceEventInfo {
    std::string callerObject;
    DebugResourceType resourceType;
    DebugResourceOperation operation;
    unsigned int frameNumber;
    float sizeMb;

    void send();
};

struct FrameResourcesInfo {
    unsigned int frameNumber;
    int resourcesCreated;
    int resourcesLoaded;
    int resourcesUnloaded;
    float totalMemoryMb;

    void send();
};

struct IndividualResourceTypeInfo {
    DebugResourceType resourceType;
    int count;
};

class ResourceTracker {
  private:
    ResourceTracker() = default;

  public:
    int createdResources = 0;
    int loadedResources = 0;
    int unloadedResources = 0;
    float totalMemoryMb = 0.0f;

    static ResourceTracker &getInstance() {
        static ResourceTracker instance;
        return instance;
    }
};

#endif // TRACER_DATA_H