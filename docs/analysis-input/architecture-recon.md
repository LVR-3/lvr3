# Code Context

## Files Retrieved
1. `CMakeLists.txt` (lines 2-15, 66-323, 484-583, 659-808, 918-935)
   - Root build graph, option surface, dependency discovery, ext libs, binary selection, and build-time conditionals.
2. `src/liblvr2/CMakeLists.txt` (lines 1-30, 118-220, 228-285, 356-399)
   - Core library build units (object/static/shared/CUDA variants) and optional feature blocks.
3. `src/tools/lvr2_reconstruct/Main.cpp` (lines 239-240, 243-310, 388-413, 414-488, 598-640, 788-950)
   - Primary runtime pipeline and command wiring for `lvr2_reconstruct`.
4. `src/tools/lvr2_reconstruct/Options.cpp` (lines 72-74, 215, 248-276)
   - CLI option definitions and getters for reconstruction.
5. `src/tools/lvr2_reconstruct/CMakeLists.txt` (lines 5-29)
   - Minimal per-tool target wiring for the flagship executable.
6. `src/liblvr2/io/ModelFactory.cpp` (lines 82-269)
   - Format-based IO dispatch and save/load orchestration.
7. `include/lvr2/io/ModelFactory.hpp` (lines 44-58)
   - IO factory API and shared transform state.
8. `include/lvr2/config/BaseOption.hpp` + `src/liblvr2/config/BaseOption.cpp` (lines 1-35, 20-80)
   - Shared option/parser base and coordinate transform handling.
9. `include/lvr2/reconstruction/PointsetSurface.hpp` + `include/lvr2/reconstruction/SearchTree.hpp` (lines 50-120, 35-95)
   - Surface abstraction and NN-search interface used by reconstruction.
10. `include/lvr2/reconstruction/SearchTreeFlann.hpp` (lines 55-89)
    - FLANN-backed concrete neighbor search.
11. `include/lvr2/types/Model.hpp` + `include/lvr2/types/PointBuffer.hpp` + `include/lvr2/types/ScanTypes.hpp` (lines 1-70, 1-90, 1-140)
    - Core data containers and scan-project typed model graph.
12. `include/lvr2/util/ScanProjectUtils.hpp` (lines 1-90, 185-250)
    - Scan project loading helpers used by reconstruct.
13. `CMakeModules/lvr2-config.cmake.in` (lines 45-100)
    - Exported dependency contract for downstream consumers.
14. `package.xml` (lines 1-70)
    - Declared package dependencies and packaging intent.
15. `ext/laslib/CMakeLists.txt` + `ext/rply/CMakeLists.txt` (lines 1-60, 1-28)
    - Vendored C/C++ libs shipped as build targets.

## Key Code
- **High-level pipeline entry (reconstruct):**
  - `main()` in `src/tools/lvr2_reconstruct/Main.cpp` parses `reconstruct::Options`, loads data via `ModelFactory::readModel`, runs reconstruction, then optimization/finalization and writes outputs.
  - Core flow: `loadPointCloud` -> `reconstructMesh` -> `optimizeMesh` -> `Materializer`/`TextureFinalizer` -> `ModelFactory::saveModel`.
- **Option-driven branching:**
  - In `Options.cpp`, `--pcm` and `--decomposition` are declared, and `Main.cpp` switches point-cloud manager based on PCM (`FLANN|LVR2|NANOFLANN|LBVH_CUDA`).
  - Reconstruction decomposition selection chooses `MC|PMC|MT|SF` and constructs `PointsetGrid` + `FastReconstruction`.
- **Library IO abstraction:**
  - `ModelFactory::readModel()` dispatches by file extension (`.ply`, `.pts`, `.obj`, `.las`, `.dat`, `.h5/.3d` dirs) into concrete IO classes (PLYIO/AsciiIO/UosIO/etc).
  - Same for save path; unsupported extensions log and no-op.
- **Core graph model:**
  - `PointBuffer` and `Model` encapsulate point/mesh payloads.
  - `ScanTypes` defines scan/position/lidar/camera object graph and shared pointers.
- **Reconstruction abstractions:**
  - `PointsetSurface` base interface defines distance/normals/bounding box/search-tree behavior.
  - `SearchTree` interface (`kSearch`, `radiusSearch`) with FLANN implementation in `SearchTreeFlann`.

## Architecture
- **Build architecture:**
  - Single top-level `CMakeLists.txt` acts as orchestrator. It detects many external deps (`TBB`, `GDAL`, `FLANN`, `OpenCV`, `HDF5`, `Boost`, `Eigen3`, `OpenMP`, optional `CUDA/OpenCL/VTK/PCL`), includes vendored libs, then builds `src/liblvr2` and a tool subset under option control.
  - Core library is built as both `lvr2_static` and `lvr2` shared; CUDA path optionally builds `lvr2cuda(_static)` and may add Cesium via `ExternalProject_Add` for 3DTILES.
  - Tool targets are mostly standalone executables with per-tool dependency lists and rely on `lvr2_static` + `LVR2_LIB_DEPENDENCIES`.
- **Data flow dependency chain (main executable):**
  - CLI → `BaseOption`/`reconstruct::Options` → `ModelFactory`/`ScanProjectUtils` → `PointsetSurface` (PCM + search tree) → `Grid` + `FastReconstruction` → algorithms (`optimization`, `color/cluster/material`) → IO save.
- **Downstream contracts:**
  - `CMakeModules/lvr2-config.cmake.in` exports required/optional dependency requirements and legacy `LVR2_USE_STATIC_LIBS` behavior.
- **Dependency shape:**
  - Heavy 3rd-party footprint: external packages + bundled LAS/PLY/utility libs (`ext/laslib`, `ext/rply`, `ext/nanoflann`, `ext/psimpl`, `ext/spdlog`, etc.).

## Simplification findings
- **Option parsing inconsistency risk:** `src/tools/lvr2_reconstruct/Options.cpp` binds `--decomposition` to `m_pcm` (same storage as PCM option). This couples unrelated CLI options and can silently overwrite defaults.
- **Silent CLI flag bug:** `doTextureAnalysis()` checks `"textureAnalyis"` (typo), so declared `"textureAnalysis"` is not detected by the getter.
- **Tool CMake duplication:** ~30 tools repeat identical patterns (`set(..._DEPENDENCIES)`, `add_executable`, `target_link_libraries`). This is ripe for macro/template extraction to reduce drift.
- **Dependency complexity concentration:** all dependency decisions live in one root CMake file with many conditional blocks; making it harder to reason about minimal build sets and optional feature boundaries.

## Start Here
Open `CMakeLists.txt` first (top-level build orchestration, options, dependency graph, and which targets are included by default) before editing CMake behavior; then inspect `src/tools/lvr2_reconstruct/Main.cpp` for runtime flow and `src/tools/lvr2_reconstruct/Options.cpp` for CLI correctness issues.