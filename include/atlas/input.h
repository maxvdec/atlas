/*
 input.hpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Input processing
 Copyright (c) 2025 maxvdec
*/

#ifndef ATLAS_INPUT_HPP
#define ATLAS_INPUT_HPP

#include <SDL3/SDL.h>
#include <memory>
#include <string>
#include <vector>

/**
 * @brief Enumeration of keyboard keys. Each key is a valid key in a keyboard.
 *
 */
enum class Key : int {
    Unknown = SDL_SCANCODE_UNKNOWN,
    Space = SDL_SCANCODE_SPACE,
    Apostrophe = SDL_SCANCODE_APOSTROPHE,
    Comma = SDL_SCANCODE_COMMA,
    Minus = SDL_SCANCODE_MINUS,
    Period = SDL_SCANCODE_PERIOD,
    Slash = SDL_SCANCODE_SLASH,
    Key0 = SDL_SCANCODE_0,
    Key1 = SDL_SCANCODE_1,
    Key2 = SDL_SCANCODE_2,
    Key3 = SDL_SCANCODE_3,
    Key4 = SDL_SCANCODE_4,
    Key5 = SDL_SCANCODE_5,
    Key6 = SDL_SCANCODE_6,
    Key7 = SDL_SCANCODE_7,
    Key8 = SDL_SCANCODE_8,
    Key9 = SDL_SCANCODE_9,
    Semicolon = SDL_SCANCODE_SEMICOLON,
    Equal = SDL_SCANCODE_EQUALS,
    A = SDL_SCANCODE_A,
    B = SDL_SCANCODE_B,
    C = SDL_SCANCODE_C,
    D = SDL_SCANCODE_D,
    E = SDL_SCANCODE_E,
    F = SDL_SCANCODE_F,
    G = SDL_SCANCODE_G,
    H = SDL_SCANCODE_H,
    I = SDL_SCANCODE_I,
    J = SDL_SCANCODE_J,
    K = SDL_SCANCODE_K,
    L = SDL_SCANCODE_L,
    M = SDL_SCANCODE_M,
    N = SDL_SCANCODE_N,
    O = SDL_SCANCODE_O,
    P = SDL_SCANCODE_P,
    Q = SDL_SCANCODE_Q,
    R = SDL_SCANCODE_R,
    S = SDL_SCANCODE_S,
    T = SDL_SCANCODE_T,
    U = SDL_SCANCODE_U,
    V = SDL_SCANCODE_V,
    W = SDL_SCANCODE_W,
    X = SDL_SCANCODE_X,
    Y = SDL_SCANCODE_Y,
    Z = SDL_SCANCODE_Z,
    LeftBracket = SDL_SCANCODE_LEFTBRACKET,
    Backslash = SDL_SCANCODE_BACKSLASH,
    RightBracket = SDL_SCANCODE_RIGHTBRACKET,
    GraveAccent = SDL_SCANCODE_GRAVE,
    Escape = SDL_SCANCODE_ESCAPE,
    Enter = SDL_SCANCODE_RETURN,
    Tab = SDL_SCANCODE_TAB,
    Backspace = SDL_SCANCODE_BACKSPACE,
    Insert = SDL_SCANCODE_INSERT,
    Delete = SDL_SCANCODE_DELETE,
    Right = SDL_SCANCODE_RIGHT,
    Left = SDL_SCANCODE_LEFT,
    Down = SDL_SCANCODE_DOWN,
    Up = SDL_SCANCODE_UP,
    PageUp = SDL_SCANCODE_PAGEUP,
    PageDown = SDL_SCANCODE_PAGEDOWN,
    Home = SDL_SCANCODE_HOME,
    End = SDL_SCANCODE_END,
    CapsLock = SDL_SCANCODE_CAPSLOCK,
    ScrollLock = SDL_SCANCODE_SCROLLLOCK,
    NumLock = SDL_SCANCODE_NUMLOCKCLEAR,
    PrintScreen = SDL_SCANCODE_PRINTSCREEN,
    Pause = SDL_SCANCODE_PAUSE,
    F1 = SDL_SCANCODE_F1,
    F2 = SDL_SCANCODE_F2,
    F3 = SDL_SCANCODE_F3,
    F4 = SDL_SCANCODE_F4,
    F5 = SDL_SCANCODE_F5,
    F6 = SDL_SCANCODE_F6,
    F7 = SDL_SCANCODE_F7,
    F8 = SDL_SCANCODE_F8,
    F9 = SDL_SCANCODE_F9,
    F10 = SDL_SCANCODE_F10,
    F11 = SDL_SCANCODE_F11,
    F12 = SDL_SCANCODE_F12,
    F13 = SDL_SCANCODE_F13,
    F14 = SDL_SCANCODE_F14,
    F15 = SDL_SCANCODE_F15,
    F16 = SDL_SCANCODE_F16,
    F17 = SDL_SCANCODE_F17,
    F18 = SDL_SCANCODE_F18,
    F19 = SDL_SCANCODE_F19,
    F20 = SDL_SCANCODE_F20,
    F21 = SDL_SCANCODE_F21,
    F22 = SDL_SCANCODE_F22,
    F23 = SDL_SCANCODE_F23,
    F24 = SDL_SCANCODE_F24,
    F25 = SDL_SCANCODE_UNKNOWN,
    KP0 = SDL_SCANCODE_KP_0,
    KP1 = SDL_SCANCODE_KP_1,
    KP2 = SDL_SCANCODE_KP_2,
    KP3 = SDL_SCANCODE_KP_3,
    KP4 = SDL_SCANCODE_KP_4,
    KP5 = SDL_SCANCODE_KP_5,
    KP6 = SDL_SCANCODE_KP_6,
    KP7 = SDL_SCANCODE_KP_7,
    KP8 = SDL_SCANCODE_KP_8,
    KP9 = SDL_SCANCODE_KP_9,
    KPDecimal = SDL_SCANCODE_KP_PERIOD,
    KPDivide = SDL_SCANCODE_KP_DIVIDE,
    KPMultiply = SDL_SCANCODE_KP_MULTIPLY,
    KPSubtract = SDL_SCANCODE_KP_MINUS,
    KPAdd = SDL_SCANCODE_KP_PLUS,
    KPEnter = SDL_SCANCODE_KP_ENTER,
    KPEqual = SDL_SCANCODE_KP_EQUALS,
    LeftShift = SDL_SCANCODE_LSHIFT,
    LeftControl = SDL_SCANCODE_LCTRL,
    LeftAlt = SDL_SCANCODE_LALT,
    LeftSuper = SDL_SCANCODE_LGUI,
    RightShift = SDL_SCANCODE_RSHIFT,
    RightControl = SDL_SCANCODE_RCTRL,
    RightAlt = SDL_SCANCODE_RALT,
    RightSuper = SDL_SCANCODE_RGUI,
    Menu = SDL_SCANCODE_APPLICATION
};

/**
 * @brief Enumeration of mouse buttons. Each button represents a physical mouse
 * button that can be pressed.
 *
 */
enum class MouseButton : int {
    /**
     * @brief Generic mouse button 1.
     *
     */
    Button1 = SDL_BUTTON_LEFT,
    /**
     * @brief Generic mouse button 2.
     *
     */
    Button2 = SDL_BUTTON_RIGHT,
    /**
     * @brief Generic mouse button 3.
     *
     */
    Button3 = SDL_BUTTON_MIDDLE,
    /**
     * @brief Generic mouse button 4.
     *
     */
    Button4 = SDL_BUTTON_X1,
    /**
     * @brief Generic mouse button 5.
     *
     */
    Button5 = SDL_BUTTON_X2,
    /**
     * @brief Generic mouse button 6.
     *
     */
    Button6 = 6,
    /**
     * @brief Generic mouse button 7.
     *
     */
    Button7 = 7,
    /**
     * @brief Generic mouse button 8.
     *
     */
    Button8 = 8,
    /**
     * @brief The last valid mouse button.
     *
     */
    Last = 8,
    /**
     * @brief The left mouse button (primary button).
     *
     */
    Left = SDL_BUTTON_LEFT,
    /**
     * @brief The right mouse button (secondary button).
     *
     */
    Right = SDL_BUTTON_RIGHT,
    /**
     * @brief The middle mouse button (scroll wheel button).
     *
     */
    Middle = SDL_BUTTON_MIDDLE
};

constexpr int CONTROLLER_AXIS_LEFT_X = 0;
constexpr int CONTROLLER_AXIS_LEFT_Y = 1;
constexpr int CONTROLLER_AXIS_RIGHT_X = 2;
constexpr int CONTROLLER_AXIS_RIGHT_Y = 3;
constexpr int CONTROLLER_AXIS_LEFT_TRIGGER = 4;
constexpr int CONTROLLER_AXIS_RIGHT_TRIGGER = 5;
constexpr int CONTROLLER_AXIS_LAST = CONTROLLER_AXIS_RIGHT_TRIGGER;

inline bool atlasIsControllerYAxis(int axisIndex) {
    return axisIndex == CONTROLLER_AXIS_LEFT_Y ||
           axisIndex == CONTROLLER_AXIS_RIGHT_Y;
}

inline SDL_GamepadAxis atlasToSDLGamepadAxis(int axisIndex) {
    switch (axisIndex) {
    case CONTROLLER_AXIS_LEFT_X:
        return SDL_GAMEPAD_AXIS_LEFTX;
    case CONTROLLER_AXIS_LEFT_Y:
        return SDL_GAMEPAD_AXIS_LEFTY;
    case CONTROLLER_AXIS_RIGHT_X:
        return SDL_GAMEPAD_AXIS_RIGHTX;
    case CONTROLLER_AXIS_RIGHT_Y:
        return SDL_GAMEPAD_AXIS_RIGHTY;
    case CONTROLLER_AXIS_LEFT_TRIGGER:
        return SDL_GAMEPAD_AXIS_LEFT_TRIGGER;
    case CONTROLLER_AXIS_RIGHT_TRIGGER:
        return SDL_GAMEPAD_AXIS_RIGHT_TRIGGER;
    default:
        return SDL_GAMEPAD_AXIS_INVALID;
    }
}

enum class TriggerType { MouseButton, Key, ControllerButton };

struct ControllerButtonTrigger {
    int controllerID;
    int buttonIndex;
};

struct Trigger {
    TriggerType type;
    union {
        MouseButton mouseButton;
        Key key;
        ControllerButtonTrigger controllerButton;
    };

    static Trigger fromKey(Key key) {
        return Trigger{.type = TriggerType::Key, .key = key};
    }

    static Trigger fromMouseButton(MouseButton button) {
        return Trigger{.type = TriggerType::MouseButton, .mouseButton = button};
    }

    static Trigger fromControllerButton(int controllerID, int buttonIndex) {
        return Trigger{.type = TriggerType::ControllerButton,
                       .controllerButton = {.controllerID = controllerID,
                                            .buttonIndex = buttonIndex}};
    }
};

enum class AxisTriggerType { MouseAxis, KeyCustom, ControllerAxis };

struct AxisTrigger {
    AxisTriggerType type;

    Trigger positiveX = {};
    Trigger negativeX = {};
    Trigger positiveY = {};
    Trigger negativeY = {};

    int controllerID = -1;
    bool controllerAxisSingle = false;
    int axisIndex = -1;
    int axisIndexY = -1;

    bool isJoystick = false;

    static AxisTrigger mouse() {
        return AxisTrigger{.type = AxisTriggerType::MouseAxis};
    }

    static AxisTrigger custom(Trigger positiveX, Trigger negativeX,
                              Trigger positiveY, Trigger negativeY) {
        return AxisTrigger{.type = AxisTriggerType::KeyCustom,
                           .positiveX = positiveX,
                           .negativeX = negativeX,
                           .positiveY = positiveY,
                           .negativeY = negativeY};
    }

    static AxisTrigger controller(int controllerID, int axisIndex,
                                  bool single = true, int axisIndexY = -1) {
        return AxisTrigger{.type = AxisTriggerType::ControllerAxis,
                           .controllerID = controllerID,
                           .controllerAxisSingle = single,
                           .axisIndex = axisIndex,
                           .axisIndexY = axisIndexY};
    }
};

struct AxisPacket {
    float deltaX = 0.f;
    float deltaY = 0.f;
    float x = 0.f;
    float y = 0.f;
    float valueX = 0.f;
    float valueY = 0.f;
    float inputDeltaX = 0.f;
    float inputDeltaY = 0.f;
    bool hasValueInput = false;
    bool hasDeltaInput = false;
};

/**
 * @brief Structure representing mouse movement data. It is automatically
 * handled and updated by the \ref Window class.
 *
 */
struct MousePacket {
    /**
     * @brief The current x position of the mouse cursor.
     *
     */
    float xpos = 0.f;
    /**
     * @brief The current y position of the mouse cursor.
     *
     */
    float ypos = 0.f;
    /**
     * @brief The change in x position since the last update.
     *
     */
    float xoffset = 0.f;
    /**
     * @brief The change in y position since the last update.
     *
     */
    float yoffset = 0.f;
    /**
     * @brief Whether the pitch (y-axis rotation) is constrained to prevent
     * the camera from flipping upside down.
     *
     */
    bool constrainPitch = true;
    /**
     * @brief Whether the mouse is being used for the first time.
     *
     */
    bool firstMouse = true;
};

/**
 * @brief Structure representing mouse scroll data. It is automatically handled
 * by the \ref Window class.
 *
 */
struct MouseScrollPacket {
    /**
     * @brief The scroll offset in the x direction.
     *
     */
    float xoffset;
    /**
     * @brief The scroll offset in the y direction.
     *
     */
    float yoffset;
};

class InputAction {
  public:
    static std::shared_ptr<InputAction>
    createButtonInputAction(const std::string &name,
                            const std::vector<Trigger> &triggers) {
        InputAction action;
        action.name = name;
        action.isAxis = false;
        action.buttonTriggers = triggers;
        return std::make_shared<InputAction>(action);
    }
    static std::shared_ptr<InputAction>
    createAxisInputAction(const std::string &name,
                          const std::vector<AxisTrigger> &triggers) {
        InputAction action;
        action.name = name;
        action.isAxis = true;
        action.axisTriggers = triggers;
        return std::make_shared<InputAction>(action);
    }

    static std::shared_ptr<InputAction>
    createSingleAxisInputAction(const std::string &name,
                                const Trigger &positiveTrigger,
                                const Trigger &negativeTrigger) {
        InputAction action;
        action.name = name;
        action.isAxis = true;
        action.isAxisSingle = true;
        action.axisTriggers.push_back(
            AxisTrigger::custom(positiveTrigger, negativeTrigger, {}, {}));
        return std::make_shared<InputAction>(action);
    }

    std::string name;
    bool isAxis = false;
    bool isAxisSingle = false;
    std::vector<Trigger> buttonTriggers;
    std::vector<AxisTrigger> axisTriggers;

    float axisX = 0.f;
    float axisY = 0.f;
    float axisDeltaX = 0.f;
    float axisDeltaY = 0.f;
    bool clampAxis = true;
    float axisClampMin = -1.f;
    float axisClampMax = 1.f;
    bool normalize2D = false;
    float controllerDeadzone = 0.2f;
    bool invertControllerY = false;
    float axisScaleX = 1.f;
    float axisScaleY = 1.f;
};

/**
 * @brief Abstract class that declares some object that can recieve input and
 * react to the changes in it.
 * \subsection interactive-example Example
 * ```cpp
 * class PlayerController : public Interactive {
 *  public:
 *    void onKeyPress(Key key, float deltaTime) override {
 *      if (key == Key::W) {
 *        // Move forward
 *      }
 *    }
 *    void onMouseMove(MousePacket data, float deltaTime) override {
 *      // Look around based on mouse movement
 *    }
 * };
 * ```
 *
 */
class Interactive {
  public:
    virtual ~Interactive() = default;
    /**
     * @brief Function that is called automatically when a key is pressed.
     *
     * @param key The key that was pressed.
     * @param deltaTime The time elapsed since the last frame.
     */
    virtual void onKeyPress([[maybe_unused]] Key key,
                            [[maybe_unused]] float deltaTime) {};
    /**
     * @brief Function that is called automatically when a key is released.
     *
     * @param key The key that was released.
     * @param deltaTime The time elapsed since the last frame.
     */
    virtual void onKeyRelease([[maybe_unused]] Key key,
                              [[maybe_unused]] float deltaTime) {};
    /**
     * @brief Function that is called automatically when the mouse is moved.
     *
     * @param data The mouse movement data. See \ref MousePacket.
     * @param deltaTime The time elapsed since the last frame.
     */
    virtual void onMouseMove([[maybe_unused]] MousePacket data,
                             [[maybe_unused]] float deltaTime) {};
    /**
     * @brief Function that is called automatically when a mouse button is
     * pressed.
     *
     * @param button The mouse button that was pressed.
     * @param deltaTime The time elapsed since the last frame.
     */
    virtual void onMouseButtonPress([[maybe_unused]] MouseButton button,
                                    [[maybe_unused]] float deltaTime) {};
    /**
     * @brief Function that is called automatically when the mouse is scrolled.
     *
     * @param data The mouse scroll data. See \ref MouseScrollPacket.
     * @param deltaTime The time elapsed since the last frame.
     */
    virtual void onMouseScroll([[maybe_unused]] MouseScrollPacket data,
                               [[maybe_unused]] float deltaTime) {};
    /**
     * @brief Function that is called automatically at each frame.
     *
     * @param deltaTime The time elapsed since the last frame.
     */
    virtual void atEachFrame([[maybe_unused]] float deltaTime) {};

    /**
     * @brief The last mouse data received. Updated automatically by the \ref
     * Window class.
     *
     */
    MousePacket lastMouseData = MousePacket();
};

#endif // ATLAS_INPUT_HPP
