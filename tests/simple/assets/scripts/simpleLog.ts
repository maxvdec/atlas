import { Component, CoreObject, GameObject } from "atlas";
import { RenderPassType, RenderTarget, RenderTargetType } from "atlas/graphics";
import { Input, Key } from "atlas/input";
import { Debug } from "atlas/log";
import { Position3d, Size2d } from "atlas/units";
import { Rigidbody } from "bezel";

export class SimpleLog extends Component {
    init() {
        // Called once when the script is initialized
        Debug.print("Hello from SimpleLog!");
        let halfSize = Size2d.zero();
        Debug.print(`Half size: ${halfSize.toString()}`);

        let rigidbody = this.getParent()
            .as(CoreObject)
            ?.getComponent(Rigidbody);
        rigidbody?.setMotionType("Dynamic");
    }

    update(deltaTime: number) {
        let parent = this.getObject("Cube");
        parent.setPosition(Position3d.zero());

        if (Input.isKeyPressed(Key.A)) {
            Debug.print("Key A is active!");
        }
    }
}
