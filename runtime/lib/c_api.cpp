#include "atlas/runtime/c_api.h"

#include "atlas/runtime/context.h"

#include <exception>

bool atlas_runtime_run_in_metal_view(const char *projectFile, void *metalView,
                                     void *sdlInputWindow) {
    if (projectFile == nullptr || projectFile[0] == '\0') {
        return false;
    }

    try {
        runtime::runProjectInMetalView(
            projectFile, metalView,
            reinterpret_cast<CoreWindowReference>(sdlInputWindow));
        return true;
    } catch (const std::exception &) {
        return false;
    } catch (...) {
        return false;
    }
}
