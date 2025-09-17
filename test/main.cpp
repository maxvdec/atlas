
#include "atlas/object.h"
#include <atlas/window.h>
#include <vector>

int main() {
    Window window({"My Window", 1600, 1200});
    std::vector<CoreVertex> triangle = {
        {{0.0, 0.5, 0.0}}, {{-0.5, -0.5, 0.0}}, {{0.5, -0.5, 0.0}}};
    CoreObject triangleObject;
    triangleObject.attachVertices(triangle);

    window.addObject(&triangleObject);
    window.run();
    return 0;
}
