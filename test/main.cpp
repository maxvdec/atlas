
#include "atlas/object.h"
#include "atlas/units.h"
#include "atlas/workspace.h"
#include <atlas/window.h>
#include <iostream>
#include <vector>

int main() {
    Window window({"My Window", 1600, 1200});
    std::vector<CoreVertex> quad = {{{0.5, 0.5, 0.0}, Color::red()},
                                    {{0.5, -0.5, 0.0}, Color::green()},
                                    {{-0.5, -0.5, 0.0}, Color::blue()},
                                    {{-0.5, 0.5, 0.0}, Color::white()}};
    CoreObject quadObject;
    quadObject.attachVertices(quad);
    quadObject.attachIndices({0, 1, 3, 1, 2, 3});

    Workspace::get().setRootPath(std::filesystem::path(TEST_PATH));
    Resource texture_resource = Workspace::get().createResource(
        "resources/wall.jpg", "WallTexture", ResourceType::Image);
    std::cout << "Image loaded " << texture_resource.path << std::endl;

    window.addObject(&quadObject);
    window.run();
    return 0;
}
