# Atlas

Atlas is a Game Engine that uses the latest technologies to provide a fast, simple but powerful experience for developers.
It is built with C++ and uses OpenGL for rendering, with plans to support Vulkan and Metal in the future.

## Features

- Cross-platform support (Windows, macOS, Linux)
- Fast graphics rendering with OpenGL
- Simple and intuitive API
- Physics engine (Bezel)

## Engine Architecture

- The main renderer is Atlas. Atlas is the one in charge of the graphics rendering, input handling, window management and more.
- The physics engine is Bezel. Bezel is a separate library that handles all the physics calculations and simulations.

## Roadmap to Beta

- [x] Basic window creation and management
- [x] Input handling (keyboard, mouse)
- [x] Basic rendering (shapes, textures)
- [x] Simple UI
- [ ] Editor UI
- [ ] Editor application
- [ ] Editor runtime
- [x] Add mesh loading
- [x] Scene management
- [x] Physics engine integration (Bezel)
- [x] Audio support (Finewave)
- [ ] Scripting support
- [x] Asset management
- [x] Documentation and examples
- [x] Particle system
- [x] Lights
- [x] Shadows (except for point lights)
- [x] Post-processing effects **for alpha 2**
- [ ] Sky, atmosphere, weather and fluid simulation system (Hydra)
- [ ] Animation system (Mold)
- [ ] Instancing support **for alpha 2**
- [ ] Optimizations and performance improvements
- [x] CLI for packing applications
- [ ] Runtime independent of final executable
- [ ] Scene packing
- [ ] Add deferred rendering for better performance with many lights **for alpha 2**
- [ ] Add PBR rendering
- [ ] Add other texture supports (normal, roughness, metallic, ao, etc.)
- [x] Add HDR support **for alpha 2**
- [ ] Add SSAO **for alpha 2**
- [ ] Add cascaded shadow maps for directional lights
- [ ] Add soft shadows **for alpha 2**
- [x] Add shadows for point lights **for alpha 2**
- [x] Add normal and parallax mapping **for alpha 2**
- [ ] Handle better collisions with Bezel
- [ ] Add support for constraints in Bezel
- [ ] Add support for character controllers in Bezel
- [ ] Add support for motors in Bezel
- [ ] Add volumetric lighting
- [x] Add bloom for lights **for alpha 2**
- [x] Add depth of field **for alpha 2**
- [ ] Add terrain system
- [ ] Add motion blur
- [ ] Physically based bloom
- [ ] Add area lights

## Other features planned for the future

- [ ] AI system (Adverse)
- [ ] UI library (Graphite)
- [ ] Vegetation and foliage system
- [ ] Terrain generation and rendering (Terra)
- [ ] Network support (Lasse)
- [ ] Metal support
- [ ] Vulkan support
- [ ] Ray tracing rendering (Overflow)
- [ ] Add 2d physics (Bezel 2D support)
