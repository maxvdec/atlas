import os
import re
import subprocess
import sys

if len(sys.argv) < 3:
    print("Usage: python pack_shaders.py <input_dir> <output_file>")
    sys.exit(1)

input_dir = sys.argv[1]
output_file = sys.argv[2]

def make_var_name(filename):
    name = os.path.splitext(filename)[0]
    name = re.sub(r"(?<!^)(?=[A-Z])", "_", name)
    name = re.sub(r"[^0-9a-zA-Z_]", "_", name)
    return name.upper()

def detect_shader_stage(filename):
    """Detect shader stage from filename"""
    name_lower = filename.lower()
    if 'vert' in name_lower:
        return 'vertex'
    elif 'frag' in name_lower:
        return 'fragment'
    elif 'comp' in name_lower:
        return 'compute'
    elif 'geom' in name_lower:
        return 'geometry'
    return 'vertex'  # Default to vertex if unclear

with open(output_file, "w") as out:
    out.write("// This file contains packed shader source code compiled with Slang.\n")
    out.write("#ifndef ATLAS_GENERATED_SHADERS_H\n")
    out.write("#define ATLAS_GENERATED_SHADERS_H\n\n")
    
    for root, dirs, files in os.walk(input_dir):
        if "lib" in dirs:
            dirs.remove("lib")
        
        for filename in files:
            if not filename.endswith('.slang'):
                continue
                
            path = os.path.join(root, filename)
            if not os.path.isfile(path):
                continue
            
            var_name = make_var_name(filename)
            shader_dir = os.path.dirname(path)
            include_dir = os.path.join(shader_dir, "include")
            
            # CRITICAL: Detect shader stage
            stage = detect_shader_stage(filename)
            
            slangc_cmd = [
                "slangc",
                "-target", "glsl_410",
                "-profile", "glsl_410",
                "-stage", stage,  # THIS IS REQUIRED!
            ]
            
            if os.path.isdir(include_dir):
                slangc_cmd.extend(["-I", include_dir])
            
            slangc_cmd.extend([
                path,
                "-entry", "main",
            ])
            
            print(f"Compiling {filename} as {stage} shader...")
            
            try:
                compiled_shader = subprocess.run(
                    slangc_cmd,
                    capture_output=True,
                    text=True,
                    check=True,
                ).stdout
            except subprocess.CalledProcessError as e:
                print(f"Error compiling {filename}: {e.stderr}")
                compiled_shader = ""
            
            out.write(f'static const char* {var_name} = R"(\n{compiled_shader}\n)";\n\n')
    
    out.write("#endif // ATLAS_GENERATED_SHADERS_H\n")