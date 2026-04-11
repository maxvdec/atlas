#ifndef ATLAS_RUNTIME_C_API_H
#define ATLAS_RUNTIME_C_API_H

#ifndef __cplusplus
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Runs an Atlas project using a host NSView as the Metal target.
 *
 * This entrypoint is intended for FFI consumers (for example Rust). It is
 * available only when Atlas is built with the Metal backend.
 *
 * @param projectFile Absolute or relative path to the .atlas project file.
 * @param metalView Host NSView pointer used for CAMetalLayer presentation.
 * @param sdlInputWindow Optional SDL_Window* cast to void*. This window can
 * be used to keep SDL input/audio integration active while rendering to
 * `metalView`.
 * @return `true` on success, `false` when startup fails.
 */
bool atlas_runtime_run_in_metal_view(const char *projectFile, void *metalView,
                                     void *sdlInputWindow);

#ifdef __cplusplus
}
#endif

#endif
