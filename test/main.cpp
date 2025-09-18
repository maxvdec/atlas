
#include "atlas/object.h"
#include <atlas/window.h>
#include <vector>

int main() {
    Window window({"My Window", 1600, 1200});
    std::vector<CoreVertex> quad = {{{0.5, 0.5, 0.0}},
                                    {{0.5, -0.5, 0.0}},
                                    {{-0.5, -0.5, 0.0}},
                                    {{-0.5, 0.5, 0.0}}};
    CoreObject quadObject;
    quadObject.attachVertices(quad);
    quadObject.attachIndices({0, 1, 3, 1, 2, 3});

    window.addObject(&quadObject);
    window.run();
    return 0;
}
