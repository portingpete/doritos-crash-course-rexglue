#!/usr/bin/env python3
"""Extract available XEX metadata into JSON using bundled local tools."""

from __future__ import annotations

import argparse
import errno
import hashlib
import json
import os
import re
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


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


def find_xextool(game_root: Path) -> Path:
    candidates = (
        game_root / "xextool.exe",
        Path(r"K:\XexTool_v6.3\xextool.exe"),
        Path(r"K:\1943\Battlefield\xextool.exe"),
    )
    for candidate in candidates:
        if candidate.exists():
            return candidate
    checked = ", ".join(str(candidate) for candidate in candidates)
    raise FileNotFoundError(f"xextool.exe not found. Checked: {checked}")


def run_xextool(game_root: Path, executable: Path) -> str:
    xextool = find_xextool(game_root)
    executable_arg = native_windows_path(executable)
    try:
        result = subprocess.run(
            [str(xextool), "-l", executable_arg],
            cwd=ROOT,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )
    except OSError as exc:
        if exc.errno == errno.ENOEXEC:
            raise RuntimeError(
                "xextool.exe cannot be executed from this WSL session. "
                "Run this tool from PowerShell or enable WSL Windows interop."
            ) from exc
        raise
    if result.returncode != 0:
        raise RuntimeError(f"xextool failed with exit {result.returncode}:\n{result.stdout}")
    return result.stdout


def native_windows_path(path: Path) -> str:
    """Return a path a Windows helper exe can open, including from WSL."""
    if os.name == "nt":
        return str(path)
    try:
        result = subprocess.run(
            ["wslpath", "-w", str(path)],
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            check=True,
        )
        converted = result.stdout.strip()
        if converted:
            return converted
    except Exception:
        pass
    return str(path)


def parse_xextool(output: str) -> dict:
    data: dict = {
        "xex_flags": [],
        "basefile": {},
        "execution_id": {},
        "static_libraries": [],
        "import_libraries": [],
        "resources": [],
        "sections": [],
        "raw_xextool_output": output,
    }

    section = ""
    for raw_line in output.splitlines():
        line = raw_line.rstrip()
        stripped = line.strip()
        if not stripped:
            continue

        if stripped in {
            "Xex Info",
            "Basefile Info",
            "Execution Id",
            "Static Libraries",
            "Import Libraries",
            "Resources",
            "Sections",
        }:
            section = stripped
            continue

        if section == "Xex Info" and not ":" in stripped:
            data["xex_flags"].append(stripped)
        elif section == "Basefile Info":
            match = re.match(r"([^:]+):\s+(.+)$", stripped)
            if match:
                key = match.group(1).strip().lower().replace(" ", "_")
                value = match.group(2).strip()
                if key in {"load_address", "entry_point", "export_table", "checksum"}:
                    value = value.split()[0]
                data["basefile"][key] = value
        elif section == "Execution Id":
            match = re.match(r"([^:]+):\s+(.+)$", stripped)
            if match:
                key = match.group(1).strip().lower().replace(" ", "_")
                data["execution_id"][key] = match.group(2).strip()
        elif section == "Static Libraries":
            match = re.match(r"\d+\)\s+(\S+)\s+(.+)$", stripped)
            if match:
                data["static_libraries"].append(
                    {"name": match.group(1), "version": match.group(2).strip()}
                )
        elif section == "Import Libraries":
            match = re.match(r"\d+\)\s+(\S+)\s+(.+)$", stripped)
            if match:
                data["import_libraries"].append(match.group(1))
        elif section == "Resources":
            match = re.match(r"\d+\)\s+([0-9A-F]+)\s+-\s+([0-9A-F]+)\s+:\s+(.+)$", stripped)
            if match:
                data["resources"].append(
                    {"start": match.group(1), "end": match.group(2), "name": match.group(3)}
                )
        elif section == "Sections":
            match = re.match(r"\d+\)\s+([0-9A-F]+)\s+-\s+([0-9A-F]+)\s+:\s+(.+)$", stripped)
            if match:
                data["sections"].append(
                    {"start": match.group(1), "end": match.group(2), "kind": match.group(3)}
                )

    return data


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--game-root", default="assets\\game", help="Path to extracted game files")
    parser.add_argument(
        "--executable",
        default="default.xex_uncrypted.xex",
        help="Expected XEX filename",
    )
    parser.add_argument("--out", default="logs/metadata/doritos-xex.json", help="JSON output path")
    args = parser.parse_args()

    game_root = Path(args.game_root)
    if not game_root.is_absolute():
        game_root = ROOT / game_root
    executable = game_root / args.executable
    out = Path(args.out)
    if not out.is_absolute():
        out = ROOT / out

    try:
        if not executable.exists():
            raise FileNotFoundError(f"Executable not found: {display_path(executable)}")
        xextool_output = run_xextool(game_root, executable)
        data = parse_xextool(xextool_output)
        data.update(
            {
                "game_root": display_path(game_root),
                "executable": display_path(executable),
                "size": executable.stat().st_size,
                "sha256": sha256(executable),
            }
        )
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_text(json.dumps(data, indent=2, sort_keys=True), encoding="utf-8")
    except Exception as exc:
        print(f"extract_metadata: {exc}", file=sys.stderr)
        return 1

    print(f"Wrote metadata: {display_path(out)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

