#include "atlas/camera.h"
#include "atlas/particle.h"
#include "atlas/light.h"
#include "atlas/object.h"
#include "atlas/scene.h"
#include "atlas/text.h"
#include "atlas/texture.h"
#include "atlas/units.h"
#include "atlas/window.h"
#include "atlas/workspace.h"
#include "atlas/component.h"
#include "atlas/audio.h"
#include "aurora/procedural.h"
#include "aurora/terrain.h"
#include "hydra/atmosphere.h"
#include "hydra/fluid.h"
#include <memory>

class SphereCube : public CompoundObject {
    CoreObject sphere;
    CoreObject cube;

  public:
    void init() override {
        cube = createDebugBox({0.5f, 0.5f, 0.5f});
        cube.setPosition({-1.0f, cube.getPosition().y, 0.0f});
        cube.initialize();
        cube.body->applyMass(0); // Make it static
        this->addObject(&cube);

        for (int i = 0; i < 6; i++) {
            Instance &instance = cube.createInstance();
            instance.move({0.0f, 0.6f * i, 0.0f});
        }

        sphere = createDebugSphere(0.25f);
        sphere.setPosition({1.0f, sphere.getPosition().y, 0.0f});
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
  private:
    float phase = 0.0f;

  public:
    void update(float deltaTime) override {
        float amplitude = 0.01f;
        float frequency = 4.0f;
        phase += deltaTime * frequency * 2.0f * M_PI;
        float position = amplitude * sin(phase);
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

class WaterPot : public CompoundObject {
    CoreObject pot;
    Fluid water;

  public:
    void init() override {
        pot = createBox({1.0f, 0.25f, 0.25f}, Color(0.6f, 0.4f, 0.2f));

        Instance &potRight = pot.createInstance();
        potRight.move({0.0f, 0.0f, 1.0f});
        Instance &potDown = pot.createInstance();
        potDown.rotate({0.0f, 90.0f, 0.0f});
        potDown.move({-0.5f, 0.0f, 0.5f});
        Instance &potUp = pot.createInstance();
        potUp.rotate({0.0f, -90.0f, 0.0f});
        potUp.move({0.5f, 0.0f, 0.5f});
        pot.initialize();
        this->addObject(&pot);

        Texture waterDUDV =
            Texture::fromResource(Workspace::get().createResource(
                "water_dudv.png", "WaterDUDV", ResourceType::Image));

        Texture waterNormal =
            Texture::fromResource(Workspace::get().createResource(
                "water_normal.png", "WaterNormal", ResourceType::Image));

        water = Fluid();
        water.create({0.9, 0.9}, Color::blue());
        water.setPosition({0.0f, 0.10f, 0.5f});
        water.movementTexture = waterDUDV;
        water.normalTexture = waterNormal;
        water.initialize();
        this->addObject(&water);
    }
};

class MainScene : public Scene {
    CoreObject ground;
    CoreObject ball;
    CoreObject ball2;
    DirectionalLight light;
    Camera camera;
    CoreObject lightObject;
    SphereCube sphereCube;
    Text fpsText;
    Model backpack;
    RenderTarget frameBuffer;
    Terrain terrain;
    AreaLight areaLight;
    ParticleEmitter emitter;
    WaterPot waterPot;
    Model sponza;

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

        sponza = Model();
        sponza.fromResource(Workspace::get().createResource(
            "sponza.obj", "SponzaModel", ResourceType::Model));
        sponza.setScale({0.01f, 0.01f, 0.01f});

        sponza.material.albedo = Color(1.0, 0.0, 0.0, 1.0);

        this->setAmbientIntensity(0.2f);

        window.addObject(&sponza);

        window.useDeferredRendering();
        atmosphere.enable();
        atmosphere.secondsPerHour = 4.f;
        atmosphere.setTime(12);
        atmosphere.cycle = false;
        atmosphere.useGlobalLight();

        atmosphere.castShadowsFromSunlight(4096);
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
