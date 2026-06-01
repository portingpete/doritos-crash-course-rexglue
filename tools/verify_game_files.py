#!/usr/bin/env python3
"""Verify required Doritos Crash Course files without modifying them."""

from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
REQUIRED_ASSETS = (
    "ArcadeInfo.xml",
    "data.pak",
    "game.png",
    "game_bkgnd.jpg",
    "game_boxart.jpg",
    "game_marketplace.png",
)


def display_path(path: Path) -> str:
    try:
        return path.resolve().relative_to(ROOT).as_posix()
    except ValueError:
        return str(path)


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest().upper()


def verify(game_root: Path, executable: str, title_update: str | None) -> tuple[dict, list[str]]:
    errors: list[str] = []
    data: dict = {
        "game_root": display_path(game_root),
        "executable_name": executable,
        "title_update_path": title_update or "",
        "executable": {},
        "required_assets": {},
        "optional": {},
    }

    if not game_root.exists():
        errors.append(f"Game root does not exist: {display_path(game_root)}")
        return data, errors
    if not game_root.is_dir():
        errors.append(f"Game root is not a directory: {display_path(game_root)}")
        return data, errors

    exe_path = game_root / executable
    exe_info = {
        "path": display_path(exe_path),
        "exists": exe_path.exists(),
        "size": exe_path.stat().st_size if exe_path.exists() else 0,
        "sha256": sha256(exe_path) if exe_path.exists() else "",
    }
    data["executable"] = exe_info
    if not exe_path.exists():
        errors.append(f"Missing executable: {display_path(exe_path)}")

    for asset in REQUIRED_ASSETS:
        path = game_root / asset
        data["required_assets"][asset] = {
            "path": display_path(path),
            "exists": path.exists(),
            "size": path.stat().st_size if path.exists() else 0,
        }
        if not path.exists():
            errors.append(f"Missing required asset: {display_path(path)}")

    if title_update:
        tu_path = Path(title_update)
        if not tu_path.is_absolute():
            tu_path = ROOT / tu_path
        data["optional"]["title_update"] = {
            "path": display_path(tu_path),
            "exists": tu_path.exists(),
            "size": tu_path.stat().st_size if tu_path.exists() else 0,
        }
        if not tu_path.exists():
            errors.append(f"Title update path does not exist: {display_path(tu_path)}")
    else:
        data["optional"]["title_update"] = {"path": "", "exists": False, "size": 0}

    return data, errors


def print_text(data: dict, errors: list[str]) -> None:
    print(f"Game root: {data['game_root']}")
    exe = data["executable"]
    print(f"Executable: {exe.get('path', '')}")
    print(f"  exists: {exe.get('exists', False)}")
    print(f"  sha256: {exe.get('sha256', '')}")
    print("Required assets:")
    for name, info in data["required_assets"].items():
        state = "ok" if info["exists"] else "missing"
        print(f"  - {name}: {state}")
    if errors:
        print("Errors:", file=sys.stderr)
        for error in errors:
            print(f"  - {error}", file=sys.stderr)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--game-root", default="assets\\game", help="Path to extracted game files")
    parser.add_argument(
        "--executable",
        default="default.xex_uncrypted.xex",
        help="Expected XEX filename",
    )
    parser.add_argument("--title-update", help="Optional title update path")
    parser.add_argument("--json", action="store_true", help="Print machine-readable JSON")
    args = parser.parse_args()

    game_root = Path(args.game_root)
    if not game_root.is_absolute():
        game_root = ROOT / game_root

    data, errors = verify(game_root, args.executable, args.title_update)
    if args.json:
        print(json.dumps(data, indent=2, sort_keys=True))
        if errors:
            for error in errors:
                print(error, file=sys.stderr)
    else:
        print_text(data, errors)
    return 1 if errors else 0


if __name__ == "__main__":
    raise SystemExit(main())

