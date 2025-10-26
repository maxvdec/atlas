# Atlas

![GitHub contributors](https://img.shields.io/github/contributors/maxvdec/atlas)
![GitHub last commit](https://img.shields.io/github/last-commit/maxvdec/atlas)
![Tests](https://github.com/maxvdec/atlas/actions/workflows/build.yaml/badge.svg)
![GitHub Issues or Pull Requests](https://img.shields.io/github/issues/maxvdec/atlas)
![GitHub Repo stars](https://img.shields.io/github/stars/maxvdec/atlas)

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
- The audio engine is Finewave. Finewave is a separate library that handles all the audio playback and processing.
- The terrain system is Aurora. Aurora is a separate library that handles terrain generation and rendering.

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
- [x] Post-processing effects
- [ ] Sky, atmosphere, weather and fluid simulation system (Hydra)
- [ ] Animation system (Mold)
- [x] Instancing support
- [x] Optimizations and performance improvements
- [x] CLI for packing applications
- [ ] Runtime independent of final executable
- [ ] Scene packing
- [x] Add deferred rendering for better performance with many lights
- [x] Add PBR rendering **for alpha 3**
- [x] Add other texture supports (normal, roughness, metallic, ao, etc.) **for alpha 3**
- [x] Add HDR support
- [x] Add SSAO
- [x] Add soft shadows
- [x] Add shadows for point lights
- [x] Add normal and parallax mapping
- [ ] Handle better collisions with Bezel
- [ ] Add support for constraints in Bezel
- [ ] Add support for character controllers in Bezel
- [ ] Add support for motors in Bezel
- [x] Add volumetric lighting **for alpha 3**
- [x] Add bloom for lights
- [x] Add depth of field
- [x] Add terrain system (Aurora) **for alpha 3**
- [x] Add motion blur **for alpha 3**
- [x] Physically based bloom **for alpha 3**
- [x] Add area lights **for alpha 3**
- [ ] Screen-space reflections
- [x] Add rim lighting **for alpha 3**
- [x] Add chromatic aberration **for alpha 3**
- [x] Add posterization **for alpha 3**
- [ ] Add pixelization **for alpha 3**
- [ ] Add dialation **for alpha 3**
- [ ] Add film grain **for alpha 3**
- [ ] Add LUT tables **for alpha 3**
- [ ] Add foam and simple fluid rendering
- [ ] Screen-space refractions

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
