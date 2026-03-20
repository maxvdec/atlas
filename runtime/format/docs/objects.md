# Objects that can appear in scene files

These are the IDs and documentation for the objects that can appear in scene files. These are the objects that are used to represent the entities, components and values in the scene format files.

## Solid (`type = "solid"`)

A solid is simply an object that can be defined as a 3D object with a material and components:

* `solid_type`: The type of the solid, which can be `cube`, `plane`, `pyramid` or `sphere`.
* `position`: The position of the solid in the scene, defined as an array of three numbers representing the x, y and z coordinates.
* `rotation`: The rotation of the solid in the scene, defined as an array of three numbers representing the rotation around the x, y and z axes in degrees.
* `scale`: The scale of the solid in the scene, defined as an array of three numbers representing the scale along the x, y and z axes.
* `material`: The material of the solid, which is a reference to a material file (`.amat`) that defines the properties of the material, such as textures, colors and other properties.
* `components`: An array of components that are attached to the solid, which can be used to define the behavior and properties of the solid in the scene. Each component is defined as an object with a `type` property that specifies the type of the component, and other properties that define the values for that component.

## CompoundObject (`type = "compound"`)

A compound object is an object that can contain other objects, which can be used to create more complex scenes. It has the following properties:

* `position`: The position of the compound object in the scene, defined as an array of three numbers representing the x, y and z coordinates.
* `rotation`: The rotation of the compound object in the scene, defined as an array of three numbers representing the rotation around the x, y and z axes in degrees.
* `scale`: The scale of the compound object in the scene, defined as an array of three numbers representing the scale along the x, y and z axes.
* `objects`: An array of objects that are contained within the compound object, which can be of any type (solid, compound, etc.). Each object is defined as an object with a `type` property that specifies the type of the object, and other properties that define the values for that object.

## Model (`type = "model"`)

A model is an object that represents a 3D model that can be loaded from a file. It has the following properties:

* `source`: The source of the model, which is a reference to a model file (e.g., `.obj`, `.fbx`, etc.) that defines the geometry and other properties of the model.
* `position`: The position of the model in the scene, defined as an array of three numbers representing the x, y and z coordinates.
* `rotation`: The rotation of the model in the scene, defined as an array of three numbers representing the rotation around the x, y and z axes in degrees.
* `scale`: The scale of the model in the scene, defined as an array of three numbers representing the scale along the x, y and z axes.
* `material`: The material of the model, which is a reference to a material file (`.amat`) that defines the properties of the material, such as textures, colors and other properties.
* `components`: An array of components that are attached to the model, which can be used to define the behavior and properties of the model in the scene. Each component is defined as an object with a `type` property that specifies the type of the component, and other properties that define the values for that component.

## ParticleEmitter (`type = "particle_emitter"`)
A particle emitter is an object that represents a particle system that can be used to create effects such as fire, smoke, etc. It has the following properties:

* `maxParticles`: The maximum number of particles that can be emitted by the particle emitter.
* `texture`: The texture of the particles, which is a reference to a texture file (e.g., `.png`, `.jpg`, etc.) that defines the appearance of the particles.
* `color`: The color of the particles, defined as an array of three numbers representing the red, green and blue components of the color (values between 0 and 255).
* `useTexture`: A boolean value that indicates whether to use the texture for the particles or not. If `false`, the particles will be rendered as simple colored points.
* `position`: The position of the particle emitter in the scene, defined as an array of three numbers representing the x, y and z coordinates.
* `direction`: The direction of the particle emitter, defined as an array of three numbers representing the x, y and z components of the direction vector.
* `spawnRadius`: The radius around the particle emitter where the particles will be spawned, defined as a single number representing the radius in units.
* `spawnRate`: The rate at which particles are emitted, defined as a single number representing the number of particles emitted per second.
* `emitOnce`: A boolean value that indicates whether to emit particles only once or continuously. If `true`, the particle emitter will emit particles only once and then stop. If `false`, the particle emitter will continue emitting particles until it is removed from the scene or disabled.
* `settings`:
    * `minLifetime`: The minimum lifetime of the particles, defined as a single number representing the lifetime in seconds.
    * `maxLifetime`: The maximum lifetime of the particles, defined as a single number representing the lifetime in seconds.
    * `minSize`: The minimum size of the particles, defined as a single number representing the size in units.
    * `maxSize`: The maximum size of the particles, defined as a single number representing
    * `fadeSpeed`: The speed at which the particles fade out, defined as a single number representing the speed in units per second. A higher value means that the particles will fade out faster.
    * `gravity`: The gravity applied to the particles, defined as a single number representing the strength of the gravity in units per second squared. A higher value means that the particles will be pulled down faster.
    * `spread`: The spread of the particles, defined as a single number representing the angle in degrees that the particles will spread out from the direction vector. A higher value means that the particles will spread out more.
    * `speedVariation`: The variation in the speed of the particles, defined as a single number representing the percentage of variation in the speed (values between 0 and 100). A higher value means that the particles will have more variation in their speed.

## Terrain (`type = "terrain"`)
A terrain is an object that represents a heightmap-based terrain that can be used to create landscapes and other outdoor environments. It has the following properties:

* `heightmap`: The heightmap of the terrain, which is a reference to a heightmap file (e.g., `.png`, `.jpg`, etc.) that defines the height values of the terrain.
* `moistureTexture`: The moisture texture of the terrain, which is a reference to a texture file (e.g., `.png`, `.jpg`, etc.) that defines the moisture values of the terrain. This can be used to create different biomes and vegetation on the terrain.
* `temperatureTexture`: The temperature texture of the terrain, which is a reference to a texture file (e.g., `.png`, `.jpg`, etc.) that defines the temperature values of the terrain. This can be used to create different biomes and vegetation on the terrain.
* `generator`: The generator settings for the terrain, which is an object that defines the settings for the terrain generation algorithm. See the documentation for 'other' for more information on the generator settings.
* `width`: The width of the terrain, defined as a single number representing the width in units.
* `height`: The height of the terrain, defined as a single number representing the height in units.
* `resolution`: The resolution of the terrain, defined as a single number representing the number of vertices along each axis. A higher value means that the terrain will have more detail, but it will also require more resources to render and generate.
* `position`: The position of the terrain in the scene, defined as an array of three numbers representing the x, y and z coordinates.
* `rotation`: The rotation of the terrain in the scene, defined as an array of three numbers representing the rotation around the x, y and z axes in degrees.
* `scale`: The scale of the terrain in the scene, defined as an array of three numbers representing the scale along the x, y and z axes.
* `biomes`: See the documentation for 'other' for more information on the biome settings for the terrain.
* `maxPeak`: The maximum peak height of the terrain, defined as a single number representing the maximum height in units. This can be used to limit the height of the terrain and create more realistic landscapes.
* `minPeak`: The minimum peak height of the terrain, defined as a single number representing the minimum height in units. This can be used to limit the height of the terrain and create more realistic landscapes.

## Skybox (`type = "skybox"`)

A skybox is an object that represents a skybox that can be used to create a background for the scene. It has the following properties:

* `cubemap`: The cubemap of the skybox, which is a reference to a cubemap file (e.g., `.hdr`, `.dds`, etc.) that defines the appearance of the skybox. A cubemap is a texture that contains six images that represent the six faces of a cube, which can be used to create a 360-degree background for the scene.