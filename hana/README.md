# The Hana Shading Language (Version 1.0)

The Hana Shading Language is a high-level shading language designed for real-time graphics applications. It compiles to GLSL, HLSL and MSL to support multiple graphics APIs. 
It was created for Atlas to use in its rendering engine, but it can be used in regular projects to create beautiful shaders with ease.

# Introduction

## Overview
Hana is a shading language that allows developers to write shaders in a more intuitive and user-friendly way. It abstracts away the complexities of different graphics APIs and provides a unified syntax for writing shaders.
This is useful when dealing with multiple platforms and graphics APIs, as developers can write their shaders once and have them work across different systems without modification. Also, it provides a very strong type system to catch errors at compile time rather than runtime, making it easier to debug and maintain shaders.

Hana is a language inspired by existing shading languages like GLSL, HLSL and MSL, but it introduces several new features and improvements to enhance the developer experience. Some of these features include:

* Strong static typing
* Built-in support for common graphics data types (vectors, matrices, textures, etc.)
* Functions and control flow statements
* Modular shader structure with support for includes and libraries
* Cross-platform compatibility with multiple graphics APIs
* Better syntax, inspired by Swift, making it easier to read and write shaders

# Concepts

First, let's go over some basic concepts of Hana:

## How to run hana files

To run hana files, you need to use the `hana` tool. There are two ways to use it: via command line or programmatically.

### Command Line
To compile a hana file via command line, you can use the following command:

```bash
hana compile <input_file.hana> --for <vulkan|opengl|metal> --out <output_dir>
```

Alternatively, to get the compiled shader code directly in the terminal, you can use:

```bash
hana compile <input_file.hana> --for <vulkan|opengl|metal>
```

### Programmatically

You can include the `<atlas/core/hana.hpp>` header and use the `hana::compile` function to compile hana files programmatically. Here's an example:

```cpp
#include <atlas/core/hana.hpp>

std::string source = R"(
    // Hana shader code here
)";

hana::CompiledShader compiledShader = hana::compile(source, hana::Target::Vulkan, "./"); // You need to specify the root directory for includes
```

# How to write Hana shaders

## Stages

Each shader consists of multiple stages. Instead of living in separate files (although they can), stages are usually defined in the same file. Each stage gets a function and an entry point.

Here are all the possible stages:

```hana
@vertex
func vertex_main() -> {}

@fragment
func fragment_main() -> {}

@tessellation(control)
func tess_control_main() -> {}

@tessellation(evaluation)
func tess_evaluation_main() -> {}

@geometry
func geometry_main() -> {}

@compute
func compute_main() -> {}

@mesh // For this one, you need an extension. Use 'use hana::mesh' at the top of your file.
func mesh_main() -> {}

@task // For this one, you need an extension. Use 'use hana::task' at the top of your file.
func task_main() -> {}

@raytracing(generation) // For this one, you need an extension. Use 'use hana::raytracing' at the top of your file.
func raygen_main() -> {}

@raytracing(closest) // For this one, you need an extension. Use 'use hana::raytracing' at the top of your file.
func closesthit_main() -> {}

@raytracing(any) // For this one, you need an extension. Use 'use hana::raytracing' at the top of your file.
func anyhit_main() -> {}

@raytracing(miss) // For this one, you need an extension. Use 'use hana::raytracing' at the top of your file.
func miss_main() -> {}

@raytracing(intersection) // For this one, you need an extension. Use 'use hana::raytracing' at the top of your file.
func intersection_main() -> {}

@raytracing(callable) // For this one, you need an extension. Use 'use hana::raytracing' at the top of your file.
func callable_main() -> {}
```

Each stage function must be annotated with the appropriate stage attribute (e.g., `@vertex`, `@fragment`, etc.) and must have a unique name. The entry point function is where the execution of the shader begins for that stage.

## Inputs and Outputs

Inputs and outputs are defined using special structs annotated with some attributes to indicate their purpose. Here are some examples:

```hana
@stage(vertex, in)
struct VertexInput {
    vec3 position;
    vec3 normal;
    vec2 texCoord;
}

@stage(vertex, out)
@stage(fragment, in)
struct VertexOutput {
    vec3 fragNormal;
    vec2 fragTexCoord;
}
```

Now, you can use these structs in your stage functions:

```hana
@vertex
func vertex_main(VertexInput input) -> VertexOutput {
    VertexOutput output;
    output.fragNormal = input.normal;
    output.fragTexCoord = input.texCoord;
    return output;
}
```

Fragment outputs work differently, as they are set as variables, with the `@output(layoutCount)` attribute:

```hana
@output(0)
vec4 outColor;

@output(1)
vec4 brightColor;

@fragment
func fragment_main(VertexOutput input) -> {
    outColor = vec4(input.fragNormal, 1.0);
    brightColor = vec4(0.0);
    if (length(input.fragNormal) > 0.5) {
        brightColor = outColor;
    }
}
```

## Syntax and Types

Hana's syntax is inspired by languages like Swift and Rust, making it easy to read and write. Here are some basic syntax rules and types:

### Basic Types
- `bool`: Boolean type
- `int`: 32-bit signed integer
- `uint`: 32-bit unsigned integer
- `float`: 32-bit floating-point number
- `double`: 64-bit floating-point number
- `vec2`, `vec3`, `vec4`: 2D, 3D, and 4D vectors of floats
- `ivec2`, `ivec3`, `ivec4`: 2D, 3D, and 4D vectors of integers
- `uvec2`, `uvec3`, `uvec4`: 2D, 3D, and 4D vectors of unsigned integers
- `mat2`, `mat3`, `mat4`: 2x2, 3x3, and 4x4 matrices of floats

### Control Flow
Hana supports standard control flow statements like `if`, `else`, `for`, `while`, and `switch`. Here's an example:

```hana
func example_func(int value) -> int {
    var int result = 0;
    if (value > 0) {
        result = value * 2;
    } else {    
        result = value - 2;
    }
    return result;
}
```

## Adding data to the shader

### Uniforms

Uniforms are defined using the `@uniform` attribute. Here's an example:

```hana
@uniform(set = 0, binding = 0)
var mat4 modelMatrix;
```

For OpenGL, the uniform's name will be `modelMatrix`. To change that, use the `@opengl` attribute:

```hana
@uniform(set = 0, binding = 0)
@opengl("uModelMatrix")
var mat4 modelMatrix;
```

### Textures and Samplers
Textures are defined using the `@texture` attribute, and the samplers are going to be defined automatically. But take in account that the bind location will be shared between the texture and the sampler.

```hana
@texture(set = 0, binding = 1) // This will take binding 1 for the texture and binding 2 for the sampler
Texture2D albedoTexture;

// Later in code we use 'vec4 color = albedoTexture.sample(texCoord);'
```

### Buffers

First, we need to align a structure to be used in the buffer:

```hana
@align(std430)
struct Particle {
    vec3 position;
    vec3 velocity;
    float lifetime;
}
```

Then, we can define a storage buffer:

```hana
@buffer(set = 0, binding = 3)
Buffer particles;
```

Where `particles.count` gives the number of elements in the buffer, and `particles[index]` gives access to a specific element.

Now, since some versions of OpenGL don't support writable storage buffers, we need to translate the buffer to a uniform buffer for that API.

```hana
@buffer(set = 0, binding = 3)
@openglTransformToUniform("lights", 16) // 16 is the max number of elements
Buffer lights;
```

### Push Constants
Push constants are defined using the `@push` attribute. Here's an example:

```hana
@push
const mat4 viewProjectionMatrix;
```

## Versions and libraries

At the top of your Hana file, you can specify the version of Hana you want to use and import any libraries you need. Here's an example:

```hana
@hana latest

use hana // For core and math functions
use hana::mesh // For mesh shaders
use hana::task // For task shaders
use hana::raytracing // For raytracing shaders
use hana::texture // For advanced texture functions

use myLibrary // Your own library

include "anyfile.hana"
```

## Your own libraries

To create your own libraries, you just need to create a folder with a `.hanalib` file inside, like this:

```
MyLibrary/
    .hanalib
    utils.hana
    math.hana
    lighting.hana
```

That `.hanalib` file is a simple text file that includes all the files you want to be part of your library:

```
name "myLibrary"
include "utils.hana" (name "myLibrary::utils")
include "math.hana" (name "myLibrary::math")
include "lighting.hana" (name "myLibrary::lighting")
```

After that, you can use your library in your Hana shaders by passing it in the command:

```bash
hana compile <input_file.hana> --for <vulkan|opengl|metal> --out <output_dir> --lib ./MyLibrary
```

## Stage-specific variables

There are some built-in variables that are specific to certain stages. Here are some examples:

### Vertex Stage
- `@position`: The position of the vertex in clip space.
- `@pointSize`: The size of the point sprite.
- `@instanceId`: The instance ID for instanced rendering.
- `@vertexId`: The vertex ID within the current primitive.
- `@drawId`: The draw ID for multi-draw indirect rendering.

### Tessellation Control Stage
- `@invocationId`: The invocation ID of the current patch.
- `@in[]`: The input control points for the patch.
- `@out[]`: The output control points for the patch.
- `@tessLevelOuter[]`: The outer tessellation levels for the patch.
- `@tessLevelInner[]`: The inner tessellation levels for the patch.
- `@primitiveId`: The primitive ID of the current patch.

### Tessellation Evaluation Stage
- `@tessCoord`: The tessellation coordinates of the current fragment.
- `@in[]`: The input control points for the patch.
- `@primitiveId`: The primitive ID of the current patch.
- `@position`: The position of the vertex in clip space.

### Geometry Stage
- `@in[]`: The input vertices for the primitive.
- `@emitVertex()`: Function to emit a vertex.
- `@endPrimitive()`: Function to end the current primitive.
- `@primitiveIdIn`: The primitive ID of the input primitive.
- `@primitiveId`: The primitive ID of the output primitive.
- `@layer`: The layer for layered rendering.
- `@viewportIndex`: The viewport index for multi-viewport rendering.
  
### Fragment Stage
- `@fragCoordinates`: The fragment's coordinates in window space.
- `@frontFacing`: A boolean indicating whether the primitive is front-facing.
- `@pointCoordinates`: The coordinates of the point sprite.
- `@sampleId`: The sample ID for multi-sampling.
- `@samplePosition`: The sample position for multi-sampling.
- `@sampleMask[]`: The sample mask for multi-sampling.
- `@fragDepth`: The depth value of the fragment.
- `@sampleMaskIn[]`: The input sample mask for multi-sampling.
- `@primitiveId`: The primitive ID of the current fragment.

### Compute Stage
- `@localInvocationId`: The local invocation ID within the workgroup.
- `@globalInvocationId`: The global invocation ID across all workgroups.
- `@workgroupId`: The workgroup ID.
- `@numWorkgroups`: The total number of workgroups.
- `@localInvocationIndex`: The linear index of the local invocation within the workgroup.

### Mesh and Task Stages
- `@meshVertices[]`: The output vertices for the mesh shader.
- `@meshPrimitives[]`: The output primitives for the mesh shader.
- `@taskCount`: The number of mesh tasks to be generated.
- `@workgroupId`: The workgroup ID for the task shader.
- `@localInvocationId`: The local invocation ID within the task shader.

### Raytracing Stages
- `@rayOrigin`: The origin of the ray.
- `@rayDirection`: The direction of the ray.
- `@hitT`: The hit distance along the ray.
- `@launchId`: The launch ID of the current ray.
- `@launchSize`: The total size of the ray launch.
- `@primitiveId`: The primitive ID of the hit geometry.
- `@instanceId`: The instance ID of the hit geometry.
- `@geometryId`: The geometry ID of the hit geometry.
- `@hitKind`: The hit kind of the ray.
- `@callableData[]`: The callable data for callable shaders.
- `@missIndex`: The miss index for miss shaders.
- `@intersectionT`: The intersection distance along the ray.
- `@intersectionCandidate`: A boolean indicating whether the intersection candidate is valid.
- `@acceptIntersection()`: Function to accept the intersection.
- `@rejectIntersection()`: Function to reject the intersection.
- `@reportIntersection(t, hitKind)`: Function to report an intersection with the given distance and hit kind.
- `@terminateRay()`: Function to terminate the ray.
