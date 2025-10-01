# Tutorial
\page tutorial Tutorial

This tutorial teaches you how to create a scene like this:

![Result](result.png)

To make this, we'll going to use everything we need, from UI to physics, to components. This will teach you the basics of the engine.

## Installation

First of all, we need to install the engine. To install the engine, go to the following [GitHub repository](https://github.com/maxvdec/atlas) and go to the releases page.
After downloading the latest release, you can download the executable called `atlas`. That executable is the core of Atlas. With the executable you can run, create and do whatever
with the engine. It is pretty useful and straightforward.

### Create a Project

Creating a project with the atlas executable is super simple. Enter a folder and try running `atlas create testProject`. This will download the necessary things and resources from the
latest stable release. 

The project structure once created is pretty simple. You just basically need to center with the `testProject/main.cpp` file which is the basic file we're going to need. First of all, 
we're going to edit `testProject/main.cpp` and remove all of its content, because we're going to begin from scratch

## Setting up the enviornment

### Creating a Window

Creating a Window is the first step to every application. The window is our canvas, and to create it, with Atlas is quite simple. This is the code we're going to need:

```c++
#include <atlas/window.h>

int main() {
    Window window({.title = "My Window",
                   .width = 1600,
                   .height = 1200,
                   .mouseCaptured = false});
    window.run();
    return 0;
}
```

If you go see the documentation for \ref Window, you'll see that the constructor can take more arguments, but for now, we like these. This way, we can see the window running
normally and our mouse is not captured. Here's how the result looks like:

![CreatingAWindow](create-window.png)

### Creating a Scene

Now, scenes are one other important concept in Atlas. Scenes are the base of every small interaction and thing that happens in the game. It organizes objects and more. Here's a rather simple.
In this scene, we're just going to create a cube.

```c++
#include "atlas/object.h"
#include "atlas/scene.h"
#include "atlas/window.h"

class MainScene : public Scene {
    CoreObject cube;

  public:
    void initialize(Window &window) override {
        cube = createBox({1.0f, 1.0f, 1.0f}, Color::red());
        cube.rotate({45.0f, 45.0f, 0.0f});
        window.addObject(&cube);
    }
};

int main() {
    Window window({.title = "My Window",
                   .width = 1600,
                   .height = 1200,
                   .mouseCaptured = false});
    MainScene scene;
    window.setScene(&scene);
    window.run();
    return 0;
}
```

Now here, a lot is happening. Look at this fragment:

```c++
cube = createBox({1.0f, 1.0f, 1.0f}, Color::red());
cube.rotate({45.0f, 45.0f, 0.0f});
```

We create a box and we set its color to red. You can tweak the size by touching the first three values. We also add this object to the window.
When you create a scene make sure that all the objects that are going to render, you must declare them in class level, so the pointers are not destroyed.

Also, note how we instanciate the scene and set it as the main scene of the window.

```c++
MainScene scene;
window.setScene(&scene);
```

If we run this, we get something like this:

![UnlitScene](unlit-scene.png)

Do you see the cube? Maybe you don't and that's because there aren't any lights. Not even a directional light! So we might need to create one

### Creating a Directional Light

Directional Lights simulate the sun. They are simple since they just have a direction. So it's simple to set them like this:

```c++
#include <atlas/light.h>

class MainScene : public Scene {
    CoreObject cube;
    DirectionalLight light;

  public:
    void initialize(Window &window) override {
        cube = createBox({1.0f, 1.0f, 1.0f}, Color::red());
        cube.rotate({45.0f, 45.0f, 0.0f});
        window.addObject(&cube);

        light = DirectionalLight({-0.75f, -1.0f, 0.0}, Color::white());
        this->addDirectionalLight(&light);
    }
};
```

When we run the scene now, we get a better result!

![LitScene](lit-scene.png)

For even better results, we can boost up the **ambient light**. That will give lighter colors, and better illumination. Try adding this line:

```c++
this->ambientLight.intensity = 0.3f;
```

That gives us this result!

![Ambient Light](ambient.png)

But the scene still fills a bit empty, so let's try adding the skybox.

### Adding the Skybox

To add a Skybox, we need to understand that is basically six pictures conforming some sort of dice. You normally have six different textures, build a **cubemap** and pass it to the **skybox**. To make a cubemap, you should download the textures here:

#### Building the Cubemap

Try adding these textures to a new folder in `testProject/resources`. Then, inside that, we can add `skybox`. Inside `testProject/resources/skybox` add the following pictures:

* This as `nx.png`
  ![NX](nx.png)
* This as `ny.png`
  ![NY](ny.png)
* This as `nz.png`
  ![NZ](nz.png)
* This as `px.png`
  ![PX](px.png)
* This as `py.png`
  ![PY](py.png)
* And this as `pz.png`
  ![PZ](pz.png)

Now let's hop on on building the actual cubemap!

Let's make a function in our main scene that does that.

```c++
#include <atlas/workspace.h>
#include <atlas/texture.h>

class MainScene : public Scene {
    CoreObject cube;
    DirectionalLight light;

  public:
    Cubemap createCubemap() {
        // Cubemaps are in this order: right, left, top, bottom, front, back
        Resource right = Workspace::get().createResource(
            "skybox/px.png", "RightSkybox", ResourceType::Image);
        Resource left = Workspace::get().createResource(
            "skybox/nx.png", "LeftSkybox", ResourceType::Image);
        Resource top = Workspace::get().createResource(
            "skybox/py.png", "TopSkybox", ResourceType::Image);
        Resource bottom = Workspace::get().createResource(
            "skybox/ny.png", "BottomSkybox", ResourceType::Image);
        Resource front = Workspace::get().createResource(
            "skybox/pz.png", "FrontSkybox", ResourceType::Image);
        Resource back = Workspace::get().createResource(
            "skybox/nz.png", "BackSkybox", ResourceType::Image);
        ResourceGroup group = Workspace::get().createResourceGroup(
            "Skybox", {right, left, top, bottom, front, back});

        return Cubemap::fromResourceGroup(group);
    }

    void initialize(Window &window) override {
        // We need to set the root path to resources
        Workspace::get().setRootPath("resources/");

        cube = createBox({1.0f, 1.0f, 1.0f}, Color::red());
        cube.rotate({45.0f, 45.0f, 0.0f});
        window.addObject(&cube);

        light = DirectionalLight({-0.75f, -1.0f, 0.0}, Color::white());
        this->addDirectionalLight(&light);

        this->ambientLight.intensity = 0.3f;
    }
};
```

#### Building the Skybox

After this, is quite simple to build the skybox. Let's add it first to our class properties:

```c++
class MainScene : public Scene {
    CoreObject cube;
    DirectionalLight light;
    Skybox skybox; 
...
```

And then, inside the `initialize` function, we can add this line at the end:

```c++
skybox = Skybox();
skybox.cubemap = createCubemap();
skybox.display(window);
```

This will display the skybox in the window. Now, if we run the scene, we get this:

![Skybox](skybox.png)

Now, the perspective is a bit weird because the rotation of the cube is a bit odd. Let's fix that by adding a camera.

### Adding a Camera

To add a camera, we simply need to create a camera object and set its position. Let's add this to our class properties:

```c++
#include <atlas/camera.h>

class MainScene : public Scene {
    CoreObject cube;
    DirectionalLight light;
    Skybox skybox;
    Camera camera;
...
```
And then, inside the `initialize` function, we can set its position and make the window use it:

```c++
camera = Camera();
camera.setPosition({-5.0f, 1.0f, 2.0f});
camera.lookAt({0.0f, 0.0f, 0.0f});
window.setCamera(&camera);
```

Now, the perspective is way better:

![Camera](camera.png)

Let's begin building the scene by adding the ground and falling ball.

## Setting up the scene
### Creating the ground

To create the ground, we can simply create a debug box and scale it to our needs. To make that
let's try adding this to our class properties:

```c++
class MainScene : public Scene {
    CoreObject ground;
    DirectionalLight light;
    Skybox skybox;
    Camera camera;
...
```

Let's remove all of our cube code and add this to the `initialize` function:

```c++
ground = createDebugBox({5.0f, 0.3f, 5.0f});
```

But before adding it to the window, we're going to add a couple stuff. First, we're going to create a texture for it that resembles a double checkerboard.

#### Creating a texture

To create a texture, we can use the `Texture::createDoubleCheckerboard` function. This function creates a texture with a double checkerboard pattern. Let's create a texture for the ground:

```c++
Texture checkerboard = Texture::createDoubleCheckerboard(4096, 4096, 640, 80, Color(0.5, 0.5, 1.0),
Color(0.5, 0.5, 1.0), Color(0.375, 0.375, 0.75));
```

We can then set this texture to the ground:

```c++
ground.attachTexture(checkerboard);
```

After this, we need to make sure the ground doesn't fall. To do that, we can set its mass to infinity, which will sove the issue:

```c++
ground.body->applyMass(0);
```

Now we can add the ground to the window:

```c++
window.addObject(&ground);
```

Doing this, we get this result:

![Ground](plane.png)

### Creating a falling ball

Again, to create a falling ball, we can use the `createDebugSphere` function. Let's add a new property to our class:

```c++
class MainScene : public Scene {
    CoreObject ground;
    CoreObject ball;
    DirectionalLight light;
    Skybox skybox;
    Camera camera;
...
```

And then, inside the `initialize` function, we can add this code:

```c++
ball = createDebugSphere(0.1);
ball.setPosition({0.0, 2.0, 0.0}); // Start above the ground
ball.body->linearVelocity = {2.0, 0.0, 0.0}; // Give it some initial horizontal velocity
ball.body->friction = 0.5f;
window.addObject(&ball);
```

Adding this code, we get this result:
![Falling Ball](fall-ball.png)

But we miss something... the ball has no shadows! Let's add shadows to the directional light by adding this line under the initialization of the light:

```c++
light.castShadows(window, 4096); // 4096 is the shadow map resolution
```

After this, we get this nice shadowed result:
![Shadows](shadows.png)

After this, we are ready to add our first compound object.

## Creating a compound object

Compound objects are objects that are made up of multiple shapes. To create a compound object, we create a new class that inherits from `CompoundObject`. Let's create a simple static sphere and cube.

```c++
#include <atlas/component.h>

class SphereCube : public CompoundObject {
    CoreObject sphere;
    CoreObject cube;

  public:
    void init() override {
        cube = createDebugBox({0.5f, 0.5f, 0.5f});
        cube.setPosition({-1.0, 1.0, 0.0});
        cube.initialize();
        cube.body->applyMass(0); // Make it static
        this->addObject(&cube);

        sphere = createDebugSphere(0.25f);
        sphere.setPosition({1.0, 1.0, 0.0});
        sphere.initialize();
        sphere.body->applyMass(0); // Make it static
        this->addObject(&sphere);
    }
};

```

Now, let's go ahead and add this compound object to our scene. First, we need to add a new property to our class:

```c++
class MainScene : public Scene {
    CoreObject ground;
    CoreObject ball;
    SphereCube sphereCube;
    DirectionalLight light;
    Skybox skybox;
    Camera camera;
...
```

Then, inside the `initialize` function, we can add this code:

```c++
sphereCube = SphereCube();
window.addObject(&sphereCube);
```

And we get this result:

![Compound Object](compound.png)

\tableofcontents

Now, we need to move those shapes a bit! We're going to create our first component to do that.

## Creating a component

Creating a component is super simple. Components are classes that inherit from `Component` and have an `update` function that is called every frame. Let's create a simple component that moves the objects horizontally sinusoially.

```c++
#include <atlas/component.h>

class HorizontalMover : public Component {
  public:
    void update(float deltaTime) override {
        Window* window = Window::mainWindow;
        float amplitude = 0.01f;
        float position = amplitude * sin(window->getTime());
        object->move({position, 0.0f, 0.0f});
    }
};
```

And we can now apply this component to our compound object from our `initialize` function in our scene:

```c++
sphereCube.addComponent<HorizontalMover>(HorizontalMover());
```

Now, if we run the scene, we get this result:

![Moving Compound Object](moving.png)

Now we just have to add one thing, the text on the screen.

## Adding UI

To add a text, we first declare the text object in our class properties:

```c++
#include <atlas/text.h>

class MainScene : public Scene {
    CoreObject ground;
    CoreObject ball;
    SphereCube sphereCube;
    DirectionalLight light;
    Skybox skybox;
    Camera camera;
    Text fpsText;
...
```

Then, inside the `initialize` function, we can add this code:

```c++
Resource fontResource = Workspace::get().createResource("fonts/somefont.ttf", "SomeFont", ResourceType::Font);

fpsText = Text("FPS: 0", Font::fromResource("SomeFont", fontResource, 24), {25.0, 25.0}, Color::white());

window.addObject(&fpsText);
```

If we run this, we get this result:

![Text](text.png)

But we're not done yet. We need to update the text every frame to show the current FPS. To do that, we can create a new component that updates the text every frame. The thing is that we need to pass the text object to the component and we want our component to just work with text objects. So we can create a TraitComponent that works with text objects. Here's how we can do that:

```c++
#include <atlas/component.h>

class FPSTextUpdater : public TraitComponent<Text> {
  public:
    void updateComponent(Text *object) override {
        int fps = static_cast<int>(getWindow()->getFramesPerSecond());
        object->content = "FPS: " + std::to_string(fps);
    }
};
```

And we can now apply this component to our text object from our `initialize` function in our scene:

```c++
fpsText.addTraitComponent<FPSTextUpdater>(FPSTextUpdater());
```

## Conclusion

And that's it! We have our final scene!

```c++
#include "atlas/camera.h"
#include "atlas/light.h"
#include "atlas/object.h"
#include "atlas/scene.h"
#include "atlas/text.h"
#include "atlas/texture.h"
#include "atlas/window.h"
#include "atlas/workspace.h"
#include "atlas/component.h"

class SphereCube : public CompoundObject {
    CoreObject sphere;
    CoreObject cube;

  public:
    void init() override {
        cube = createDebugBox({0.5f, 0.5f, 0.5f});
        cube.setPosition({-1.0, 1.0, 0.0});
        cube.initialize();
        cube.body->applyMass(0); // Make it static
        this->addObject(&cube);

        sphere = createDebugSphere(0.25f);
        sphere.setPosition({1.0, 1.0, 0.0});
        sphere.initialize();
        sphere.body->applyMass(0); // Make it static
        this->addObject(&sphere);
    }
};

class HorizontalMover : public Component {
  public:
    void update(float deltaTime) override {
        Window *window = Window::mainWindow;
        float amplitude = 0.01f;
        float position = amplitude * sin(window->getTime());
        object->move({position, 0.0f, 0.0f});
    }
};

class MainScene : public Scene {
    CoreObject ground;
    CoreObject ball;
    DirectionalLight light;
    Skybox skybox;
    Camera camera;
    SphereCube sphereCube;
    Text fpsText;

  public:
    Cubemap createCubemap() {
        Resource right = Workspace::get().createResource(
            "skybox/px.png", "RightSkybox", ResourceType::Image);
        Resource left = Workspace::get().createResource(
            "skybox/nx.png", "LeftSkybox", ResourceType::Image);
        Resource top = Workspace::get().createResource(
            "skybox/py.png", "TopSkybox", ResourceType::Image);
        Resource bottom = Workspace::get().createResource(
            "skybox/ny.png", "BottomSkybox", ResourceType::Image);
        Resource front = Workspace::get().createResource(
            "skybox/pz.png", "FrontSkybox", ResourceType::Image);
        Resource back = Workspace::get().createResource(
            "skybox/nz.png", "BackSkybox", ResourceType::Image);
        ResourceGroup group = Workspace::get().createResourceGroup(
            "Skybox", {right, left, top, bottom, front, back});

        return Cubemap::fromResourceGroup(group);
    }

    void initialize(Window &window) override {
        Workspace::get().setRootPath(std::string(TEST_PATH) + "/resources/");

        camera = Camera();
        camera.setPosition({-5.0f, 1.0f, 2.0f});
        camera.lookAt({0.0f, 0.0f, 0.0f});
        window.setCamera(&camera);

        ground = createDebugBox({5.0f, 0.5f, 5.0f});
        Texture checkerboard = Texture::createDoubleCheckerboard(
            4096, 4096, 640, 80, Color(0.5, 0.5, 1.0), Color(0.5, 0.5, 1.0),
            Color(0.375, 0.375, 0.75));
        ground.attachTexture(checkerboard);
        ground.body->applyMass(0);
        window.addObject(&ground);

        ball = createDebugSphere(0.1);
        ball.setPosition({0.0, 2.0, 0.0}); // Start above the ground
        ball.body->linearVelocity = {2.0, 0.0, 0.0};
        ball.body->friction = 0.5f;
        window.addObject(&ball);

        sphereCube = SphereCube();
        sphereCube.addComponent<HorizontalMover>(HorizontalMover());
        window.addObject(&sphereCube);

        Resource fontResource = Workspace::get().createResource(
            "arial.ttf", "Arial", ResourceType::Font);

        fpsText = Text("FPS: 0", Font::fromResource("Arial", fontResource, 24),
                       {25.0, 25.0}, Color::white());

        window.addObject(&fpsText);

        light = DirectionalLight({-0.75f, -1.0f, 0.0}, Color::white());
        this->addDirectionalLight(&light);

        this->ambientLight.intensity = 0.3f;

        skybox = Skybox();
        skybox.cubemap = createCubemap();
        skybox.display(window);
    }
};

int main() {
    Window window({.title = "My Window",
                   .width = 1600,
                   .height = 1200,
                   .mouseCaptured = false});
    MainScene scene;
    window.setScene(&scene);
    window.run();
    return 0;
}
```