#include "atlas/camera.h"
#include "atlas/effect.h"
#include "atlas/particle.h"
#include "atlas/light.h"
#include "atlas/object.h"
#include "atlas/physics.h"
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
#include <iostream>
#include <memory>

class SphereCube : public CompoundObject {
    CoreObject sphere;
    CoreObject cube;

  public:
    void init() override {
        cube = createDebugBox({0.5f, 0.5f, 0.5f});
        cube.setPosition({-1.0f, cube.getPosition().y, 0.0f});
        cube.initialize();
        this->addObject(&cube);

        for (int i = 0; i < 6; i++) {
            Instance &instance = cube.createInstance();
            instance.move({0.0f, 0.6f * i, 0.0f});
        }

        sphere = createDebugSphere(0.25f);
        sphere.setPosition({1.0f, sphere.getPosition().y, 0.0f});
        sphere.initialize();
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

class BallBehavior : public Component {
  public:
    void init() override {
        if (object && object->rigidbody) {
            object->rigidbody->applyImpulse({0.0f, 0.0f, 20.0f});
        }
    }
    void update(float) override {
        if (Window::mainWindow->isKeyClicked(Key::N)) {
            auto hinge = object->getComponent<HingeJoint>();
            if (hinge) {
                std::cout << "Breaking hinge joint\n";
                hinge->breakJoint();
            }
        }
    }
};

class MainScene : public Scene {
    CoreObject ground;
    CoreObject wallLeft;
    CoreObject wallRight;
    CoreObject wallBack;
    CoreObject cieling;
    CoreObject tallBox;
    CoreObject cubeBox;
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
    int giDebugMode = 0;

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
        } else if (window.isKeyClicked(Key::G) &&
                   window.ddgiSystem != nullptr &&
                   window.ddgiSystem->probeSpace != nullptr) {
            giDebugMode = (giDebugMode + 1) % 4;
            float debugAlpha = 0.0f;
            if (giDebugMode == 1) {
                debugAlpha = 20.0f;
            } else if (giDebugMode == 2) {
                debugAlpha = 30.0f;
            } else if (giDebugMode == 3) {
                debugAlpha = 40.0f;
            }
            window.ddgiSystem->probeSpace->debugColor =
                Color(3.0f, 3.0f, 3.0f, debugAlpha);
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

#define COZY_LIGHT_COLOR Color(1.0f, 0.96f, 0.86f)

    void initialize(Window &window) override {
        Environment env;
        env.fog.intensity = 0.0;
        env.volumetricLighting.enabled = false;
        env.lightBloom.radius = 0.008f;
        env.lightBloom.maxSamples = 5;
        this->setEnvironment(env);

        Workspace::get().setRootPath(std::string(TEST_PATH) + "/resources/");

        camera = Camera();
        camera.setPosition({0.0f, 1.0f, -4.f});
        camera.lookAt({0.0f, 1.0f, 0.2f});
        camera.farClip = 100.0f;
        window.setCamera(&camera);

        areaLight = AreaLight();
        areaLight.position = {0.0f, 1.98f, 0.0f};
        areaLight.setColor(COZY_LIGHT_COLOR);
        areaLight.intensity = 2.2f;
        areaLight.range = 6.5f;
        areaLight.size = {0.70f, 0.46f};
        areaLight.angle = 125.0f;
        areaLight.castsBothSides = false;
        areaLight.setRotation({90.0f, 0.0f, 0.0f});
        areaLight.castShadows(window, 2048);
        this->addAreaLight(&areaLight);
        areaLight.createDebugObject();
        areaLight.addDebugObject(window);

        const Color wallWhite = Color(0.84f, 0.74f, 0.58f);
        const Color boxWhite = Color(0.90f, 0.84f, 0.70f);
        const Color wallRed = Color(0.98f, 0.05f, 0.03f);
        const Color wallGreen = Color(0.04f, 0.95f, 0.07f);

        ground = createBox({2.0f, 0.05f, 2.0f});
        ground.material.albedo = wallWhite;
        ground.material.roughness = 0.96f;
        ground.material.metallic = 0.0f;
        ground.material.ao = 1.0f;
        ground.setPosition({0.0f, -0.025f, 0.0f});
        window.addObject(&ground);

        cieling = createBox({2.0f, 0.05f, 2.0f});
        cieling.material.albedo = wallWhite;
        cieling.material.roughness = 0.95f;
        cieling.material.metallic = 0.0f;
        cieling.material.ao = 1.0f;
        cieling.setPosition({0.0f, 2.025f, 0.0f});
        window.addObject(&cieling);

        wallLeft = createBox({0.05f, 2.0f, 2.0f});
        wallLeft.material.albedo = wallRed;
        wallLeft.material.roughness = 0.94f;
        wallLeft.material.metallic = 0.0f;
        wallLeft.material.ao = 1.0f;
        wallLeft.setPosition({-1.025f, 1.0f, 0.0f});
        window.addObject(&wallLeft);

        wallRight = createBox({0.05f, 2.0f, 2.0f});
        wallRight.material.albedo = wallGreen;
        wallRight.material.roughness = 0.94f;
        wallRight.material.metallic = 0.0f;
        wallRight.material.ao = 1.0f;
        wallRight.setPosition({1.025f, 1.0f, 0.0f});
        window.addObject(&wallRight);

        wallBack = createBox({2.0f, 2.0f, 0.05f});
        wallBack.material.albedo = wallWhite;
        wallBack.material.roughness = 0.94f;
        wallBack.material.metallic = 0.0f;
        wallBack.material.ao = 1.0f;
        wallBack.setPosition({0.0f, 1.0f, 1.025f});
        window.addObject(&wallBack);

        tallBox = createBox({0.56f, 1.2f, 0.56f});
        tallBox.material.albedo = boxWhite;
        tallBox.material.roughness = 0.78f;
        tallBox.material.metallic = 0.0f;
        tallBox.material.ao = 1.0f;
        tallBox.setPosition({-0.42f, 0.6f, 0.5f});
        tallBox.setRotation({0.0f, -16.0f, 0.0f});
        window.addObject(&tallBox);

        cubeBox = createBox({0.6f, 0.6f, 0.6f});
        cubeBox.material.albedo = boxWhite;
        cubeBox.material.roughness = 0.76f;
        cubeBox.material.metallic = 0.0f;
        cubeBox.material.ao = 1.0f;
        cubeBox.setPosition({0.42f, 0.3f, -0.5f});
        cubeBox.setRotation({0.0f, 11.0f, 0.0f});
        window.addObject(&cubeBox);

        Resource fontResource = Workspace::get().createResource(
            "arial.ttf", "Arial", ResourceType::Font);

        fpsText = Text("FPS: 0", Font::fromResource("Arial", fontResource, 24),
                       {25.0, 25.0}, Color::white());

        fpsText.addTraitComponent<Text>(FPSTextUpdater());
        window.addUIObject(&fpsText);

        this->setAmbientIntensity(0.10f);

        frameBuffer = RenderTarget(window, RenderTargetType::Multisampled);
        window.addRenderTarget(&frameBuffer);
        frameBuffer.display(window);

        window.enableGlobalIllumination();
        window.ddgiSystem->probeSpacing = 0.30f;
        window.ddgiSystem->raysPerProbe = 64;
        window.ddgiSystem->maxRayDistance = 7.5f;
        window.ddgiSystem->normalBias = 0.03f;
        window.ddgiSystem->hysteresis = 0.96f;
        window.ddgiSystem->probeSpace->probeResolution = 8;
        window.ddgiSystem->probeSpace->textureBorderSize = 1;
        window.ddgiSystem->probeSpace->debugColor =
            Color(1.0f, 1.0f, 1.0f, 0.0f);
        // window.ddgiSystem->irradianceMap->display(window);

        this->setUseAtmosphereSkybox(false);

        window.usesDeferred = true;
        window.enableSSR(false);
    }
};

int main() {
    Window window({.title = "My Window",
                   .width = 1600,
                   .height = 1200,
                   .renderScale = 5.0f,
                   .mouseCaptured = true,
                   .multisampling = false,
                   .ssaoScale = 0.4f});
    MainScene scene;
    window.setScene(&scene);
    window.run();
    return 0;
}
