#include <SDL3/SDL_main.h>
#include "atlas/camera.h"
#include "atlas/input.h"
#include "atlas/particle.h"
#include "atlas/light.h"
#include "atlas/object.h"
#include "atlas/physics.h"
#include "atlas/scene.h"
#include "graphite/image.h"
#include "graphite/input.h"
#include "graphite/layout.h"
#include "graphite/style.h"
#include "graphite/text.h"
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
#include <vector>

class FPSTextUpdater : public TraitComponent<Text> {
  public:
    void updateComponent(Text *object) override {
        int fps = static_cast<int>(getWindow()->getFramesPerSecond());
        object->content = "FPS: " + std::to_string(fps);
    }
};

class MainScene : public Scene {
    CoreObject ground;
    CoreObject ball;
    CoreObject ball2;
    DirectionalLight light;
    Camera camera;
    CoreObject lightObject;
    Text fpsText;
    RenderTarget frameBuffer;
    AreaLight areaLight;
    Controller controller;
    Text welcomeText;
    Image previewImage;
    TextField inputField;
    Button actionButton;
    Checkbox demoCheckbox;
    Row headerRow;
    Row controlsRow;
    Column uiColumn;

    bool doesUpdate = true;
    bool fall = false;

  public:
    void update(Window &window) override {
        if (!doesUpdate)
            return;

        if (!inputField.isFocused()) {
            camera.updateWithActions(window, "move", "look", "upAndDown");
            if (window.isKeyActive(Key::Escape)) {
                window.releaseMouse();
                doesUpdate = false;
            } else if (window.isKeyPressed(Key::Q)) {
                fall = !fall;
            }
            if (fall) {
                camera.position.y -= 10.f * window.getDeltaTime();
            }
        }
    }

    void initialize(Window &window) override {
        Environment env;
        env.fog.intensity = 0.0;
        env.volumetricLighting.enabled = false;
        env.lightBloom.radius = 0.008f;
        env.lightBloom.maxSamples = 5;
        this->setEnvironment(env);

        auto moveAction = InputAction::createAxisInputAction(
            "move", {
                        AxisTrigger::custom(
                            Trigger::fromKey(Key::D), Trigger::fromKey(Key::A),
                            Trigger::fromKey(Key::W), Trigger::fromKey(Key::S)),
                    });

        window.addInputAction(moveAction);
        auto lookAction =
            InputAction::createAxisInputAction("look", {AxisTrigger::mouse()});
        window.addInputAction(lookAction);
        auto upAndDownAction = InputAction::createSingleAxisInputAction(
            "upAndDown", Trigger::fromKey(Key::Space),
            Trigger::fromKey(Key::LeftShift));
        window.addInputAction(upAndDownAction);

        Workspace::get().setRootPath(std::string(TEST_PATH) + "/resources/");

        camera = Camera();
        camera.setPosition({-5.0f, 1.0f, 2.0f});
        camera.lookAt({0.0f, 0.0f, 0.0f});
        camera.farClip = 1000.f;
        window.setCamera(&camera);

        ground = createBox({5.0f, 0.1f, 5.0f}, Color(0.8f, 0.8f, 0.8f));
        ground.attachTexture(
            Texture::fromResource(Workspace::get().createResource(
                "ground.jpg", "GroundTexture", ResourceType::Image)));
        ground.setPosition({0.0f, -0.1f, 0.0f});
        window.addObject(&ground);

        areaLight.position = {0.0f, 2.0f, 0.0f};
        areaLight.rotate({0.0f, 90.0f, 0.0f});
        areaLight.castsBothSides = true;
        areaLight.setColor({0.7f, 0.7f, 0.7f, 1.0f});
        this->addAreaLight(&areaLight);
        areaLight.createDebugObject();
        areaLight.addDebugObject(window);

        Resource fontResource = Workspace::get().createResource(
            "arial.ttf", "Arial", ResourceType::Font);

        fpsText = Text("FPS: 0", Font::fromResource("Arial", fontResource, 24),
                       Color::white());

        fpsText.addTraitComponent<Text>(FPSTextUpdater());

        welcomeText = Text("Welcome to Atlas!",
                           Font::fromResource("Arial", fontResource, 36),
                           Color(1.0f, 0.5f, 0.0f, 1.0f));

        previewImage = Image(
            Texture::fromResource(Workspace::get().createResource(
                "ground.jpg", "UIPreviewTexture", ResourceType::Image)),
            {.width = 320.0f, .height = 180.0f});

        inputField =
            TextField(fpsText.font, 420.0f, {.x = 0.0f, .y = 0.0f}, "",
                      "Type here...");
        inputField
            .setPadding({.width = 18.0f, .height = 12.0f})
            .setOnChange([](const TextFieldChangeEvent &event) {
                std::cout << event.text << std::endl;
            });

        actionButton = Button(fpsText.font, "Click me");
        actionButton
            .setMinimumSize({.width = 150.0f, .height = 52.0f})
            .setOnClick([](const ButtonClickEvent &event) {
                std::cout << "Button clicked: " << event.label << std::endl;
            });

        demoCheckbox = Checkbox(fpsText.font, "Enable atlas mode");
        demoCheckbox.setOnToggle([](const CheckboxToggleEvent &event) {
            std::cout << "Checkbox " << event.label << ": "
                      << (event.checked ? "true" : "false") << std::endl;
        });

        graphite::Theme theme;
        theme.text.normal().fontSize(28.0f).foreground(
            Color(0.93f, 0.95f, 0.99f, 1.0f));
        theme.image.normal()
            .padding(12.0f, 12.0f)
            .background(Color(0.05f, 0.08f, 0.12f, 0.55f))
            .border(2.0f, Color(1.0f, 1.0f, 1.0f, 0.12f))
            .cornerRadius(28.0f);
        theme.textField.normal()
            .font(fpsText.font)
            .fontSize(24.0f)
            .padding(20.0f, 14.0f)
            .foreground(Color(0.96f, 0.97f, 0.99f, 1.0f))
            .background(Color(0.06f, 0.08f, 0.12f, 0.92f))
            .border(2.0f, Color(1.0f, 1.0f, 1.0f, 0.12f))
            .cornerRadius(18.0f)
            .tint(Color(1.0f, 0.55f, 0.14f, 1.0f));
        theme.textField.focused()
            .background(Color(0.08f, 0.1f, 0.15f, 0.96f))
            .border(2.0f, Color(1.0f, 0.55f, 0.14f, 1.0f));
        theme.button.normal()
            .font(fpsText.font)
            .fontSize(23.0f)
            .padding(22.0f, 14.0f)
            .foreground(Color(0.98f, 0.98f, 0.99f, 1.0f))
            .background(Color(0.12f, 0.16f, 0.2f, 0.96f))
            .border(2.0f, Color(1.0f, 1.0f, 1.0f, 0.12f))
            .cornerRadius(18.0f);
        theme.button.hovered()
            .background(Color(0.16f, 0.21f, 0.26f, 0.98f))
            .border(2.0f, Color(0.86f, 0.9f, 0.97f, 0.48f));
        theme.button.pressed()
            .background(Color(1.0f, 0.55f, 0.14f, 0.96f))
            .foreground(Color(0.1f, 0.08f, 0.06f, 1.0f))
            .border(2.0f, Color(1.0f, 0.78f, 0.48f, 1.0f));
        theme.checkbox.normal()
            .font(fpsText.font)
            .fontSize(22.0f)
            .padding(0.0f, 6.0f)
            .foreground(Color(0.92f, 0.95f, 0.99f, 1.0f))
            .background(Color(0.08f, 0.11f, 0.15f, 0.96f))
            .border(2.0f, Color(1.0f, 1.0f, 1.0f, 0.12f))
            .cornerRadius(10.0f)
            .tint(Color(1.0f, 0.55f, 0.14f, 1.0f));
        theme.checkbox.hovered()
            .background(Color(0.12f, 0.15f, 0.2f, 0.98f))
            .border(2.0f, Color(0.9f, 0.93f, 0.98f, 0.42f));
        theme.checkbox.checked()
            .border(2.0f, Color(1.0f, 0.55f, 0.14f, 1.0f))
            .tint(Color(1.0f, 0.55f, 0.14f, 1.0f));
        theme.row.normal()
            .padding(18.0f, 16.0f)
            .background(Color(0.05f, 0.07f, 0.1f, 0.38f))
            .border(2.0f, Color(1.0f, 1.0f, 1.0f, 0.08f))
            .cornerRadius(24.0f);
        theme.column.normal()
            .padding(30.0f, 30.0f)
            .background(Color(0.02f, 0.04f, 0.07f, 0.26f))
            .border(2.0f, Color(1.0f, 1.0f, 1.0f, 0.08f))
            .cornerRadius(32.0f);
        theme.stack.normal()
            .padding(18.0f, 18.0f)
            .background(Color(0.03f, 0.05f, 0.08f, 0.42f))
            .border(2.0f, Color(1.0f, 1.0f, 1.0f, 0.08f))
            .cornerRadius(24.0f);
        graphite::Theme::set(theme);

        welcomeText.style()
            .normal()
            .fontSize(54.0f)
            .foreground(Color(1.0f, 0.5f, 0.0f, 1.0f));
        fpsText.style()
            .normal()
            .fontSize(34.0f)
            .foreground(Color(0.95f, 0.97f, 1.0f, 1.0f));

        headerRow = Row({&welcomeText, &fpsText}, 30.0f);
        controlsRow = Row({&actionButton, &demoCheckbox}, 22.0f);
        uiColumn = Column(
            {&headerRow, &previewImage, &inputField, &controlsRow}, 20.0f,
                          {.width = 20.0f, .height = 20.0f},
                          {.x = 20.0f, .y = 20.0f});
        window.addUIObject(&uiColumn);

        ball = createDebugSphere(0.5f, 32, 32);
        ball.material.metallic = 1.0f;
        ball.material.roughness = 0.0f;
        ball.move({0.f, 1.0f, 1.0f});
        window.addObject(&ball);

        ball2 = createDebugSphere(0.5f, 32, 32);
        ball2.move({0.f, 1.0f, -1.0f});
        window.addObject(&ball2);

        light = DirectionalLight({0.35f, -1.0f, 0.2f}, Color::white());
        this->setAmbientIntensity(0.2f);

        frameBuffer = RenderTarget(window, RenderTargetType::Multisampled);
        window.addRenderTarget(&frameBuffer);
        frameBuffer.display(window);

        this->setUseAtmosphereSkybox(true);

        atmosphere.enable();
        atmosphere.secondsPerHour = 4.f;
        atmosphere.setTime(12.0);
        atmosphere.cycle = false;
        atmosphere.wind = {0.1f, 0.0f, 0.0f};
    }
};

int main() {
    Window window({.title = "My Window",
                   .width = 1600,
                   .height = 1200,
                   .renderScale = 1.f,
                   .mouseCaptured = false,
                   .multisampling = false,
                   .ssaoScale = 0.4f});
    MainScene scene;
    window.setScene(&scene);
    window.run();
    return 0;
}
