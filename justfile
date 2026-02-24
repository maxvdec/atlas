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
    find atlas test \( -name '*.cpp' -o -name '*.h' \) -print0 | xargs -0 clang-format --dry-run --Werror
    run-clang-tidy -p build -quiet -warnings-as-errors='*'

clean:
    rm -rf build include/atlas/core/default_shaders.h docs/html

frametest:
    just build
    timeout 2 ./build/bin/atlas_test

cli:
    cargo build

release backend="AUTO":
    mkdir -p build
    cd build && cmake -G "{{ GENERATOR }}" -DCMAKE_BUILD_TYPE=Release -DBACKEND={{ backend }} .. 
    cd build && ninja -j8

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
