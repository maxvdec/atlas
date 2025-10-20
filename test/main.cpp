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
#include "aurora/procedural.h"
#include "aurora/terrain.h"
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

        for (int i = 0; i < 6; i++) {
            Instance &instance = cube.createInstance();
            instance.move({0.0f, 0.6f * i, 0.0f});
        }

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
    DirectionalLight light;
    Skybox skybox;
    Camera camera;
    CoreObject lightObject;
    SphereCube sphereCube;
    Text fpsText;
    Model backpack;
    RenderTarget frameBuffer;
    Terrain terrain;
    AreaLight areaLight;

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
        Environment env;
        env.fog.intensity = 0.0;
        this->setEnvironment(env);

        Workspace::get().setRootPath(std::string(TEST_PATH) + "/resources/");

        camera = Camera();
        camera.setPosition({-5.0f, 1.0f, 2.0f});
        camera.lookAt({0.0f, 0.0f, 0.0f});
        camera.farClip = 1000.f;
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

        sphereCube.setPosition({0.0, 0.25, 0.0});
        window.addObject(&sphereCube);

        ground = createBox({5.0f, 0.1f, 5.0f}, Color(0.3f, 0.8f, 0.3f));
        ground.attachTexture(
            Texture::fromResource(Workspace::get().createResource(
                "ground.jpg", "GroundTexture", ResourceType::Image)));
        ground.setPosition({0.0f, -0.1f, 0.0f});
        window.addObject(&ground);

        backpack.setPosition({0.0f, 0.2f, 0.0f});

        areaLight.position = {0.0f, 2.0f, 0.0};
        areaLight.rotate({0.0f, 90.0f, 0.0f});
        areaLight.castsBothSides = true;
        // this->addAreaLight(&areaLight);
        areaLight.createDebugObject();
        areaLight.addDebugObject(window);

        Resource fontResource = Workspace::get().createResource(
            "arial.ttf", "Arial", ResourceType::Font);

        fpsText = Text("FPS: 0", Font::fromResource("Arial", fontResource, 24),
                       {25.0, 25.0}, Color::white());

        fpsText.addTraitComponent<Text>(FPSTextUpdater());
        window.addUIObject(&fpsText);

        lightObject = createBox({1.0f, 1.0f, 1.0f}, Color::yellow());
        lightObject.setPosition({0.0f, 0.001f, 0.0f});
        // lightObject.makeEmissive(this, {5.0, 5.0, 5.0}, 2.0f);
        for (int i = 0; i < 4; i++) {
            Instance &instance = lightObject.createInstance();
            instance.move({0.0f, 1.1f * i, 0.0f});
        }

        ball = createDebugSphere(0.5f, 76, 76);
        ball.body->applyMass(0.0);
        ball.move({0.f, 1.0f, 1.5});
        ball.material.reflectivity = 1.f;

        this->setAmbientIntensity(0.1f);

        skybox = Skybox();
        skybox.cubemap = createCubemap();
        skybox.display(window);
        this->setSkybox(&skybox);

        Resource heightmapResource = Workspace::get().createResource(
            "terrain/heightmap.png", "Heightmap", ResourceType::Image);

        CompoundGenerator compoundGen;
        compoundGen.addGenerator(MountainGenerator(0.01f, 1.f, 5, 0.5f));

        terrain = Terrain(heightmapResource);
        terrain.move({20.f, 0.0, 0.0});
        Biome grasslandBiome =
            Biome("Grassland", Color(0.1f, 0.8f, 0.1f, 1.0f));
        grasslandBiome.condition = [](Biome &biome) {
            biome.maxHeight = 10.0f;
        };
        terrain.addBiome(grasslandBiome);

        Biome mountainBiome = Biome("Mountain", Color(0.5f, 0.5f, 0.5f, 1.0f));
        mountainBiome.condition = [](Biome &biome) {
            biome.minHeight = 10.0f;
            biome.maxHeight = 150.0f;
        };
        terrain.addBiome(mountainBiome);

        Biome snowBiome = Biome("Snow", Color(4.0f, 4.0f, 4.0f, 4.0f));
        snowBiome.condition = [](Biome &biome) { biome.minHeight = 150.0f; };
        terrain.addBiome(snowBiome);
        terrain.resolution = 100;
        terrain.maxPeak = 100.f;

        light = DirectionalLight({1.0f, -0.3f, 0.5f}, Color::white());
        this->addDirectionalLight(&light);

        this->setAmbientIntensity(0.1);

        frameBuffer = RenderTarget(window);
        window.addRenderTarget(&frameBuffer);
        frameBuffer.display(window);

        window.useDeferredRendering();
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
