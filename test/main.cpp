#include "atlas/camera.h"
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
    DirectionalLight light;
    Skybox skybox;
    Camera camera;
    SphereCube sphereCube;
    Text fpsText;
    Model backpack;

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

        Color whiteMultiplier = Color(1.0, 1.0, 1.0);
        Color mediumMultiplier = Color(0.75, 0.75, 0.75);
        Color darkMultiplier = Color(0.5, 0.5, 0.5);

        Color blue = Color(0.5, 0.5, 1.0);

        Texture checkerboard = Texture::createDoubleCheckerboard(
            4096, 4096, 640, 80, blue * whiteMultiplier, blue * darkMultiplier,
            blue * mediumMultiplier);

        ground = createDebugBox({5.0f, 0.5f, 5.0f});
        ground.setPosition({0.0f, -0.6f, 0.0f});
        ground.body->applyMass(0); // Make it static
        ground.attachTexture(checkerboard);
        ground.body->friction = 0.0f;
        window.addObject(&ground);

        ball = createDebugBox({0.2f, 0.2f, 0.2f});
        ball.setPosition({0.0f, 2.0f, 0.0f});
        ball.body->applyLinearImpulse({0.0f, 0.0f, -1.0f});
        ball.attachTexture(checkerboard);
        ball.body->friction = 1.0f;
        window.addObject(&ball);

        ball2 = createDebugSphere(0.1f);
        ball2.setPosition({1.0f, 2.0f, 0.0f});
        ball2.body->applyLinearImpulse({0.0f, 0.0f, -1.0f});
        ball2.attachTexture(checkerboard);
        ball2.body->friction = 1.0f;
        window.addObject(&ball2);

        Resource fontResource = Workspace::get().createResource(
            "arial.ttf", "Arial", ResourceType::Font);

        fpsText = Text("FPS: 0", Font::fromResource("Arial", fontResource, 24),
                       {25.0, 25.0}, Color::white());

        fpsText.addTraitComponent<Text>(FPSTextUpdater());
        window.addUIObject(&fpsText);

        light = DirectionalLight({-0.75f, -1.0f, 0.0}, Color::white());
        this->addDirectionalLight(&light);

        this->ambientLight.intensity = 0.3f;

        skybox = Skybox();
        skybox.cubemap = createCubemap();
        skybox.display(window);
        this->setSkybox(&skybox);
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