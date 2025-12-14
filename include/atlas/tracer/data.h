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

// Objects Debug

enum class DebugObjectType {
    StaticMesh = 1,
    SkeletalMesh = 2,
    ParticleSystem = 3,
    LightProbe = 4,
    Terrain = 5,
    Other = 6,
};

struct DebugObjectPacket {
    int objectId;
    DebugObjectType objectType;

    int triangleCount;
    int materialCount;

    float vertexBufferSizeMb;
    float indexBufferSizeMb;
    float textureCount;

    int drawCallsForObject;

    void send();
};

// Traces Debug

enum class DebugMemoryDomain {
    GPU = 1,
    CPU = 2,
};

enum class DebugResourceKind {
    VertexBuffer = 1,
    IndexBuffer = 2,
    UniformBuffer = 3,
    StorageBuffer = 4,
    Texture2d = 5,
    Texture3d = 6,
    TextureCube = 7,
    RenderTarget = 8,
    DepthStencil = 9,
    Sampler = 10,
    PipelineCache = 11,
    AccelerationStructure = 12,
    Other = 13,
};

struct AllocationPacket {
    std::string description;
    std::string owner;
    DebugMemoryDomain domain;
    DebugResourceKind kind;
    float sizeMb;
    unsigned int frameNumber;

    void send();
};

struct FrameMemoryPacket {
    unsigned int frameNumber;
    float totalAllocatedMb;
    float totalGPUMb;
    float totalCPUMb;
    int allocationCount;
    int deallocationCount;

    void send();
};

#endif // TRACER_DATA_H