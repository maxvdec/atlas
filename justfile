GENERATOR := "Ninja"

build backend="AUTO" bezel_native="OFF":
    mkdir -p build
    cd build && cmake -G "{{ GENERATOR }}" -DBACKEND={{ backend }} -DBEZEL_NATIVE={{ bezel_native }} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON .. 
    cd build && ninja -j8

target target backend="AUTO" bezel_native="OFF":
    mkdir -p build
    cd build && cmake -G "{{ GENERATOR }}" -DBACKEND={{ backend }} -DBEZEL_NATIVE={{ bezel_native }} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON .. 
    cd build && ninja -j8 {{ target }}

run backend="AUTO" bezel_native="OFF":
    just build {{ backend }} {{ bezel_native }}
    MTL_HUD_ENABLED=0 ./build/bin/atlas_test

debug backend="AUTO" bezel_native="OFF":
    just build {{ backend }} {{ bezel_native }}
    MTL_HUD_ENABLED=1 ./build/bin/atlas_test

clangd backend="AUTO" bezel_native="OFF":
    mkdir -p build
    cd build && cmake -G "{{ GENERATOR }}" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DBACKEND={{ backend }} -DBEZEL_NATIVE={{ bezel_native }} .. 
    ln -sf build/compile_commands.json compile_commands.json

lint:
    run-clang-tidy -p build -quiet -warnings-as-errors='*'

clean:
    rm -rf build include/atlas/core/default_shaders.h docs/html

frametest:
    just build
    timeout 2 ./build/bin/atlas_test

cli:
    cargo build

release-metal:
    rm -rf build/release-metal
    mkdir -p build/release-metal dist/release
    cd build/release-metal && cmake -G "{{ GENERATOR }}" -DCMAKE_BUILD_TYPE=Release -DBACKEND=METAL ../..
    cd build/release-metal && ninja -j8 
    JOLT_PATH=build/release-metal/lib/libJolt.a; if [ ! -f "$JOLT_PATH" ]; then for CAND in build/release-metal/_deps/joltphysics-build/lib/libJolt.a build/release-metal/_deps/joltphysics-build/lib/*/libJolt.a; do if [ -f "$CAND" ]; then JOLT_PATH="$CAND"; break; fi; done; fi; if [ ! -f "$JOLT_PATH" ]; then echo "Could not find libJolt.a for METAL release"; exit 1; fi; libtool -static -o dist/release/macOS-atlas-metal.a build/release-metal/lib/libatlas.a build/release-metal/lib/libbezel.a build/release-metal/lib/libfinewave.a build/release-metal/lib/libaurora.a build/release-metal/lib/libhydra.a build/release-metal/lib/libopal.a build/release-metal/lib/libphoton.a "$JOLT_PATH"

release-opengl:
    rm -rf build/release-opengl
    mkdir -p build/release-opengl dist/release
    cd build/release-opengl && cmake -G "{{ GENERATOR }}" -DCMAKE_BUILD_TYPE=Release -DBACKEND=OPENGL ../..
    cd build/release-opengl && ninja -j8 atlas bezel finewave aurora hydra opal photon
    JOLT_PATH=build/release-opengl/lib/libJolt.a; if [ ! -f "$JOLT_PATH" ]; then for CAND in build/release-opengl/_deps/joltphysics-build/lib/libJolt.a build/release-opengl/_deps/joltphysics-build/lib/*/libJolt.a; do if [ -f "$CAND" ]; then JOLT_PATH="$CAND"; break; fi; done; fi; if [ ! -f "$JOLT_PATH" ]; then echo "Could not find libJolt.a for OPENGL release"; exit 1; fi; libtool -static -o dist/release/macOS-atlas-opengl.a build/release-opengl/lib/libatlas.a build/release-opengl/lib/libbezel.a build/release-opengl/lib/libfinewave.a build/release-opengl/lib/libaurora.a build/release-opengl/lib/libhydra.a build/release-opengl/lib/libopal.a build/release-opengl/lib/libphoton.a "$JOLT_PATH"

release-vulkan:
    rm -rf build/release-vulkan
    mkdir -p build/release-vulkan dist/release
    cd build/release-vulkan && cmake -G "{{ GENERATOR }}" -DCMAKE_BUILD_TYPE=Release -DBACKEND=VULKAN ../..
    cd build/release-vulkan && ninja -j8 atlas bezel finewave aurora hydra opal photon
    JOLT_PATH=build/release-vulkan/lib/libJolt.a; if [ ! -f "$JOLT_PATH" ]; then for CAND in build/release-vulkan/_deps/joltphysics-build/lib/libJolt.a build/release-vulkan/_deps/joltphysics-build/lib/*/libJolt.a; do if [ -f "$CAND" ]; then JOLT_PATH="$CAND"; break; fi; done; fi; if [ ! -f "$JOLT_PATH" ]; then echo "Could not find libJolt.a for VULKAN release"; exit 1; fi; libtool -static -o dist/release/macOS-atlas-vulkan.a build/release-vulkan/lib/libatlas.a build/release-vulkan/lib/libbezel.a build/release-vulkan/lib/libfinewave.a build/release-vulkan/lib/libaurora.a build/release-vulkan/lib/libhydra.a build/release-vulkan/lib/libopal.a build/release-vulkan/lib/libphoton.a "$JOLT_PATH"

release backend="AUTO":
    if [ "{{ backend }}" = "AUTO" ] || [ "{{ backend }}" = "all" ] || [ "{{ backend }}" = "ALL" ]; then just release-metal && just release-opengl && just release-vulkan; elif [ "{{ backend }}" = "metal" ] || [ "{{ backend }}" = "METAL" ]; then just release-metal; elif [ "{{ backend }}" = "opengl" ] || [ "{{ backend }}" = "OPENGL" ]; then just release-opengl; elif [ "{{ backend }}" = "vulkan" ] || [ "{{ backend }}" = "VULKAN" ]; then just release-vulkan; else echo "Unknown backend '{{ backend }}'. Use AUTO|metal|opengl|vulkan" && exit 1; fi

docs backend="AUTO":
    mkdir -p build
    doxygen -w html header.html delete.html delete.css
    rm delete.html delete.css
    cd build && cmake -G "{{ GENERATOR }}" -DBACKEND={{ backend }} ..
    cp header.html build/_deps/doxygen-awesome-css-src/header.html
    rm header.html
    doxygen Doxyfile

run-docs:
    just docs
    python3 -m http.server 8000 --directory docs/html
