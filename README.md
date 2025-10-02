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

## Roadmap to Alpha

- [x] Basic window creation and management
- [x] Input handling (keyboard, mouse)
- [x] Basic rendering (shapes, textures)
- [ ] Simple UI **for alpha**
- [ ] Add mesh loading **for alpha**
- [x] Scene management
- [x] Physics engine integration (Bezel)
- [x] Audio support (Finewave)
- [ ] Scripting support
- [x] Asset management
- [x] Documentation and examples **for alpha**
- [x] Particle system **for alpha**
- [x] Lights
- [x] Shadows (except for point lights)
- [ ] Post-processing effects
- [ ] Sky, atmosphere, weather and fluid simulation system (Hydra)
- [ ] Animation system (Mold)
- [ ] Advanced materials and shaders
- [ ] Instancing support
- [ ] Optimizations and performance improvements
- [x] CLI for packing applications
- [ ] Runtime independent of final executable 
- [ ] Scene packing

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
