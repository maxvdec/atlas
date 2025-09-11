
build:
    cmake .
    make

run:
    just build
    ./bin/atlas_test

clangd:
    cmake . -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
