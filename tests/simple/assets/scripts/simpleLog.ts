import { Component } from "atlas";
import { Debug } from "atlas/log";

export class SimpleLog extends Component {
    init() {
        // Called once when the script is initialized
        Debug.print("Hello from SimpleLog!");
    }

    update(deltaTime: number) {
        // Called every frame with the time elapsed since the last frame
    }
}
