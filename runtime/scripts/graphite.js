import { Resource, ResourceType, UIObject } from "atlas";
import { Color, Position2d, Position3d, Size2d } from "atlas/units";

export const ElementAlignment = Object.freeze({
    Top: 0,
    Center: 1,
    Bottom: 2,
});

export const LayoutAnchor = Object.freeze({
    TopLeft: 0,
    TopCenter: 1,
    TopRight: 2,
    CenterLeft: 3,
    Center: 4,
    CenterRight: 5,
    BottomLeft: 6,
    BottomCenter: 7,
    BottomRight: 8,
});

export const UIStyleState = Object.freeze({
    Normal: 0,
    Hovered: 1,
    Pressed: 2,
    Disabled: 3,
    Focused: 4,
    Checked: 5,
});

function commitObject(object) {
    const updated = globalThis.__atlasUpdateObject(object);
    return updated == null ? object : updated;
}

function toPosition3d(position = Position2d.zero()) {
    return new Position3d(position.x, position.y, 0);
}

function makeDefaultStyle() {
    return new UIStyle();
}

function isPosition2dLike(value) {
    return (
        value != null &&
        typeof value === "object" &&
        !Array.isArray(value) &&
        typeof value.x === "number" &&
        typeof value.y === "number"
    );
}

function resolveCreated(value, fallback) {
    return value == null ? fallback : value;
}

export class UIStyleVariant {
    constructor() {
        this.paddingValue = undefined;
        this.cornerRadiusValue = undefined;
        this.borderWidthValue = undefined;
        this.backgroundColorValue = undefined;
        this.borderColorValue = undefined;
        this.foregroundColorValue = undefined;
        this.tintColorValue = undefined;
        this.fontValue = undefined;
        this.fontSizeValue = undefined;
    }

    padding(value) {
        this.paddingValue = value;
        return this;
    }

    cornerRadius(value) {
        this.cornerRadiusValue = value;
        return this;
    }

    borderWidth(value) {
        this.borderWidthValue = value;
        return this;
    }

    backgroundColor(value) {
        this.backgroundColorValue = value;
        return this;
    }

    borderColor(value) {
        this.borderColorValue = value;
        return this;
    }

    foregroundColor(value) {
        this.foregroundColorValue = value;
        return this;
    }

    tintColor(value) {
        this.tintColorValue = value;
        return this;
    }

    font(value) {
        this.fontValue = value;
        return this;
    }

    fontSize(value) {
        this.fontSizeValue = value;
        return this;
    }
}

export class UIStyle {
    constructor() {
        this.__normal = new UIStyleVariant();
        this.__hovered = new UIStyleVariant();
        this.__pressed = new UIStyleVariant();
        this.__disabled = new UIStyleVariant();
        this.__focused = new UIStyleVariant();
        this.__checked = new UIStyleVariant();
    }

    normal() {
        return this.__normal;
    }

    hovered() {
        return this.__hovered;
    }

    pressed() {
        return this.__pressed;
    }

    disabled() {
        return this.__disabled;
    }

    focused() {
        return this.__focused;
    }

    checked() {
        return this.__checked;
    }

    variant(state) {
        switch (state) {
            case UIStyleState.Normal:
                return this.__normal;
            case UIStyleState.Hovered:
                return this.__hovered;
            case UIStyleState.Pressed:
                return this.__pressed;
            case UIStyleState.Disabled:
                return this.__disabled;
            case UIStyleState.Focused:
                return this.__focused;
            case UIStyleState.Checked:
                return this.__checked;
            default:
                return this.__normal;
        }
    }
}

export class Theme {
    constructor() {
        this.text = new UIStyle();
        this.image = new UIStyle();
        this.textField = new UIStyle();
        this.button = new UIStyle();
        this.checkbox = new UIStyle();
        this.row = new UIStyle();
        this.column = new UIStyle();
        this.stack = new UIStyle();
    }

    static current() {
        return globalThis.__graphiteGetTheme();
    }

    static set(theme) {
        globalThis.__graphiteSetTheme(theme);
    }

    static reset() {
        globalThis.__graphiteResetTheme();
    }
}

export class Font {
    constructor() {
        this.name = "";
        this.atlas = null;
        this.size = 0;
        this.resource = new Resource(ResourceType.Font, "", "");
        this.texture = null;
    }

    static fromResource(resource) {
        return globalThis.__graphiteCreateFont(resource);
    }

    static getFont(name) {
        return globalThis.__graphiteGetFont(name);
    }

    changeSize(size) {
        return resolveCreated(globalThis.__graphiteChangeFontSize(this, size), this);
    }
}

export class Image extends UIObject {
    constructor(
        texture = null,
        size = Size2d.zero(),
        position = Position2d.zero(),
        tint = Color.white(),
    ) {
        super();
        this.texture = texture;
        this.position = toPosition3d(position);
        this.size = size;
        this.tint = tint;
        this.__style = null;
        return resolveCreated(globalThis.__graphiteCreateImage(this), this);
    }

    style() {
        if (this.__style == null) {
            this.__style = makeDefaultStyle();
        }
        return this.__style;
    }

    setStyle(style) {
        this.__style = style;
        return resolveCreated(globalThis.__graphiteSetUIObjectStyle(this, style), this);
    }

    setTexture(texture) {
        this.texture = texture;
        return commitObject(this);
    }

    setSize(size) {
        this.size = size;
        return commitObject(this);
    }
}

export class TextField extends UIObject {
    constructor(
        font = null,
        maximumWidth = 320,
        position = Position2d.zero(),
        text = "",
        placeholder = "",
    ) {
        super();
        this.text = text;
        this.placeholder = placeholder;
        this.font = font;
        this.position = toPosition3d(position);
        this.fontSize = 0;
        this.padding = new Size2d(14, 10);
        this.maximumWidth = maximumWidth;
        this.textColor = Color.white();
        this.placeholderColor = new Color(1, 1, 1, 0.45);
        this.backgroundColor = new Color(0.08, 0.09, 0.12, 0.94);
        this.borderColor = new Color(1, 1, 1, 0.15);
        this.focusedBorderColor = new Color(1, 0.55, 0.14, 1);
        this.cursorColor = Color.white();
        this.__style = null;
        return resolveCreated(globalThis.__graphiteCreateTextField(this), this);
    }

    getText() {
        return this.text;
    }

    isFocused() {
        return globalThis.__graphiteTextFieldIsFocused(this);
    }

    getCursorIndex() {
        return globalThis.__graphiteTextFieldGetCursorIndex(this);
    }

    style() {
        if (this.__style == null) {
            this.__style = makeDefaultStyle();
        }
        return this.__style;
    }

    setText(text) {
        this.text = text;
        return commitObject(this);
    }

    setPlaceholder(placeholder) {
        this.placeholder = placeholder;
        return commitObject(this);
    }

    setPadding(padding) {
        this.padding = padding;
        return commitObject(this);
    }

    setMaximumWidth(width) {
        this.maximumWidth = width;
        return commitObject(this);
    }

    setFontSize(size) {
        this.fontSize = size;
        return commitObject(this);
    }

    setStyle(style) {
        this.__style = style;
        return resolveCreated(globalThis.__graphiteSetUIObjectStyle(this, style), this);
    }

    setOnChange(callback) {
        this.__graphiteOnChange = callback;
        return this;
    }

    focus() {
        globalThis.__graphiteTextFieldFocus(this);
    }

    blur() {
        globalThis.__graphiteTextFieldBlur(this);
    }
}

export class Button extends UIObject {
    constructor(font = null, label = "", position = Position2d.zero()) {
        super();
        this.label = label;
        this.font = font;
        this.position = toPosition3d(position);
        this.fontSize = 0;
        this.padding = new Size2d(18, 12);
        this.minimumSize = Size2d.zero();
        this.textColor = Color.white();
        this.backgroundColor = new Color(0.15, 0.16, 0.2, 0.96);
        this.hoverBackgroundColor = new Color(0.2, 0.22, 0.27, 0.98);
        this.pressedBackgroundColor = new Color(1, 0.55, 0.14, 0.96);
        this.borderColor = new Color(1, 1, 1, 0.16);
        this.hoverBorderColor = new Color(1, 0.55, 0.14, 1);
        this.enabled = true;
        this.__style = null;
        return resolveCreated(globalThis.__graphiteCreateButton(this), this);
    }

    getLabel() {
        return this.label;
    }

    isHovered() {
        return globalThis.__graphiteButtonIsHovered(this);
    }

    isEnabled() {
        return this.enabled;
    }

    style() {
        if (this.__style == null) {
            this.__style = makeDefaultStyle();
        }
        return this.__style;
    }

    setLabel(label) {
        this.label = label;
        return commitObject(this);
    }

    setPadding(padding) {
        this.padding = padding;
        return commitObject(this);
    }

    setMinimumSize(size) {
        this.minimumSize = size;
        return commitObject(this);
    }

    setFontSize(size) {
        this.fontSize = size;
        return commitObject(this);
    }

    setStyle(style) {
        this.__style = style;
        return resolveCreated(globalThis.__graphiteSetUIObjectStyle(this, style), this);
    }

    setOnClick(callback) {
        this.__graphiteOnClick = callback;
        return this;
    }

    setEnabled(enabled) {
        this.enabled = enabled;
        commitObject(this);
    }
}

export class Checkbox extends UIObject {
    constructor(font = null, label = "", position = Position2d.zero()) {
        super();
        this.label = label;
        this.font = font;
        this.position = toPosition3d(position);
        this.fontSize = 0;
        this.padding = new Size2d(8, 8);
        this.boxSize = 18;
        this.spacing = 10;
        this.checked = false;
        this.enabled = true;
        this.textColor = Color.white();
        this.boxBackgroundColor = new Color(0.12, 0.13, 0.17, 0.95);
        this.hoverBoxBackgroundColor = new Color(0.18, 0.19, 0.24, 0.98);
        this.borderColor = new Color(1, 1, 1, 0.16);
        this.activeBorderColor = new Color(1, 0.55, 0.14, 1);
        this.checkColor = new Color(1, 0.55, 0.14, 1);
        this.__style = null;
        return resolveCreated(globalThis.__graphiteCreateCheckbox(this), this);
    }

    getLabel() {
        return this.label;
    }

    isChecked() {
        return this.checked;
    }

    isHovered() {
        return globalThis.__graphiteCheckboxIsHovered(this);
    }

    isEnabled() {
        return this.enabled;
    }

    style() {
        if (this.__style == null) {
            this.__style = makeDefaultStyle();
        }
        return this.__style;
    }

    setLabel(label) {
        this.label = label;
        return commitObject(this);
    }

    setPadding(padding) {
        this.padding = padding;
        return commitObject(this);
    }

    setFontSize(size) {
        this.fontSize = size;
        return commitObject(this);
    }

    setBoxSize(size) {
        this.boxSize = size;
        return commitObject(this);
    }

    setSpacing(spacing) {
        this.spacing = spacing;
        return commitObject(this);
    }

    setStyle(style) {
        this.__style = style;
        return resolveCreated(globalThis.__graphiteSetUIObjectStyle(this, style), this);
    }

    setOnToggle(callback) {
        this.__graphiteOnToggle = callback;
        return this;
    }

    setChecked(checked) {
        this.checked = checked;
        commitObject(this);
    }

    setEnabled(enabled) {
        this.enabled = enabled;
        commitObject(this);
    }

    toggle() {
        return globalThis.__graphiteCheckboxToggle(this);
    }
}

export class Column extends UIObject {
    constructor(
        children = [],
        spacing = 0,
        padding = Size2d.zero(),
        position = Position2d.zero(),
    ) {
        super();
        if (isPosition2dLike(children)) {
            position = children;
            children = [];
        }
        this.spacing = spacing;
        this.maxSize = Size2d.zero();
        this.padding = padding;
        this.children = children;
        this.position = toPosition3d(position);
        this.alignment = ElementAlignment.Top;
        this.anchor = LayoutAnchor.TopLeft;
        this.style = makeDefaultStyle();
        return resolveCreated(globalThis.__graphiteCreateColumn(this), this);
    }

    addChild(child) {
        this.children.push(child);
        commitObject(this);
    }

    setChildren(children) {
        this.children = children;
        commitObject(this);
    }

    setStyle(style) {
        this.style = style;
        return commitObject(this);
    }
}

export class Row extends UIObject {
    constructor(
        children = [],
        spacing = 0,
        padding = Size2d.zero(),
        position = Position2d.zero(),
    ) {
        super();
        if (isPosition2dLike(children)) {
            position = children;
            children = [];
        }
        this.spacing = spacing;
        this.maxSize = Size2d.zero();
        this.padding = padding;
        this.children = children;
        this.position = toPosition3d(position);
        this.alignment = ElementAlignment.Center;
        this.anchor = LayoutAnchor.TopLeft;
        this.style = makeDefaultStyle();
        return resolveCreated(globalThis.__graphiteCreateRow(this), this);
    }

    addChild(child) {
        this.children.push(child);
        commitObject(this);
    }

    setChildren(children) {
        this.children = children;
        commitObject(this);
    }

    setStyle(style) {
        this.style = style;
        return commitObject(this);
    }
}

export class Stack extends UIObject {
    constructor(
        children = [],
        padding = Size2d.zero(),
        position = Position2d.zero(),
    ) {
        super();
        if (isPosition2dLike(children)) {
            position = children;
            children = [];
        }
        this.maxSize = Size2d.zero();
        this.padding = padding;
        this.children = children;
        this.position = toPosition3d(position);
        this.horizontalAlignment = ElementAlignment.Top;
        this.verticalAlignment = ElementAlignment.Top;
        this.anchor = LayoutAnchor.TopLeft;
        this.style = makeDefaultStyle();
        return resolveCreated(globalThis.__graphiteCreateStack(this), this);
    }

    addChild(child) {
        this.children.push(child);
        commitObject(this);
    }

    setChildren(children) {
        this.children = children;
        commitObject(this);
    }

    setStyle(style) {
        this.style = style;
        return commitObject(this);
    }
}

export class Text extends UIObject {
    constructor(
        text = "",
        font = null,
        color = Color.white(),
        position = Position2d.zero(),
    ) {
        super();
        this.content = text;
        this.font = font;
        this.position = toPosition3d(position);
        this.fontSize = 0;
        this.color = color;
        this.__style = null;
        return resolveCreated(globalThis.__graphiteCreateText(this), this);
    }

    style() {
        if (this.__style == null) {
            this.__style = makeDefaultStyle();
        }
        return this.__style;
    }

    setStyle(style) {
        this.__style = style;
        return resolveCreated(globalThis.__graphiteSetUIObjectStyle(this, style), this);
    }

    setFontSize(size) {
        this.fontSize = size;
        return commitObject(this);
    }
}
