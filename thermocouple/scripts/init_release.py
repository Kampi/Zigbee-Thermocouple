import glob
import json
import os
import re
import sys
import yaml

def extract_version(branch_name: str) -> str:
    match = re.match(r"(\d+\.\d+\.\d+)_Dev", branch_name)
    if not match:
        raise ValueError(f"Invalid branch format: {branch_name}")
    return match.group(1)

def load_first_yaml_doc(file_path: str) -> dict:
    """Parse only the first YAML document, safely handling multi-document files
    that use '...' as a document-end marker without a following '---'."""
    if not os.path.exists(file_path):
        return {}
    with open(file_path, "r") as f:
        content = f.read()
    first_doc = content.split("\n...\n", 1)[0]
    return yaml.safe_load(first_doc) or {}

def entry_exists(yaml_file: str, version: str) -> bool:
    data = load_first_yaml_doc(yaml_file)
    entries = data.get("preflight", {}).get("set_text_variables", [])
    for entry in entries:
        if isinstance(entry, dict):
            if entry.get("variable") == f"@RELEASE_TITLE_VAR@{version}":
                return True
    return False

def add_release_entries(yaml_file: str, version: str):
    with open(yaml_file, "r") as f:
        content = f.read()

    new_entries = (
        f"    - variable: '@RELEASE_TITLE_VAR@{version}'\n"
        f"      command: '@GET_TITLE_CMD@ {version}'\n"
        f"    - variable: '@RELEASE_BODY_VAR@{version}'\n"
        f"      command: '@GET_BODY_CMD@ {version}'\n"
        f"\n"
    )

    marker = "    - variable: '@RELEASE_TITLE_VAR@UNRELEASED'"
    if marker not in content:
        raise ValueError(f"Could not find UNRELEASED insertion point in {yaml_file}")

    content = content.replace(marker, new_entries + marker, 1)

    with open(yaml_file, "w") as f:
        f.write(content)

def update_kibot_variant(pcb_workflow_file: str, variant: str) -> bool:
    if not os.path.exists(pcb_workflow_file):
        raise FileNotFoundError(f"Workflow file not found: {pcb_workflow_file}")
    with open(pcb_workflow_file, "r") as f:
        content = f.read()
    new_content, count = re.subn(
        r"(^  kibot_variant:\s*)\S+",
        rf"\g<1>{variant}",
        content,
        flags=re.MULTILINE,
    )
    if count == 0:
        raise ValueError(f"Could not find 'kibot_variant' in {pcb_workflow_file}")
    with open(pcb_workflow_file, "w") as f:
        f.write(new_content)
    return True

def find_kicad_pro(kibot_input_dir: str) -> str:
    matches = glob.glob(os.path.join(kibot_input_dir, "*.kicad_pro"))
    if not matches:
        raise FileNotFoundError(f"No .kicad_pro file found in {kibot_input_dir}")
    return matches[0]

def add_kicad_pro_variables(kicad_pro_file: str, version: str):
    with open(kicad_pro_file, "r") as f:
        data = json.load(f)

    text_vars = data.setdefault("text_variables", {})

    title_key = f"RELEASE_TITLE_{version}"
    body_key = f"RELEASE_BODY_{version}"

    if title_key in text_vars:
        return False

    text_vars[title_key] = ""
    text_vars[body_key] = ""

    with open(kicad_pro_file, "w") as f:
        json.dump(data, f, indent=2)
        f.write("\n")

    return True

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python init_release.py <branch_name> <kibot_input_dir>")
        sys.exit(1)

    branch_name = sys.argv[1]
    kibot_input_dir = sys.argv[2]
    version = extract_version(branch_name)

    print(f"Detected version: {version}")
    yaml_file = os.path.join(kibot_input_dir, "kibot_yaml", "kibot_pre_set_text_variables.yaml")
    print(f"Using YAML file: {yaml_file}")

    kicad_pro_file = find_kicad_pro(kibot_input_dir)
    print(f"Using KiCad project file: {kicad_pro_file}")

    if entry_exists(yaml_file, version):
        print(f"Entries for {version} already exist. Skipping.")
        sys.exit(0)

    add_release_entries(yaml_file, version)
    print(f"Added release entries to YAML for {version}")

    if add_kicad_pro_variables(kicad_pro_file, version):
        print(f"Added text variables to KiCad project for {version}")
    else:
        print(f"KiCad project variables for {version} already exist. Skipping.")

    pcb_workflow_file = os.path.join(".github", "workflows", "pcb.yml")
    update_kibot_variant(pcb_workflow_file, "PRELIMINARY")
    print(f"Set kibot_variant to PRELIMINARY in {pcb_workflow_file}")
