GENERATOR := "Ninja"

build enable_opengl="OFF" bezel_native="OFF":
    mkdir -p build
    cd build && cmake -G "{{ GENERATOR }}" -DBACKEND_OPENGL={{ enable_opengl }} -DBEZEL_NATIVE={{ bezel_native }} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
    cd build && ninja

target target enable_opengl="OFF" bezel_native="OFF":
    mkdir -p build
    cd build && cmake -G "{{ GENERATOR }}" -DBACKEND_OPENGL={{ enable_opengl }} -DBEZEL_NATIVE={{ bezel_native }} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
    cd build && ninja {{ target }}

run enable_opengl="OFF" bezel_native="OFF":
    just build {{ enable_opengl }} {{ bezel_native }}
    MTL_HUD_ENABLED=0 ./build/bin/atlas_test

debug enable_opengl="OFF" bezel_native="OFF":
    just build {{ enable_opengl }} {{ bezel_native }}
    MTL_HUD_ENABLED=1 ./build/bin/atlas_test

clangd enable_opengl="OFF" bezel_native="OFF":
    mkdir -p build
    cd build && cmake -G "{{ GENERATOR }}" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DBACKEND_OPENGL={{ enable_opengl }} -DBEZEL_NATIVE={{ bezel_native }} ..
    ln -sf build/compile_commands.json compile_commands.json

lint:
    find atlas test \( -name '*.cpp' -o -name '*.h' \) -print0 | xargs -0 clang-format --dry-run --Werror

clean:
    rm -rf build include/atlas/core/default_shaders.h docs/html

frametest:
    just build
    timeout 2 ./build/bin/atlas_test

# Runs a short smoke test and fails if validation errors are printed.
check:
    just build
    rm -f build/atlas_check.log
    (timeout 5 ./build/bin/atlas_test 2>&1 | tee build/atlas_check.log) || true
    ! grep -E "\\[ERROR\\]|VUID-" build/atlas_check.log

cli:
    cargo build

release enable_opengl="OFF":
    mkdir -p build
    cd build && cmake -G "{{ GENERATOR }}" -DCMAKE_BUILD_TYPE=Release -DBACKEND_OPENGL={{ enable_opengl }} ..
    cd build && ninja

docs enable_opengl="OFF":
    mkdir -p build
    doxygen -w html header.html delete.html delete.css
    rm delete.html delete.css
    cd build && cmake -G "{{ GENERATOR }}" -DBACKEND_OPENGL={{ enable_opengl }} ..
    cp header.html build/_deps/doxygen-awesome-css-src/header.html
    rm header.html
    doxygen Doxyfile

run-docs:
    just docs
    python3 -m http.server 8000 --directory docs/html
