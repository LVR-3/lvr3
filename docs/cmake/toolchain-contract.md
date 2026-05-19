# CMake toolchain contract

LVR3 targets ROS 2 Lyrical on Ubuntu Resolute 26.04. That target lane provides a CMake 4.2 series toolchain, so the project intentionally requires **CMake 4.2 or newer**.

## Required host tools

- CMake 4.2+
- Ninja, when using the shipped presets or CI smoke lanes
- A C++20-capable compiler
- vcpkg for the primary dependency lane, unless using the explicit system-package opt-out preset

## Executable contract

The contract is enforced in three places:

1. `CMakeLists.txt` uses `cmake_minimum_required(VERSION 4.2)`.
2. `CMakePresets.json` declares `cmakeMinimumRequired` 4.2.
3. `tests/cmake_toolchain_contract.cmake` checks the running CMake, root minimum, presets, README, and CI policy wiring.

A verification host below CMake 4.2 is not an intended Lyrical/Resolute toolchain. Provision CMake 4.2+ before using presets, configure, build, or CTest replay. Do not lower the project minimum for older distro compatibility unless a future ADR reintroduces older target lanes.

## Verification commands

```bash
cmake --version
ninja --version
c++ --version
cmake --list-presets=all
cmake -DLVR2_SOURCE_DIR="$PWD" -P tests/cmake_toolchain_contract.cmake
```

Passing the toolchain and preset phase only proves the host can read the project and presets. Dependency bootstrap, static/shared policy, and build/test failures remain separate verification slices.
