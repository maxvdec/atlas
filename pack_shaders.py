import os
import sys

input_dir = sys.argv[1]
output_file = sys.argv[2]

with open(output_file, "w") as out:
    out.write("// This file contains packed shader source code.\n")
    out.write("#ifndef ATLAS_GENERATED_SHADERS_H\n")
    out.write("#define ATLAS_GENERATED_SHADERS_H\n\n")
    for filename in os.listdir(input_dir):
        path = os.path.join(input_dir, filename)
        if not os.path.isfile(path):
            continue

        var_name = filename.replace(".", "_").replace("-", "_").upper()
        with open(path, "r") as f:
            contents = f.read()
            out.write(f'static const char* {var_name} = R"(\n{contents}\n)";\n\n')

    out.write("#endif // ATLAS_GENERATED_SHADERS_H\n")
