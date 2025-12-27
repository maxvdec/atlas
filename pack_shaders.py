import os
import sys
import subprocess
import tempfile

if len(sys.argv) < 3:
    print("Usage: python pack_shaders.py <input_dir> <output_file> [true|false]")
    sys.exit(1)

input_dir = sys.argv[1]
output_file = sys.argv[2]
vulkan = len(sys.argv) >= 4 and sys.argv[3].lower() == 'true'

MAX_CHUNK = 60000  # max characters per literal chunk


def compile_to_spirv(glsl_code, shader_path):
    """Compile GLSL to SPIR-V using glslangValidator or glslc"""
    ext = os.path.splitext(shader_path)[1].lower()
    stage_map = {
        '.vert': 'vert',
        '.frag': 'frag',
        '.comp': 'comp',
        '.geom': 'geom',
        '.tesc': 'tesc',
        '.tese': 'tese'
    }

    stage = stage_map.get(ext, 'vert')

    with tempfile.NamedTemporaryFile(mode='w', suffix=f'.{stage}', delete=False) as tmp_in:
        tmp_in.write(glsl_code)
        tmp_in_path = tmp_in.name

    tmp_out_path = tmp_in_path + '.spv'

    try:
        try:
            subprocess.run(
                ['glslangValidator', '-V', '-o', tmp_out_path, tmp_in_path],
                check=True,
                capture_output=True
            )
        except (subprocess.CalledProcessError, FileNotFoundError):
            subprocess.run(
                ['glslc', f'-fshader-stage={stage}', '-o', tmp_out_path, tmp_in_path],
                check=True,
                capture_output=True
            )

        with open(tmp_out_path, 'rb') as f:
            return f.read()

    except subprocess.CalledProcessError as e:
        print(f"Error compiling {shader_path}: {e}")
        print(f"stderr: {e.stderr.decode() if e.stderr else 'N/A'}")
        return None

    finally:
        if os.path.exists(tmp_in_path):
            os.remove(tmp_in_path)
        if os.path.exists(tmp_out_path):
            os.remove(tmp_out_path)


def write_chunks(out, var_name, contents):
    """Write large strings in chunks using raw string literals"""
    out.write(f'static const char* {var_name} =\n')

    if contents == "":
        out.write('R"()"\n')
    else:
        for i in range(0, len(contents), MAX_CHUNK):
            chunk = contents[i:i + MAX_CHUNK]
            out.write(f'R"({chunk})"\n')

    out.write(';\n\n')


with open(output_file, "w") as out:
    out.write("// This file contains packed shader source code.\n")
    if vulkan:
        out.write("// Shaders compiled to SPIR-V for Vulkan\n")
    out.write("#ifndef ATLAS_GENERATED_SHADERS_H\n")
    out.write("#define ATLAS_GENERATED_SHADERS_H\n\n")

    for root, _, files in os.walk(input_dir):
        for filename in files:
            path = os.path.join(root, filename)
            if not os.path.isfile(path):
                continue

            var_name = os.path.basename(filename) \
                .replace(".", "_") \
                .replace("-", "_") \
                .upper()

            with open(path, "r") as f:
                contents = f.read()

            if vulkan:
                if contents.strip() == "":
                    out.write(f'// {filename} is empty\n')
                    write_chunks(out, var_name, "")
                else:
                    spirv_bytes = compile_to_spirv(contents, path)
                    if spirv_bytes is not None:
                        hex_string = ''.join(f'{b:02x}' for b in spirv_bytes)
                        out.write(f'// Compiled from {filename} (SPIR-V as hex string)\n')
                        write_chunks(out, var_name, hex_string)
                    else:
                        print(f"Warning: Failed to compile {filename}, emitting empty shader")
                        write_chunks(out, var_name, "")
            else:
                write_chunks(out, var_name, contents)

    out.write("#endif // ATLAS_GENERATED_SHADERS_H\n")


print(f"Shaders packed successfully to {output_file}")
if vulkan:
    print("SPIR-V compilation enabled")
