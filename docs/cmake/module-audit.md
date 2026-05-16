# CMake module audit

LVR3 prefers vcpkg/config packages, CMake's standard modules, pkg-config imported targets, and upstream imported targets before project-local `Find*.cmake` files. `CMakeModules/` was removed. Internal helper code and package config templates live under `cmake/`; the few retained local finders live under `cmake/modules/`.

## Remaining local modules

| Module | Dependency | Why a project-local finder remains | Current consumers | Removal condition |
|---|---|---|---|---|
| `FindFLANN.cmake` | FLANN nearest-neighbor headers | CMake 3.22 has no standard `FindFLANN`; system packages and vcpkg/upstream configs vary in target names and static/shared selection. | Required configure path in `cmake/Lvr3Dependencies.cmake`; installed for `lvr2`/`lvr3` package compatibility. | Replace with a tested config/pkg-config alias helper once vcpkg and Jammy/Noble system FLANN expose stable imported targets. |
| `FindLZ4.cmake` | LZ4 compression | CMake 3.22 has no standard `FindLZ4`; distro and vcpkg installs may provide config, pkg-config, or only headers/libraries. This finder creates a bounded `LZ4::LZ4` imported target fallback. | Required configure path in `cmake/Lvr3Dependencies.cmake`; installed for `lvr2`/`lvr3` package compatibility. | Replace with config-first `lz4::lz4` plus pkg-config imported target after system and vcpkg coverage is verified. |
| `FindDraco.cmake` | Optional Draco compression | Optional 3D Tiles/Draco paths still use historical lowercase `draco_*` variables; switching to upstream `draco::` config targets changes exported dependency behavior and needs a focused test slice. | Optional `LVR2_WITH_3DTILES=OFF` Draco probe in `cmake/Lvr3Dependencies.cmake`; optional Draco sources in `src/liblvr2/CMakeLists.txt`. | Convert optional Draco support to `find_package(draco CONFIG)` with package/export tests, then remove this module. |
| `FindOpenCL2.cmake` | Optional OpenCL support | The legacy OpenCL code expects `OPENCL_*` variables and an `OpenCL_NEW_API` switch; standard CMake `FindOpenCL` target migration needs source/tool validation. | Optional `LVR2_WITH_OPENCL` probe in `cmake/Lvr3Dependencies.cmake`; OpenCL source/tool gates. | Migrate to CMake `FindOpenCL` imported targets and explicit version/API checks. |
| `FindRDB.cmake` | Optional proprietary RDB/RDBX support | RDB is proprietary/optional and often supplied from local installations without a reliable public config package. | Optional RDB probe in `cmake/Lvr3Dependencies.cmake`; RDBX source gates in `src/liblvr2/CMakeLists.txt`; optional installed package dependency when built. | Prefer a vendor-provided config package (`RDBConfig.cmake`/`rdb_DIR`) once verified, or remove RDB support if product scope drops it. |

## Removed or relocated modules

- Removed stale `FindGEOTIFF.cmake`: no CMake consumers remained.
- Removed stale `FindOpenNI.cmake` and `FindOpenNI2.cmake`: no active build consumers remained after viewer/freenect cleanup.
- Removed stale `Findembree.cmake`: Embree discovery is config-mode before local module lookup.
- Removed legacy `TBBConfig.cmake` shim: TBB is package-backed through CMake/vcpkg/system packages.
- Moved internal helpers/templates to `cmake/`: dependency policy, sanitizer/fuzz hooks, package config templates, uninstall template, packaging setup, and CUDA/GCC compatibility helper.
