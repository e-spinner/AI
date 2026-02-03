#!/usr/bin/env python3
import os
import sys
from pathlib import Path

# --------------------------------------------------------------------------- #
# This is used by meson to symlink files like config.cfg into build directory #
# --------------------------------------------------------------------------- #


def create_symlink(src: Path, dest: Path):
    dest.parent.mkdir(parents=True, exist_ok=True)
    try:
        if dest.exists() or dest.is_symlink():
            dest.unlink()
        dest.symlink_to(src.resolve())
        print(f"[INFO] Created symlink: {dest} -> {src}")
    except Exception as e:
        print(
            f"[ERROR] Failed to create symlink: {dest} -> {src}: {e}", file=sys.stderr
        )


def process_path(src_root: Path, build_root: Path, rel_path: str, recursive: bool):
    src = src_root / rel_path
    dst = build_root / rel_path

    if src.is_dir() and recursive:
        for child in src.rglob("*"):
            if child.is_file():
                rel_child = child.relative_to(src_root)
                create_symlink(child, build_root / rel_child)
    else:
        create_symlink(src, dst)


def main():
    if len(sys.argv) < 4:
        print(
            f"Usage: {sys.argv[0]} <source_root> <build_root> [--recursive] <relative_paths...>",
            file=sys.stderr,
        )
        sys.exit(1)

    source_root = Path(sys.argv[1]).resolve()
    build_root = Path(sys.argv[2]).resolve()

    recursive = False
    rel_paths = []

    for arg in sys.argv[3:]:
        if arg == "--recursive":
            recursive = True
        else:
            rel_paths.append(arg)

    for rel_path in rel_paths:
        process_path(source_root, build_root, rel_path, recursive)


if __name__ == "__main__":
    main()
