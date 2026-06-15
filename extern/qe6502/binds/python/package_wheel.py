#!/usr/bin/env python3
"""Build a minimal qe6502 abi3 wheel from a staged Python package directory."""

from __future__ import annotations

import argparse
import base64
import csv
import hashlib
import os
import platform as host_platform
import re
import shutil
import sysconfig
import tempfile
import zipfile
from email.message import Message
from pathlib import Path


def normalize_distribution(name: str) -> str:
    return re.sub(r"[-_.]+", "_", name).lower()


def wheel_platform_tag() -> str:
    platform = sysconfig.get_platform()

    if platform.startswith("macosx-"):
        parts = platform.split("-")
        if len(parts) == 3 and parts[2] == "universal2":
            arch = host_platform.machine().lower()
            if arch in {"amd64", "x86-64", "x86_64"}:
                arch = "x86_64"
            elif arch in {"aarch64", "arm64"}:
                arch = "arm64"
            else:
                arch = parts[2]

            version = parts[1]
            if arch == "arm64":
                version_items = tuple(int(item) for item in version.split(".")[:2])
                if version_items < (11, 0):
                    version = "11.0"

            platform = f"macosx-{version}-{arch}"

    return platform.replace("-", "_").replace(".", "_")


def hash_file(path: Path) -> tuple[str, str]:
    data = path.read_bytes()
    digest = base64.urlsafe_b64encode(hashlib.sha256(data).digest()).rstrip(b"=").decode("ascii")
    return f"sha256={digest}", str(len(data))


def write_metadata(path: Path, args: argparse.Namespace, readme_text: str) -> None:
    message = Message()
    message["Metadata-Version"] = "2.4"
    message["Name"] = args.name
    message["Version"] = args.version
    message["Summary"] = args.description
    message["Author"] = args.author
    message["License"] = args.license
    message["Requires-Python"] = args.requires_python
    message["Description-Content-Type"] = "text/markdown"
    message["License-File"] = "licenses/LICENSE"

    if args.keywords:
        message["Keywords"] = ", ".join(args.keywords)

    for classifier in args.classifier:
        message["Classifier"] = classifier

    for label, url in args.project_url:
        message["Project-URL"] = f"{label}, {url}"

    path.write_text(message.as_string() + "\n" + readme_text, encoding="utf-8")


def copy_wheel_payload(src: Path, dst: Path) -> None:
    package_dir = src / "qe6502"

    if package_dir.is_dir():
        shutil.copytree(package_dir, dst / "qe6502", dirs_exist_ok=True)

    for entry in src.iterdir():
        if entry.is_file() and entry.name.startswith("_qe6502"):
            shutil.copy2(entry, dst / entry.name)


def build_wheel(args: argparse.Namespace) -> Path:
    stage_dir = args.stage_dir.resolve()
    dist_dir = args.dist_dir.resolve()
    dist_dir.mkdir(parents=True, exist_ok=True)

    readme_text = args.readme.read_text(encoding="utf-8")
    dist_info = f"{normalize_distribution(args.name)}-{args.version}.dist-info"
    wheel_tag = f"{args.python_tag}-{args.abi_tag}-{args.platform_tag or wheel_platform_tag()}"
    wheel_name = f"{args.name}-{args.version}-{wheel_tag}.whl"
    wheel_path = dist_dir / wheel_name

    with tempfile.TemporaryDirectory(prefix="qe6502-wheel-") as tmp_name:
        wheel_root = Path(tmp_name) / "wheel"
        wheel_root.mkdir(parents=True)
        copy_wheel_payload(stage_dir, wheel_root)

        dist_info_dir = wheel_root / dist_info
        dist_info_dir.mkdir(parents=True)
        license_dir = dist_info_dir / "licenses"
        license_dir.mkdir(parents=True)

        write_metadata(dist_info_dir / "METADATA", args, readme_text)
        (dist_info_dir / "WHEEL").write_text(
            "Wheel-Version: 1.0\n"
            "Generator: qe6502 package_wheel.py\n"
            "Root-Is-Purelib: false\n"
            f"Tag: {wheel_tag}\n",
            encoding="utf-8",
        )
        shutil.copy2(args.license_file, license_dir / "LICENSE")

        record_path = dist_info_dir / "RECORD"
        files = sorted(path for path in wheel_root.rglob("*") if path.is_file())

        rows: list[list[str]] = []
        for path in files:
            relative = path.relative_to(wheel_root).as_posix()
            if path == record_path:
                rows.append([relative, "", ""])
            else:
                digest, size = hash_file(path)
                rows.append([relative, digest, size])

        with record_path.open("w", newline="", encoding="utf-8") as record_file:
            writer = csv.writer(record_file)
            writer.writerows(rows)

        if wheel_path.exists():
            wheel_path.unlink()

        with zipfile.ZipFile(wheel_path, "w", compression=zipfile.ZIP_DEFLATED) as wheel:
            for path in sorted(wheel_root.rglob("*")):
                if path.is_file():
                    wheel.write(path, path.relative_to(wheel_root).as_posix())

    return wheel_path


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--stage-dir", type=Path, required=True)
    parser.add_argument("--dist-dir", type=Path, required=True)
    parser.add_argument("--name", required=True)
    parser.add_argument("--version", required=True)
    parser.add_argument("--description", required=True)
    parser.add_argument("--author", required=True)
    parser.add_argument("--license", dest="license", required=True)
    parser.add_argument("--license-file", type=Path, required=True)
    parser.add_argument("--readme", type=Path, required=True)
    parser.add_argument("--requires-python", required=True)
    parser.add_argument("--python-tag", default="cp310")
    parser.add_argument("--abi-tag", default="abi3")
    parser.add_argument("--platform-tag", default="")
    parser.add_argument("--keyword", dest="keywords", action="append", default=[])
    parser.add_argument("--classifier", action="append", default=[])
    parser.add_argument("--project-url", nargs=2, metavar=("LABEL", "URL"), action="append", default=[])
    args = parser.parse_args()

    wheel_path = build_wheel(args)
    print(wheel_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
