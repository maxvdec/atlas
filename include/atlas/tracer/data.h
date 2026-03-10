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
    /** @brief Name/id of the renderable or system that issued the call. */
    std::string callerObject;
    /** @brief Draw call topology/dispatch kind. */
    DrawCallType type;
    /** @brief Frame index at which the call was recorded. */
    unsigned int frameNumber;

    /** @brief Sends this event to the tracer sink. */
    void send();
};

/**
 * @brief Per-frame aggregate draw call telemetry.
 */
struct FrameDrawInfo {
    /** @brief Frame index for this aggregate packet. */
    unsigned int frameNumber;
    /** @brief Number of draw calls observed during the frame. */
    unsigned int drawCallCount;
    /** @brief Total frame time in milliseconds. */
    float frameTimeMs;
    /** @brief Frames per second derived from frame time. */
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
    /** @brief Emitter that triggered the resource event. */
    std::string callerObject;
    /** @brief Category of resource involved. */
    DebugResourceType resourceType;
    /** @brief Lifecycle transition that occurred. */
    DebugResourceOperation operation;
    /** @brief Frame index at which the event was recorded. */
    unsigned int frameNumber;
    /** @brief Approximate resource size in megabytes. */
    float sizeMb;

    /** @brief Sends this event to the tracer sink. */
    void send();
};

/**
 * @brief Per-frame aggregate resource telemetry.
 */
struct FrameResourcesInfo {
    /** @brief Frame index for this aggregate packet. */
    unsigned int frameNumber;
    /** @brief Number of resources created in this frame. */
    int resourcesCreated;
    /** @brief Number of resources loaded in this frame. */
    int resourcesLoaded;
    /** @brief Number of resources unloaded in this frame. */
    int resourcesUnloaded;
    /** @brief Total estimated memory footprint in MB. */
    float totalMemoryMb;

    /** @brief Sends this event to the tracer sink. */
    void send();
};

/**
 * @brief Counts resources per kind.
 */
struct IndividualResourceTypeInfo {
    /** @brief Resource category being counted. */
    DebugResourceType resourceType;
    /** @brief Number of resources observed in that category. */
    int count;
};

/**
 * @brief Singleton accumulator used by the tracer to summarize resource usage.
 */
class ResourceTracker {
  private:
    ResourceTracker() = default;

  public:
    /** @brief Number of resource create events observed. */
    int createdResources = 0;
    /** @brief Number of resource load events observed. */
    int loadedResources = 0;
    /** @brief Number of resource unload events observed. */
    int unloadedResources = 0;
    /** @brief Accumulated memory footprint estimate in MB. */
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
    /** @brief Object identifier in engine space. */
    int objectId;
    /** @brief High-level object classification. */
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
    /** @brief Human-readable allocation description. */
    std::string description;
    /** @brief Subsystem or owner responsible for the allocation. */
    std::string owner;
    /** @brief Memory domain where bytes were allocated. */
    DebugMemoryDomain domain;
    /** @brief Resource kind associated with this allocation. */
    DebugResourceKind kind;
    /** @brief Allocation size in megabytes. */
    float sizeMb;
    /** @brief Frame index at which allocation happened. */
    unsigned int frameNumber;

    /** @brief Sends this event to the tracer sink. */
    void send();
};

/**
 * @brief Per-frame aggregate memory telemetry.
 */
struct FrameMemoryPacket {
    /** @brief Frame index for this aggregate packet. */
    unsigned int frameNumber;
    /** @brief Total allocated memory in MB. */
    float totalAllocatedMb;
    /** @brief GPU memory usage in MB. */
    float totalGPUMb;
    /** @brief CPU memory usage in MB. */
    float totalCPUMb;
    /** @brief Number of allocation events in the frame. */
    int allocationCount;
    /** @brief Number of deallocation events in the frame. */
    int deallocationCount;

    /** @brief Sends this event to the tracer sink. */
    void send();
};

// Timing Debug

/**
 * @brief Per-frame timing and utilization telemetry.
 */
struct FrameTimingPacket {
    /** @brief Frame index for this aggregate packet. */
    unsigned int frameNumber;
    /** @brief Total CPU frame time in milliseconds. */
    float cpuFrameTimeMs;
    /** @brief Total GPU frame time in milliseconds. */
    float gpuFrameTimeMs;
    /** @brief Main thread execution time in milliseconds. */
    float mainThreadTimeMs;
    /** @brief Worker thread execution time in milliseconds. */
    float workerThreadTimeMs;
    /** @brief Process memory usage in MB. */
    float memoryMb;
    /** @brief CPU utilization percentage for the frame. */
    float cpuUsagePercent;
    /** @brief GPU utilization percentage for the frame. */
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
    /** @brief Event label shown in tracing UI. */
    std::string name;
    /** @brief Subsystem this timing event belongs to. */
    TimingEventSubsystem subsystem;
    /** @brief Duration in milliseconds. */
    float durationMs;
    /** @brief Frame index at which the event was captured. */
    unsigned int frameNumber;

    /** @brief Sends this event to the tracer sink. */
    void send();
};

#endif // TRACER_DATA_H
