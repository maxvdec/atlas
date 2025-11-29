
ENABLE_OPENGL := "OFF"

build:
    mkdir -p build
    cd build && cmake -DENABLE_OPENGL={{ENABLE_OPENGL}} ..
    cd build && make -j8

target target:
    mkdir -p build
    cd build && cmake -DENABLE_OPENGL={{ENABLE_OPENGL}} ..
    cd build && make -j8 {{target}}

run:
    just build
    ./build/bin/atlas_test

clangd:
    mkdir -p build
    cd build && cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DENABLE_OPENGL={{ENABLE_OPENGL}} ..
    ln -sf build/compile_commands.json compile_commands.json

lint:
    find atlas test \( -name '*.cpp' -o -name '*.h' \) -print0 | xargs -0 clang-format --dry-run --Werror

clean:
    rm -rf build compile_commands.json

frametest:
    just build
    timeout 2 ./build/bin/atlas_test

cli:
    cargo build

release:
    mkdir -p build
    cd build && cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_OPENGL={{ENABLE_OPENGL}} ..
    cd build && make -j8

docs:
    mkdir -p build
    doxygen -w html header.html delete.html delete.css
    rm delete.html delete.css
    cd build && cmake -DENABLE_OPENGL={{ENABLE_OPENGL}} ..
    cp header.html build/_deps/doxygen-awesome-css-src/header.html
    rm header.html
    doxygen Doxyfile

run-docs:
    just docs
    python3 -m http.server 8000 --directory docs/html