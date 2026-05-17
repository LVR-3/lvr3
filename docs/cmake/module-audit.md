# CMake module audit

LVR3 prefers vcpkg/config packages, CMake's standard modules, pkg-config imported targets, and upstream imported targets before project-local `Find*.cmake` files. `CMakeModules/` was removed. Internal helper code and package config templates live under `cmake/`; the few retained local finders live under `cmake/modules/`.

## Remaining local modules

| Module | Dependency | Why a project-local finder remains | Current consumers | Removal condition |
|---|---|---|---|---|
| `FindFLANN.cmake` | FLANN nearest-neighbor headers | CMake 4.2 has no standard `FindFLANN`; system packages and vcpkg/upstream configs vary in target names and static/shared selection. | Required configure path in `cmake/Lvr3Dependencies.cmake`; installed for `lvr2`/`lvr3` package compatibility. | Replace with a tested config/pkg-config alias helper once vcpkg and Lyrical/Resolute system FLANN expose stable imported targets. |
| `FindLZ4.cmake` | LZ4 compression | CMake 4.2 has no standard `FindLZ4`; distro and vcpkg installs may provide config, pkg-config, or only headers/libraries. This finder creates a bounded `LZ4::LZ4` imported target fallback. | Required configure path in `cmake/Lvr3Dependencies.cmake`; installed for `lvr2`/`lvr3` package compatibility. | Replace with config-first `lz4::lz4` plus pkg-config imported target after system and vcpkg coverage is verified. |
| `FindRDB.cmake` | Optional proprietary RDB/RDBX support | RDB is proprietary/optional and often supplied from local installations without a reliable public config package. | Optional source-tree RDB probe in `cmake/Lvr3Dependencies.cmake`; RDBX source gates in `src/liblvr2/CMakeLists.txt`. Installed packages should rely on a vendor config package when RDB is enabled. | Prefer a vendor-provided config package (`RDBConfig.cmake`/`rdb_DIR`) once verified, or remove RDB support if product scope drops it. |

## Removed or relocated modules

- Removed stale `FindGEOTIFF.cmake`: no CMake consumers remained.
- Removed stale `FindOpenNI.cmake` and `FindOpenNI2.cmake`: no active build consumers remained after viewer/freenect cleanup.
- Removed stale `Findembree.cmake`: Embree discovery is config-mode before local module lookup.
- Removed replaceable `FindDraco.cmake`: optional Draco discovery now uses package config mode (`find_package(draco CONFIG)`) and the upstream/vcpkg `draco::draco` imported target.
- Removed replaceable `FindOpenCL2.cmake`: optional OpenCL discovery now uses CMake's standard `FindOpenCL` module and preserves the legacy `OPENCL_FOUND`/`OpenCL_NEW_API` variables for source gates.
- Removed legacy `TBBConfig.cmake` shim: TBB is package-backed through CMake/vcpkg/system packages.
- Moved internal helpers/templates to `cmake/`: dependency policy, sanitizer/fuzz hooks, package config templates, uninstall template, packaging setup, and CUDA/GCC compatibility helper.
