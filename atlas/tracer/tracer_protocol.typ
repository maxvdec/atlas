#show title: set text(size: 17pt)
#show title: set align(center)

#let author(name) = [
  #set align(center)
  #name
]

#title[The Tracer Debugging and Profiling Protocol for the Atlas Game Engine Control Specification]

#pad(top: 10pt, author[Atlas Team])
#pad(bottom: 10pt, author[Version 1.0 (Release Alpha 6), December 2025])
#pad(y: 10pt, line(length: 100%))

#set heading(numbering: "1.")

= Introduction

== The purpose

The Tracer Debugging and Profiling Protocol (hereafter referred to as "the Protocol") is a standardized communication protocol designed for debugging and profiling applications built using the Atlas Game Engine. This document outlines the specifications, commands, and data formats used in the Protocol to facilitate effective debugging and performance analysis.

This is a crucial milestone in the Atlas Project, since it provides a fast way to operate with the engine for debugging and profiling purposes, making the development experience actually pleasant and allows a new level of monitoring of resources, telling exactly what is happening inside the engine at any given time and how the developer can actually optimize their code to make it run better.

== Where can we find Tracer being used?

Tracer has two endpoints, being one the client and the other the engine, in this case whatever is using the engine. The client can be any application that implements the Tracer Protocol. We have made our own implementation of a Tracer Client called "Atlas Tracer", which is available as a standalone application for macOS.

In the future, we have plans to integrate Tracer support directly into the Atlas Editor, which is still in development and will be for a long time.

== How can I implement my own Tracer Client?

The Protocol is designed to be simple and easy to implement. The communication is done over TCP/IP sockets, and the messages are formatted in JSON for ease of parsing.

This document is specifically made to help developers implement their own Tracer Clients, so they can create custom tools that fit their specific debugging and profiling needs.

= A breakdown of the protocol

== The Client

The Tracer Client is responsible for sending commands to the Atlas Game Engine and recieving responses. Note that this client is implement with TCP sockets, so it can be implemented in any programming language that supports socket programming. Also, it is worth noting that the client must handle the connection lifecycle, including establishing the connection, sending commands, receiving responses, and closing the connection when done. In the interior, the client is the TCP server and the engine is the TCP client. This means that the engine will wait until a client connects to it before starting the communication.

Basically: *The duty of the client is to recieve, interpret and send data over to the engine*

== The Engine

The Atlas Engine has built-in support for the Tracer Protocol. When the engine starts, it listens for incoming connections from Tracer Clients on a specified port. Once a client connects, the engine will start sending profiling and debugging data to the client as per the commands received.

== The Data

There are various types of data we are going to be sending over the protocol. Data has two classifications, one for type and the other for reception method.

=== Requested Data
This reception method means that the client has to specifically request the data from the engine. This is useful for data that is not needed all the time, or that can be expensive to compute.

=== Continuous Data
This reception method means that the engine will continuously send the data to the client at regular intervals (each frame). This is useful for data that is needed all the time, or that is cheap to compute.

= Data Types
Here are the data types that are used in the protocol and their JSON parameters, classified by the type of debugging or utility they provide (note that every type lives in the "`type`" field of the JSON object):

== Graphics Data

=== Draw Call Information
- Type: `draw_call`
- Information:
  - `caller_object`: The ID of the object that made the draw call.
  - `frame_number`: The frame number when the draw call was made.
  - `draw_call_type`: The type of draw call (see the enum `DrawCallType` for more information).

- Enums:

  ```cpp
  enum class DrawCallType {
    Draw = 1,
    Indexed = 2,
    Patch = 3
  }
  ```

=== Frame Draw Information
- Type: `frame_draw_info`
- Information:
  - `frame_number`: The frame number.
  - `draw_call_count`: Total number of draw calls made in the frame.
  - `frame_time_ms`: Time taken to render the frame in milliseconds.
  - `fps`: Frames per second.


== Memory Trace Data

=== Frame Memory Usage
- Type: `frame_memory_info`
- Information:
  - `frame_number`: The frame number.
  - `total_allocated_mb`: Total memory allocated in megabytes.
  - `total_gpu_mb`: Total GPU memory used in megabytes.
  - `total_cpu_mb`: Total CPU memory used in megabytes.
  - `allocation_count`: Number of memory allocations made in the frame.
  - `deallocation_count`: Number of memory deallocations made in the frame.

=== Memory Allocation / Memory Action Data
- Type: `allocation_event`
- Information:
  - `description`: A brief description of the allocation event.
  - `owner`: The ID of the object that owns the allocation.
  - `domain`: The memory domain (see the enum `MemoryDomain` for more information).
  - `kind`: The kind of memory allocation (see the enum `MemoryKind` for more information).
  - `size_mb`: Size of the allocation in megabytes.
  - `frame_number`: The frame number when the allocation was made.

- Enums:

  ```cpp
  enum class MemoryDomain {
    GPU = 1,
    CPU = 2
  }

  enum class MemoryKind {
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
    Other = 13
  }
  ```

== Object Data

=== Object Information
- Type: `debug_object`
- Information:
  - `id`: The unique identifier of the object.
  - `draw_calls`: Number of draw calls made by the object.
  - `triangle_count`: Number of triangles rendered by the object.
  - `material_count`: Number of materials used by the object.
  - `vertex_buffer_mb`: Size of vertex buffers in megabytes.
  - `index_buffer_mb`: Size of index buffers in megabytes.
  - `texture_count`: Number of textures used by the object.
  - `frame_count`: Number of frames the object has been active.
  - `object_type`: The type of the object (see the enum `DebugObjectType` for more information).

- Enums:

  ```cpp
  enum class DebugObjectType {
    StaticMesh = 1,
    SkeletalMesh = 2,
    ParticleSystem = 3,
    LightProbe = 4,
    Terrain = 5,
    Other = 6
  }
  ```

== Profiling Data

=== Frame Timing Data
- Type: `frame_timing_info`
- Information:
  - `frame_number`: The frame number.
  - `cpu_frame_time_ms`: CPU time taken to render the frame in milliseconds.
  - `gpu_frame_time_ms`: GPU time taken to render the frame in milliseconds.
  - `main_thread_time_ms`: Time taken by the main thread in milliseconds.
  - `worker_thread_time_ms`: Time taken by worker threads in milliseconds.
  - `memory_mb`: Memory used during the frame in megabytes.
  - `cpu_usage_percent`: CPU usage percentage.
  - `gpu_usage_percent`: GPU usage percentage.

=== Timing Event Data
- Type: `timing_event`
- Information:
  - `name`: The name of the timing event.
  - `subsystem`: The subsystem the event belongs to (see the enum `TimingSubsystem` for more information).
  - `duration_ms`: Duration of the event in milliseconds.
  - `frame_number`: The frame number when the event occurred.

- Enums:

  ```cpp
  enum class TimingSubsystem {
    Rendering = 1,
    Physics = 2,
    AI = 3,
    Scripting = 4,
    Animation = 5,
    Audio = 6,
    Networking = 7,
    Io = 8,
    Scene = 9,
    Other = 10
  };
  ```
== Resources Data
=== Resource Event Data
- Type: `resource_event`
- Information:
  - `caller_object`: The ID of the object that made the resource call.
  - `resource_type`: The type of resource (see the enum `ResourceType` for more information).
  - `operation`: The action performed on the resource (see the enum `ResourceAction` for more information).
  - `frame_number`: The frame number when the resource event occurred.
  - `size_mb`: Size of the resource in megabytes.

- Enums:

  ```cpp
  enum class ResourceType {
    Texture = 1,
    Buffer = 2,
    Shader = 3,
    Mesh = 4
  };

  enum class ResourceAction {
    Created = 1,
    Loaded = 2,
    Unloaded = 3
  };
  ```

=== Frame Resources Data
- Type: `frame_resources_info`
- Information:
  - `frame_number`: The frame number.
  - `total_memory_mb`: Total memory used by resources in megabytes.
  - `resource_loaded`: Number of resources loaded in the frame.
  - `resource_unloaded`: Number of resources unloaded in the frame.
  - `resources_created`: Number of resources created in the frame.

= Establishing a Connection

Just connect to the engine's IP address and port using a TCP socket. Once connectJust connect to the engine's IP address and port using a TCP socket. Once connected, you can start sending commands and receiving data. Encode your data in JSON format as specified in the following sections. The port used by default is `5123`, but it can be changed in the engine settings.

= Runtime Variables

At this time, there are no runtime variables available in the Tracer Protocol. Thus, they can't be set. This will be changed in further versions of the protocol.

= Commands

For now, no commands besides the local `/log` or `/clear` commands are available. This will be changed in further versions of the protocol.
