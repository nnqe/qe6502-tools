#!/usr/bin/env python3
"""Build a self-contained qe6502 Python source distribution."""

from __future__ import annotations

import argparse
import re
import shutil
import tarfile
import tempfile
from email.message import Message
from pathlib import Path


def normalize_distribution(name: str) -> str:
    return re.sub(r"[-_.]+", "-", name).lower()


def copy_file(src: Path, dst: Path) -> None:
    if not src.is_file():
        raise FileNotFoundError(src)
    dst.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src, dst)


def copy_dir(src: Path, dst: Path) -> None:
    if not src.is_dir():
        raise FileNotFoundError(src)
    if dst.exists():
        shutil.rmtree(dst)
    shutil.copytree(src, dst)


def metadata_text(args: argparse.Namespace, readme_text: str) -> str:
    message = Message()
    message["Metadata-Version"] = "2.4"
    message["Name"] = args.name
    message["Version"] = args.version
    message["Summary"] = args.description
    message["Author"] = args.author
    message["License"] = args.license
    message["Requires-Python"] = args.requires_python
    message["Description-Content-Type"] = "text/markdown"
    message["License-File"] = "LICENSE"

    if args.keywords:
        message["Keywords"] = ", ".join(args.keywords)

    for classifier in args.classifier:
        message["Classifier"] = classifier

    for label, url in args.project_url:
        message["Project-URL"] = f"{label}, {url}"

    return message.as_string() + "\n" + readme_text


def add_tree_to_tar(tar: tarfile.TarFile, root: Path, prefix: str) -> None:
    for path in sorted(root.rglob("*")):
        arcname = f"{prefix}/{path.relative_to(root).as_posix()}"
        info = tar.gettarinfo(str(path), arcname)
        info.uid = 0
        info.gid = 0
        info.uname = ""
        info.gname = ""
        if path.is_file():
            with path.open("rb") as file_obj:
                tar.addfile(info, file_obj)
        else:
            tar.addfile(info)


def build_sdist(args: argparse.Namespace) -> Path:
    repo_root = args.repo_root.resolve()
    stage_dir = args.stage_dir.resolve()
    dist_dir = args.dist_dir.resolve()
    dist_dir.mkdir(parents=True, exist_ok=True)

    distribution = normalize_distribution(args.name)
    root_name = f"{distribution}-{args.version}"
    sdist_path = dist_dir / f"{root_name}.tar.gz"

    with tempfile.TemporaryDirectory(prefix="qe6502-sdist-") as tmp_name:
        sdist_root = Path(tmp_name) / root_name
        sdist_root.mkdir(parents=True)

        copy_file(stage_dir / "pyproject.toml", sdist_root / "pyproject.toml")
        copy_file(args.setup_py, sdist_root / "setup.py")
        copy_file(args.setup_cfg, sdist_root / "setup.cfg")
        copy_file(args.readme, sdist_root / "README.md")
        copy_file(args.license_file, sdist_root / "LICENSE")
        copy_file(repo_root / "binds" / "python" / "_qe6502.c", sdist_root / "_qe6502.c")
        copy_file(repo_root / "binds" / "python" / "qe6502" / "__init__.py", sdist_root / "qe6502" / "__init__.py")

        copy_file(
            repo_root / "cpu" / "include" / "qe6502" / "qe6502.h",
            sdist_root / "native" / "include" / "qe6502" / "qe6502.h",
        )
        copy_file(
            repo_root / "cpu" / "include" / "qe6502" / "qe6502_version.h",
            sdist_root / "native" / "include" / "qe6502" / "qe6502_version.h",
        )
        copy_file(
            repo_root / "cpu" / "include" / "qe6502" / "qe6502_abi.h",
            sdist_root / "native" / "include" / "qe6502" / "qe6502_abi.h",
        )
        copy_file(repo_root / "cpu" / "src" / "qe6502.c", sdist_root / "native" / "src" / "qe6502.c")
        copy_dir(repo_root / "cpu" / "src" / "control_store", sdist_root / "native" / "src" / "control_store")

        readme_text = args.readme.read_text(encoding="utf-8")
        (sdist_root / "PKG-INFO").write_text(metadata_text(args, readme_text), encoding="utf-8")

        if sdist_path.exists():
            sdist_path.unlink()

        with tarfile.open(sdist_path, "w:gz") as tar:
            add_tree_to_tar(tar, sdist_root, root_name)

    return sdist_path


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=Path, required=True)
    parser.add_argument("--stage-dir", type=Path, required=True)
    parser.add_argument("--dist-dir", type=Path, required=True)
    parser.add_argument("--setup-py", type=Path, required=True)
    parser.add_argument("--setup-cfg", type=Path, required=True)
    parser.add_argument("--name", required=True)
    parser.add_argument("--version", required=True)
    parser.add_argument("--description", required=True)
    parser.add_argument("--author", required=True)
    parser.add_argument("--license", dest="license", required=True)
    parser.add_argument("--license-file", type=Path, required=True)
    parser.add_argument("--readme", type=Path, required=True)
    parser.add_argument("--requires-python", required=True)
    parser.add_argument("--keyword", dest="keywords", action="append", default=[])
    parser.add_argument("--classifier", action="append", default=[])
    parser.add_argument("--project-url", nargs=2, metavar=("LABEL", "URL"), action="append", default=[])
    args = parser.parse_args()

    sdist_path = build_sdist(args)
    print(sdist_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
