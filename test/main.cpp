#include "atlas/camera.h"
#include "atlas/effect.h"
#include "atlas/light.h"
#include "atlas/object.h"
#include "atlas/scene.h"
#include "atlas/text.h"
#include "atlas/texture.h"
#include "atlas/window.h"
#include "atlas/workspace.h"
#include "atlas/component.h"
#include "atlas/audio.h"
#include <iostream>
#include <memory>

class SphereCube : public CompoundObject {
    CoreObject sphere;
    CoreObject cube;

  public:
    void init() override {
        cube = createDebugBox({0.5f, 0.5f, 0.5f});
        cube.setPosition({-1.0, cube.getPosition().y, 0.0});
        cube.initialize();
        cube.body->applyMass(0); // Make it static
        this->addObject(&cube);

        sphere = createDebugSphere(0.25f);
        sphere.setPosition({1.0, sphere.getPosition().y, 0.0});
        sphere.initialize();
        sphere.body->applyMass(0); // Make it static
        this->addObject(&sphere);
    }
};

class FPSTextUpdater : public TraitComponent<Text> {
  public:
    void updateComponent(Text *object) override {
        int fps = static_cast<int>(getWindow()->getFramesPerSecond());
        object->content = "FPS: " + std::to_string(fps);
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

class BackpackAttach : public Component {
  public:
    void init() override {
        auto player = this->object->getComponent<AudioPlayer>();
        player->setSource(Workspace::get().createResource(
            "exampleMP3.mp3", "ExampleAudio", ResourceType::Audio));
        player->useSpatialization();
        player->source->setLooping(true);
        player->play();
    }
};

class MainScene : public Scene {
    CoreObject ground;
    CoreObject ball;
    CoreObject ball2;
    Light light;
    Skybox skybox;
    Camera camera;
    SphereCube sphereCube;
    Text fpsText;
    Model backpack;
    RenderTarget frameBuffer;

    bool doesUpdate = true;
    bool fall = false;

  public:
    void update(Window &window) override {
        if (!doesUpdate)
            return;

        camera.update(window);
        if (window.isKeyPressed(Key::Escape)) {
            window.releaseMouse();
            doesUpdate = false;
        } else if (window.isKeyClicked(Key::Q)) {
            fall = !fall;
        }
        if (fall) {
            camera.position.y -= 10.f * window.getDeltaTime();
        }
    }

    void onMouseMove(Window &window, Movement2d movement) override {
        if (!doesUpdate) {
            return;
        }
        camera.updateLook(window, movement);
    }

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

        backpack = Model();
        Resource backpackResource = Workspace::get().createResource(
            "backpack/Survival_BackPack_2.fbx", "BackpackModel",
            ResourceType::Model);
        backpack.fromResource(backpackResource);

        Resource colorTexture = Workspace::get().createResource(
            "backpack/1001_albedo.jpg", "BackpackColor", ResourceType::Image);
        Resource normalTexture = Workspace::get().createResource(
            "backpack/1001_normal.png", "BackpackNormal", ResourceType::Image);
        Texture color = Texture::fromResource(colorTexture);
        Texture normal =
            Texture::fromResource(normalTexture, TextureType::Normal);
        backpack.attachTexture(color);
        backpack.attachTexture(normal);

        sphereCube.setPosition({0.0, 1.0, 0.0});
        window.addObject(&sphereCube);

        ground = createBox({5.0f, 0.1f, 5.0f}, Color(0.3f, 0.8f, 0.3f));
        ground.attachTexture(color);
        ground.setPosition({0.0f, -0.1f, 0.0f});
        window.addObject(&ground);

        backpack.setPosition({0.0f, 0.2f, 0.0f});

        Resource fontResource = Workspace::get().createResource(
            "arial.ttf", "Arial", ResourceType::Font);

        fpsText = Text("FPS: 0", Font::fromResource("Arial", fontResource, 24),
                       {25.0, 25.0}, Color::white());

        fpsText.addTraitComponent<Text>(FPSTextUpdater());
        window.addUIObject(&fpsText);

        this->ambientLight.intensity = 0.3f;

        skybox = Skybox();
        skybox.cubemap = createCubemap();
        skybox.display(window);
        this->setSkybox(&skybox);

        frameBuffer = RenderTarget(window);
        frameBuffer.addEffect(ColorCorrection::create());
        frameBuffer.display(window);
        window.addRenderTarget(&frameBuffer);
    }
};

int main() {
    Window window({.title = "My Window",
                   .width = 1600,
                   .height = 1200,
                   .mouseCaptured = true});
    MainScene scene;
    window.setScene(&scene);
    window.run();
    return 0;
}