export class Position3d {
    constructor(x = 0, y = 0, z = 0) {
        this.x = x;
        this.y = y;
        this.z = z;
    }

    static zero() {
        return new Position3d(0, 0, 0);
    }
    static down() {
        return new Position3d(0, -1, 0);
    }
    static up() {
        return new Position3d(0, 1, 0);
    }
    static forward() {
        return new Position3d(0, 0, 1);
    }
    static back() {
        return new Position3d(0, 0, -1);
    }
    static right() {
        return new Position3d(1, 0, 0);
    }
    static left() {
        return new Position3d(-1, 0, 0);
    }
    static invalid() {
        return new Position3d(Number.NaN, Number.NaN, Number.NaN);
    }

    add(other) {
        if (typeof other === "number") {
            return new Position3d(
                this.x + other,
                this.y + other,
                this.z + other,
            );
        }
        return new Position3d(
            this.x + other.x,
            this.y + other.y,
            this.z + other.z,
        );
    }

    subtract(other) {
        if (typeof other === "number") {
            return new Position3d(
                this.x - other,
                this.y - other,
                this.z - other,
            );
        }
        return new Position3d(
            this.x - other.x,
            this.y - other.y,
            this.z - other.z,
        );
    }

    multiply(other) {
        if (typeof other === "number") {
            return new Position3d(
                this.x * other,
                this.y * other,
                this.z * other,
            );
        }
        return new Position3d(
            this.x * other.x,
            this.y * other.y,
            this.z * other.z,
        );
    }

    divide(other) {
        if (typeof other === "number") {
            return new Position3d(
                this.x / other,
                this.y / other,
                this.z / other,
            );
        }
        return new Position3d(
            this.x / other.x,
            this.y / other.y,
            this.z / other.z,
        );
    }

    is(other) {
        return this.x === other.x && this.y === other.y && this.z === other.z;
    }

    normalized() {
        const length = Math.hypot(this.x, this.y, this.z);
        if (length === 0) {
            return Position3d.zero();
        }
        return new Position3d(
            this.x / length,
            this.y / length,
            this.z / length,
        );
    }

    toString() {
        return `Position3d(${this.x}, ${this.y}, ${this.z})`;
    }
}

export class BoundingBox {
    constructor(min = Position3d.zero(), max = Position3d.zero()) {
        this.min = min;
        this.max = max;
    }

    toString() {
        return `BoundingBox(min: ${this.min.toString()}, max: ${this.max.toString()})`;
    }

    contains(point) {
        return (
            point.x >= this.min.x &&
            point.x <= this.max.x &&
            point.y >= this.min.y &&
            point.y <= this.max.y &&
            point.z >= this.min.z &&
            point.z <= this.max.z
        );
    }

    intersects(other) {
        return !(
            this.max.x < other.min.x ||
            this.min.x > other.max.x ||
            this.max.y < other.min.y ||
            this.min.y > other.max.y ||
            this.max.z < other.min.z ||
            this.min.z > other.max.z
        );
    }
}

export class Color {
    constructor(r = 0, g = 0, b = 0, a = 1) {
        this.r = r;
        this.g = g;
        this.b = b;
        this.a = a;
    }

    add(other) {
        if (typeof other === "number") {
            return new Color(
                this.r + other,
                this.g + other,
                this.b + other,
                this.a + other,
            );
        }
        return new Color(
            this.r + other.r,
            this.g + other.g,
            this.b + other.b,
            this.a + other.a,
        );
    }

    subtract(other) {
        if (typeof other === "number") {
            return new Color(
                this.r - other,
                this.g - other,
                this.b - other,
                this.a - other,
            );
        }

        return new Color(
            this.r - other.r,
            this.g - other.g,
            this.b - other.b,
            this.a - other.a,
        );
    }

    multiply(other) {
        if (typeof other === "number") {
            return new Color(
                this.r * other,
                this.g * other,
                this.b * other,
                this.a * other,
            );
        }
        return new Color(
            this.r * other.r,
            this.g * other.g,
            this.b * other.b,
            this.a * other.a,
        );
    }

    divide(other) {
        if (typeof other === "number") {
            return new Color(
                this.r / other,
                this.g / other,
                this.b / other,
                this.a / other,
            );
        }
        return new Color(
            this.r / other.r,
            this.g / other.g,
            this.b / other.b,
            this.a / other.a,
        );
    }

    static white() {
        return new Color(1, 1, 1, 1);
    }

    static black() {
        return new Color(0, 0, 0, 1);
    }

    static red() {
        return new Color(1, 0, 0, 1);
    }

    static green() {
        return new Color(0, 1, 0, 1);
    }

    static blue() {
        return new Color(0, 0, 1, 1);
    }

    static transparent() {
        return new Color(0, 0, 0, 0);
    }

    static yellow() {
        return new Color(1, 1, 0, 1);
    }

    static cyan() {
        return new Color(0, 1, 1, 1);
    }

    static magenta() {
        return new Color(1, 0, 1, 1);
    }

    static gray() {
        return new Color(0.5, 0.5, 0.5, 1);
    }

    static orange() {
        return new Color(1, 0.5, 0, 1);
    }

    static purple() {
        return new Color(0.5, 0, 0.5, 1);
    }

    static brown() {
        return new Color(0.6, 0.3, 0, 1);
    }

    static pink() {
        return new Color(1, 0.75, 0.8, 1);
    }

    static lime() {
        return new Color(0, 1, 0, 1);
    }

    static navy() {
        return new Color(0, 0, 0.5, 1);
    }

    static teal() {
        return new Color(0, 0.5, 0.5, 1);
    }

    static olive() {
        return new Color(0.5, 0.5, 0, 1);
    }

    static maroon() {
        return new Color(0.5, 0, 0, 1);
    }

    static fromHex(hex) {
        if (hex.startsWith("#")) {
            hex = hex.slice(1);
        }
        if (hex.length === 3) {
            hex = hex
                .split("")
                .map((c) => c + c)
                .join("");
        }
        if (hex.length !== 6) {
            throw new Error("Invalid hex color");
        }
        const r = parseInt(hex.slice(0, 2), 16) / 255;
        const g = parseInt(hex.slice(2, 4), 16) / 255;
        const b = parseInt(hex.slice(4, 6), 16) / 255;
        return new Color(r, g, b, 1);
    }

    static mix(color1, color2, t) {
        return new Color(
            color1.r * (1 - t) + color2.r * t,
            color1.g * (1 - t) + color2.g * t,
            color1.b * (1 - t) + color2.b * t,
            color1.a * (1 - t) + color2.a * t,
        );
    }
}

export class Position2d {
    constructor(x = 0, y = 0) {
        this.x = x;
        this.y = y;
    }

    static zero() {
        return new Position2d(0, 0);
    }

    static up() {
        return new Position2d(0, 1);
    }

    static down() {
        return new Position2d(0, -1);
    }

    static left() {
        return new Position2d(-1, 0);
    }

    static right() {
        return new Position2d(1, 0);
    }

    static invalid() {
        return new Position2d(NaN, NaN);
    }

    add(other) {
        if (typeof other === "number") {
            return new Position2d(this.x + other, this.y + other);
        }
        return new Position2d(this.x + other.x, this.y + other.y);
    }

    subtract(other) {
        if (typeof other === "number") {
            return new Position2d(this.x - other, this.y - other);
        }
        return new Position2d(this.x - other.x, this.y - other.y);
    }

    multiply(other) {
        if (typeof other === "number") {
            return new Position2d(this.x * other, this.y * other);
        }
        return new Position2d(this.x * other.x, this.y * other.y);
    }

    divide(other) {
        if (typeof other === "number") {
            return new Position2d(this.x / other, this.y / other);
        }
        return new Position2d(this.x / other.x, this.y / other.y);
    }

    is(other) {
        return this.x === other.x && this.y === other.y;
    }
}

export class Radians {
    constructor(value = 0) {
        this.value = value;
    }

    add(other) {
        return new Radians(this.value + other.value);
    }

    subtract(other) {
        return new Radians(this.value - other.value);
    }

    multiply(other) {
        if (typeof other === "number") {
            return new Radians(this.value * other);
        }
        return new Radians(this.value * other.value);
    }

    divide(other) {
        if (typeof other === "number") {
            return new Radians(this.value / other);
        }
        return new Radians(this.value / other.value);
    }

    toNumber() {
        return this.value;
    }

    static fromDegrees(degrees) {
        return new Radians((degrees * Math.PI) / 180);
    }

    toDegrees() {
        return (this.value * 180) / Math.PI;
    }
}

export class Size2d {
    constructor(width = 0, height = 0) {
        this.width = width;
        this.height = height;
    }

    toString() {
        return `Size2d(${this.width}, ${this.height})`;
    }

    add(other) {
        if (typeof other === "number") {
            return new Size2d(this.width + other, this.height + other);
        }
        return new Size2d(this.width + other.width, this.height + other.height);
    }

    subtract(other) {
        if (typeof other === "number") {
            return new Size2d(this.width - other, this.height - other);
        }
        return new Size2d(this.width - other.width, this.height - other.height);
    }

    multiply(other) {
        if (typeof other === "number") {
            return new Size2d(this.width * other, this.height * other);
        }
        return new Size2d(this.width * other.width, this.height * other.height);
    }

    divide(other) {
        if (typeof other === "number") {
            return new Size2d(this.width / other, this.height / other);
        }
        return new Size2d(this.width / other.width, this.height / other.height);
    }

    is(other) {
        return this.width === other.width && this.height === other.height;
    }

    static zero() {
        return new Size2d(0, 0);
    }
}
