Import("env")
import subprocess
from pathlib import Path
import sys

def after_build(source, target, env):
    firmware_path = Path(str(target[0]))  # ruta del .bin generado
    project_dir = Path(env['PROJECT_DIR'])
    tools_dir = project_dir / "ota" / "tools"
    script_path = tools_dir / "sign_firmware.py"

    python_exe = sys.executable  # Ejecutable Python actual

    print(f"🚀 Ejecutando firma para {firmware_path.name}...")
    subprocess.check_call([python_exe, str(script_path), str(firmware_path)])

env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", after_build)
