#!/usr/bin/env python3
"""Record opt-in, non-gating performance baselines as JSON.

With no command, this runs a deterministic lightweight smoke workload so a
configured build can prove the baseline hook works without large datasets. With a
command after ``--``, it records elapsed wall time for that command and preserves
the command's exit code.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import platform
import statistics
import subprocess
import sys
import time
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


def _percentile(values: list[float], q: float) -> float:
    if not values:
        return 0.0
    ordered = sorted(values)
    index = min(len(ordered) - 1, max(0, round((len(ordered) - 1) * q)))
    return ordered[index]


def _smoke_workload() -> dict[str, Any]:
    points: list[tuple[float, float, float]] = []
    for y in range(96):
        for x in range(96):
            z = ((x * 17 + y * 31) % 13) * 0.0001
            points.append((x * 0.01, y * 0.01, z))

    min_x = min(p[0] for p in points)
    max_x = max(p[0] for p in points)
    min_y = min(p[1] for p in points)
    max_y = max(p[1] for p in points)
    min_z = min(p[2] for p in points)
    max_z = max(p[2] for p in points)

    digest = hashlib.sha256()
    for index, (x, y, z) in enumerate(points):
        if index % 17 == 0:
            digest.update(f"v {x:.6f} {y:.6f} {z:.6f}\n".encode("ascii"))

    return {
        "points": len(points),
        "bbox": [min_x, min_y, min_z, max_x, max_y, max_z],
        "checksum": digest.hexdigest(),
    }


def _measure_smoke(repeat: int) -> tuple[list[dict[str, Any]], int]:
    elapsed: list[float] = []
    details: dict[str, Any] | None = None
    for _ in range(repeat):
        start = time.perf_counter()
        details = _smoke_workload()
        elapsed.append(time.perf_counter() - start)

    metrics = [
        {
            "name": "generated_fixture_smoke",
            "unit": "seconds",
            "repeat": repeat,
            "min": min(elapsed),
            "median": statistics.median(elapsed),
            "p90": _percentile(elapsed, 0.90),
            "max": max(elapsed),
            "details": details,
        }
    ]
    return metrics, 0


def _measure_command(command: list[str], repeat: int) -> tuple[list[dict[str, Any]], int]:
    elapsed: list[float] = []
    return_code = 0
    for _ in range(repeat):
        start = time.perf_counter()
        completed = subprocess.run(command, check=False)
        elapsed.append(time.perf_counter() - start)
        if completed.returncode != 0:
            return_code = completed.returncode
            break

    metrics = [
        {
            "name": "command_wall_time",
            "unit": "seconds",
            "repeat": len(elapsed),
            "min": min(elapsed),
            "median": statistics.median(elapsed),
            "p90": _percentile(elapsed, 0.90),
            "max": max(elapsed),
            "command": command,
            "return_code": return_code,
        }
    ]
    return metrics, return_code


def _write_report(path: Path, label: str, metrics: list[dict[str, Any]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    report = {
        "schema_version": 1,
        "non_gating": True,
        "label": label,
        "created_utc": datetime.now(timezone.utc).isoformat(),
        "host": {
            "system": platform.system(),
            "release": platform.release(),
            "machine": platform.machine(),
            "python": platform.python_version(),
            "ci": bool(os.environ.get("CI")),
        },
        "metrics": metrics,
    }
    path.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--label", default="smoke-baseline")
    parser.add_argument("--repeat", type=int, default=5)
    parser.add_argument("command", nargs=argparse.REMAINDER)
    args = parser.parse_args(argv)

    if args.repeat < 1:
        parser.error("--repeat must be at least 1")

    command = list(args.command)
    if command and command[0] == "--":
        command = command[1:]

    if command:
        metrics, return_code = _measure_command(command, args.repeat)
    else:
        metrics, return_code = _measure_smoke(args.repeat)

    _write_report(args.output, args.label, metrics)
    print(f"wrote non-gating performance baseline: {args.output}")
    return return_code


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
