import subprocess
from pathlib import Path
import shutil

# Rutas relativas (ajusta según tu proyecto)
PRIVATE_KEY = Path(__file__).parent / "private.key"
OUTPUT_DIR = Path(__file__).parent.parent / "output"

def main():
    import sys
    if len(sys.argv) != 2:
        print("Uso: python sign_firmware.py <ruta_firmware.bin>")
        exit(1)

    firmware_path = Path(sys.argv[1])
    if not firmware_path.is_file():
        print(f"❌ Firmware no encontrado: {firmware_path}")
        exit(1)

    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    sig_path = OUTPUT_DIR / (firmware_path.stem + ".sig")
    bin_output_path = OUTPUT_DIR / firmware_path.name

    print("🔏 Generando firma...")

    cmd = [
        "openssl", "dgst", "-sha256",
        "-sign", str(PRIVATE_KEY),
        "-out", str(sig_path),
        str(firmware_path)
    ]

    try:
        subprocess.check_call(cmd)
    except FileNotFoundError:
        print("❌ Error: 'openssl' no está disponible. Verifica que esté instalado y en el PATH.")
        exit(1)
    except subprocess.CalledProcessError as e:
        print(f"❌ Error al ejecutar openssl: {e}")
        exit(1)

    print("📦 Copiando firmware...")
    shutil.copy2(firmware_path, bin_output_path)

    print(f"✅ Firma generada: {sig_path}")
    print(f"✅ Firmware copiado: {bin_output_path}")

if __name__ == "__main__":
    main()
