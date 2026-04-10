import { Position2d } from "atlas/units";

const constants = globalThis.__atlasGetInputConstants?.() ?? {};

export const Key = Object.freeze(constants.Key ?? {});
export const MouseButton = Object.freeze(constants.MouseButton ?? {});
export const TriggerType = Object.freeze(constants.TriggerType ?? {});
export const AxisTriggerType = Object.freeze(constants.AxisTriggerType ?? {});

export class Trigger {
    constructor() {
        this.type = TriggerType.Key ?? 0;
        this.mouseButton = undefined;
        this.key = undefined;
        this.controllerButton = undefined;
    }

    static fromKey(key) {
        const trigger = new Trigger();
        trigger.type = TriggerType.Key ?? 1;
        trigger.key = key;
        return trigger;
    }

    static fromMouseButton(mouseButton) {
        const trigger = new Trigger();
        trigger.type = TriggerType.MouseButton ?? 0;
        trigger.mouseButton = mouseButton;
        return trigger;
    }

    static fromControllerButton(controllerID, buttonIndex) {
        const trigger = new Trigger();
        trigger.type = TriggerType.ControllerButton ?? 2;
        trigger.controllerButton = { controllerID, buttonIndex };
        return trigger;
    }
}

export class AxisTrigger {
    constructor() {
        this.type = AxisTriggerType.MouseAxis ?? 0;
        this.positiveX = Trigger.fromKey(Key.Unknown ?? 0);
        this.negativeX = Trigger.fromKey(Key.Unknown ?? 0);
        this.positiveY = Trigger.fromKey(Key.Unknown ?? 0);
        this.negativeY = Trigger.fromKey(Key.Unknown ?? 0);
        this.controllerId = undefined;
        this.controllerAxisSingle = false;
        this.axisIndex = undefined;
        this.axisIndexY = -1;
        this.isJoystick = false;
    }

    static fromMouse() {
        const trigger = new AxisTrigger();
        trigger.type = AxisTriggerType.MouseAxis ?? 0;
        return trigger;
    }

    static fromKeys(positiveX, negativeX, positiveY, negativeY) {
        const trigger = new AxisTrigger();
        trigger.type = AxisTriggerType.KeyCustom ?? 1;
        trigger.positiveX = Trigger.fromKey(positiveX);
        trigger.negativeX = Trigger.fromKey(negativeX);
        trigger.positiveY = Trigger.fromKey(positiveY);
        trigger.negativeY = Trigger.fromKey(negativeY);
        return trigger;
    }

    static fromControllerAxis(
        controllerId,
        axisIndex,
        single,
        axisIndexY = -1,
    ) {
        const trigger = new AxisTrigger();
        trigger.type = AxisTriggerType.ControllerAxis ?? 2;
        trigger.controllerId = controllerId;
        trigger.controllerAxisSingle = single;
        trigger.axisIndex = axisIndex;
        trigger.axisIndexY = axisIndexY;
        return trigger;
    }
}

export class InputAction {
    constructor() {
        this.triggers = [];
        this.axisTriggers = [];
        this.name = "";
        this.isAxis = false;
        this.isAxisSingle = false;
        this.normalized = false;
        this.invertY = false;
        this.controllerDeadzone = 0.2;
    }

    static createButtonAction(name, triggers) {
        const action = new InputAction();
        action.name = name;
        action.triggers = triggers;
        return action;
    }

    static createAxisAction(name, axisTriggers) {
        const action = new InputAction();
        action.name = name;
        action.isAxis = true;
        action.axisTriggers = axisTriggers;
        return action;
    }

    static createSingleAxisAction(name, positiveTrigger, negativeTrigger) {
        const action = new InputAction();
        action.name = name;
        action.isAxis = true;
        action.isAxisSingle = true;
        action.axisTriggers = [
            {
                type: AxisTriggerType.KeyCustom ?? 1,
                positiveX: positiveTrigger,
                negativeX: negativeTrigger,
                positiveY: Trigger.fromKey(Key.Unknown ?? 0),
                negativeY: Trigger.fromKey(Key.Unknown ?? 0),
                controllerId: undefined,
                controllerAxisSingle: false,
                axisIndex: undefined,
                axisIndexY: -1,
                isJoystick: false,
            },
        ];
        return action;
    }
}

export const Input = {
    addAction(action) {
        return globalThis.__atlasRegisterInputAction(action) ?? action;
    },

    resetActions() {
        globalThis.__atlasResetInputActions();
    },

    isKeyActive(key) {
        return globalThis.__atlasIsKeyActive(key);
    },

    isKeyPressed(key) {
        return globalThis.__atlasIsKeyPressed(key);
    },

    isMouseButtonActive(button) {
        return globalThis.__atlasIsMouseButtonActive(button);
    },

    isMouseButtonPressed(button) {
        return globalThis.__atlasIsMouseButtonPressed(button);
    },

    getTextInput() {
        return globalThis.__atlasGetTextInput();
    },

    startTextInput() {
        globalThis.__atlasStartTextInput();
    },

    stopTextInput() {
        globalThis.__atlasStopTextInput();
    },

    isTextInputActive() {
        return globalThis.__atlasIsTextInputActive();
    },

    isControllerButtonPressed(controllerID, buttonIndex) {
        return globalThis.__atlasIsControllerButtonPressed(
            controllerID,
            buttonIndex,
        );
    },

    getControllerAxisValue(controllerID, axisIndex) {
        return globalThis.__atlasGetControllerAxisValue(controllerID, axisIndex);
    },

    getControllerAxisPairValue(controllerID, axisIndexX, axisIndexY) {
        const value = globalThis.__atlasGetControllerAxisPairValue(
            controllerID,
            axisIndexX,
            axisIndexY,
        );
        return new Position2d(value?.x ?? 0, value?.y ?? 0);
    },

    captureMouse() {
        globalThis.__atlasCaptureMouse();
    },

    releaseMouse() {
        globalThis.__atlasReleaseMouse();
    },

    getMousePosition() {
        const position = globalThis.__atlasGetMousePosition();
        return new Position2d(position?.x ?? 0, position?.y ?? 0);
    },

    isActionTriggered(name) {
        return globalThis.__atlasIsActionTriggered(name);
    },

    isActionCurrentlyActive(name) {
        return globalThis.__atlasIsActionCurrentlyActive(name);
    },

    getAxisActionValue(name) {
        return globalThis.__atlasGetAxisActionValue(name);
    },
};

export class Interactive {
    constructor() {
        globalThis.__atlasRegisterInteractive(this);
    }

    onKeyPress(key, dt) {}
    onKeyRelease(key, dt) {}
    onMouseMove(packet, dt) {}
    onMouseButtonPress(button, dt) {}
    onMouseScroll(packet, dt) {}
    onEachFrame(dt) {}
}
