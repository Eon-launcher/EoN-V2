#!/usr/bin/env python3
"""Modified by EoN Launcher V2: verify manifest component classes in built APK DEX files."""

import os
import re
import subprocess
import sys
import tempfile
import zipfile
from pathlib import Path
from xml.etree import ElementTree


ANDROID_NS = "{http://schemas.android.com/apk/res/android}"
COMPONENT_TAGS = {"activity", "activity-alias", "application", "provider", "receiver", "service"}
MODULES = (
    (Path("app/src/main/AndroidManifest.xml"), "org.levimc.launcher"),
    (Path("minecraft/src/main/AndroidManifest.xml"), "com.mojang.minecraftpe"),
)
JNI_REQUIRED_CLASSES = {"com.mojang.minecraftpe.NotificationListenerService"}
DEX_CLASS = re.compile(r"Class descriptor\s*:\s*'([^']+)'")


def manifest_classes():
    classes = set()
    for manifest, default_package in MODULES:
        root = ElementTree.parse(manifest).getroot()
        package = root.get("package", default_package)
        application = root.find("application")
        if application is None:
            continue
        for component in application:
            if component.tag not in COMPONENT_TAGS:
                continue
            name = component.get(ANDROID_NS + "name")
            if not name or name.startswith("${"):
                continue
            if name.startswith("."):
                name = package + name
            elif "." not in name:
                name = package + "." + name
            classes.add(name)
    return classes


def dexdump_path():
    sdk = os.environ.get("ANDROID_SDK_ROOT") or os.environ.get("ANDROID_HOME")
    if not sdk:
        raise RuntimeError("ANDROID_SDK_ROOT or ANDROID_HOME must be set")
    tool = Path(sdk) / "build-tools" / "36.0.0" / "dexdump"
    if not tool.is_file():
        raise RuntimeError(f"Android build-tools dexdump not found: {tool}")
    return str(tool)


def classes_in_apk(apk, dexdump):
    found = set()
    with tempfile.TemporaryDirectory(prefix="manifest-dex-check-") as temp_dir:
        with zipfile.ZipFile(apk) as archive:
            dex_names = sorted(
                name for name in archive.namelist()
                if re.fullmatch(r"classes(?:[2-9]|[1-9][0-9]+)?\.dex", name)
            )
            if not dex_names:
                raise RuntimeError(f"No DEX files found in {apk}")
            for index, name in enumerate(dex_names):
                dex_path = Path(temp_dir) / f"classes-{index}.dex"
                dex_path.write_bytes(archive.read(name))
                result = subprocess.run(
                    [dexdump, "-f", str(dex_path)],
                    check=True,
                    capture_output=True,
                    text=True,
                )
                found.update(DEX_CLASS.findall(result.stdout))
    return found


def main():
    if len(sys.argv) < 2:
        raise SystemExit("Usage: check_manifest_classes.py APK [APK ...]")
    required = {
        "L" + class_name.replace(".", "/") + ";": class_name
        for class_name in manifest_classes() | JNI_REQUIRED_CLASSES
    }
    tool = dexdump_path()
    for apk in sys.argv[1:]:
        present = classes_in_apk(apk, tool)
        missing = sorted(name for descriptor, name in required.items() if descriptor not in present)
        if missing:
            raise SystemExit(f"{apk}: manifest classes missing from DEX: {', '.join(missing)}")
        print(f"{apk}: all {len(required)} manifest component classes are present in DEX")


if __name__ == "__main__":
    main()
