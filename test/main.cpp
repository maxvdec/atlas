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