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

/**
 * @file atlas/tracer/data.h
 * @brief Packet types and enums used by the Tracer Protocol.
 *
 * Each `*Info` / `*Packet` type describes a payload that can be sent to an
 * external tracer/visualizer via its `send()` method.
 *
 * \note This is an alpha API and may change.
 */

// Graphics debugging

/**
 * @brief Describes the kind of draw call that occurred.
 */
enum class DrawCallType { Draw = 1, Indexed = 2, Patch = 3 };

/**
 * @brief Draw call telemetry emitted by the renderer.
 */
struct DrawCallInfo {
    std::string callerObject;
    DrawCallType type;
    unsigned int frameNumber;

    /** @brief Sends this event to the tracer sink. */
    void send();
};

/**
 * @brief Per-frame aggregate draw call telemetry.
 */
struct FrameDrawInfo {
    unsigned int frameNumber;
    unsigned int drawCallCount;
    float frameTimeMs;
    float fps;

    /** @brief Sends this event to the tracer sink. */
    void send();
};

// Resources Event

/**
 * @brief Broad kind of debug resource.
 */
enum class DebugResourceType {
    Texture = 1,
    Buffer = 2,
    Shader = 3,
    Mesh = 4,
};

/**
 * @brief Resource lifecycle operation.
 */
enum class DebugResourceOperation {
    Created = 1,
    Loaded = 2,
    Unloaded = 3,
};

/**
 * @brief Resource lifecycle event emitted by resource systems.
 */
struct ResourceEventInfo {
    std::string callerObject;
    DebugResourceType resourceType;
    DebugResourceOperation operation;
    unsigned int frameNumber;
    float sizeMb;

    /** @brief Sends this event to the tracer sink. */
    void send();
};

/**
 * @brief Per-frame aggregate resource telemetry.
 */
struct FrameResourcesInfo {
    unsigned int frameNumber;
    int resourcesCreated;
    int resourcesLoaded;
    int resourcesUnloaded;
    float totalMemoryMb;

    /** @brief Sends this event to the tracer sink. */
    void send();
};

/**
 * @brief Counts resources per kind.
 */
struct IndividualResourceTypeInfo {
    DebugResourceType resourceType;
    int count;
};

/**
 * @brief Singleton accumulator used by the tracer to summarize resource usage.
 */
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

/**
 * @brief Coarse type classification for object telemetry.
 */
enum class DebugObjectType {
    StaticMesh = 1,
    SkeletalMesh = 2,
    ParticleSystem = 3,
    LightProbe = 4,
    Terrain = 5,
    Other = 6,
};

/**
 * @brief Object-level telemetry packet.
 */
struct DebugObjectPacket {
    int objectId;
    DebugObjectType objectType;

    int triangleCount;
    int materialCount;

    float vertexBufferSizeMb;
    float indexBufferSizeMb;
    float textureCount;

    int drawCallsForObject;
    int frameCount;

    /** @brief Sends this event to the tracer sink. */
    void send();
};

// Traces Debug

/**
 * @brief Memory domain in which an allocation lives.
 */
enum class DebugMemoryDomain {
    GPU = 1,
    CPU = 2,
};

/**
 * @brief Kind of resource being tracked for memory telemetry.
 */
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

/**
 * @brief Single allocation event packet.
 */
struct AllocationPacket {
    std::string description;
    std::string owner;
    DebugMemoryDomain domain;
    DebugResourceKind kind;
    float sizeMb;
    unsigned int frameNumber;

    /** @brief Sends this event to the tracer sink. */
    void send();
};

/**
 * @brief Per-frame aggregate memory telemetry.
 */
struct FrameMemoryPacket {
    unsigned int frameNumber;
    float totalAllocatedMb;
    float totalGPUMb;
    float totalCPUMb;
    int allocationCount;
    int deallocationCount;

    /** @brief Sends this event to the tracer sink. */
    void send();
};

// Timing Debug

/**
 * @brief Per-frame timing and utilization telemetry.
 */
struct FrameTimingPacket {
    unsigned int frameNumber;
    float cpuFrameTimeMs;
    float gpuFrameTimeMs;
    float mainThreadTimeMs;
    float workerThreadTimeMs;
    float memoryMb;
    float cpuUsagePercent;
    float gpuUsagePercent;

    /** @brief Sends this event to the tracer sink. */
    void send();
};

/**
 * @brief Subsystem classification used for timing events.
 */
enum class TimingEventSubsystem {
    Rendering = 1,
    Physics = 2,
    AI = 3,
    Scripting = 4,
    Animation = 5,
    Audio = 6,
    Networking = 7,
    Io = 8,
    Scene = 9,
    Other = 10,
};

/**
 * @brief Single timed event packet.
 */
struct TimingEventPacket {
    std::string name;
    TimingEventSubsystem subsystem;
    float durationMs;
    unsigned int frameNumber;

    /** @brief Sends this event to the tracer sink. */
    void send();
};

#endif // TRACER_DATA_H
