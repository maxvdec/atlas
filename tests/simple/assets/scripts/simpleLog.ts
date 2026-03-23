import { Component } from "atlas";
import { Debug } from "atlas/log";
import { Size2d } from "atlas/units";

export class SimpleLog extends Component {
    init() {
        // Called once when the script is initialized
        Debug.print("Hello from SimpleLog!");
        let halfSize = Size2d.zero();
        Debug.print(`Half size: ${halfSize.toString()}`);
    }

    update(deltaTime: number) {
        // Called every frame with the time elapsed since the last frame
    }
}
