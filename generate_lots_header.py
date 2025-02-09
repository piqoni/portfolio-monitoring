import os

yaml_file = "lots.yaml"
output_file = "src/lots.h"

with open(yaml_file, "r") as f:
    yaml_content = f.read()

yaml_content = yaml_content.replace('"', '\\"')  # Escape quotes
yaml_lines = yaml_content.splitlines()
yaml_string = "\\n".join(yaml_lines)

with open(output_file, "w") as f:
    f.write(f'const char* LOTS_YAML = "{yaml_string}";\n')

print(f"Embedded {yaml_file} into {output_file}")