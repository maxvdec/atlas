
build:
    cmake .
    make

run:
    just build
    ./bin/atlas_test

clangd:
    cmake . -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

lint:
   find atlas test \( -name '*.cpp' -o -name '*.h' \) -print0 | xargs -0 clang-format --dry-run --Werror

frametest:
    just build
    timeout 2 ./bin/atlas_test

cli:
    cargo build

release:
    cmake . -DCMAKE_BUILD_TYPE=Release
    make -j8

