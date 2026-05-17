# Migration Guide

## ROS 2 Lyrical / C++20 CMake baseline

The primary target is ROS 2 Lyrical on Ubuntu Resolute 26.04. CMake minimum for this branch is now **4.2**, matching the Resolute toolchain lane, and public LVR targets export `cxx_std_20`. Consumers must compile code that includes LVR public headers as C++20 or newer.

Default library output is **static** (`LVR2_BUILD_STATIC_LIBS=ON`) while shared
libraries are opt-in (`BUILD_SHARED_LIBS=OFF` by default). Shared mode can be
enabled with `-DBUILD_SHARED_LIBS=ON`; static output can be disabled with
`-DLVR2_BUILD_STATIC_LIBS=OFF` when shared output is enabled.

If both static and shared are built, the aggregate modern target remains
`lvr2::lvr2` and points to shared output; the static archive is also exported as
`lvr2::lvr2_static`.
When building static-only, `lvr2::lvr2` now resolves to the static archive so
existing modern CMake consumers that link `lvr2::lvr2` continue to work.
Legacy `${LVR2_LIBRARIES}` also follows this preference order: static target when
installed static output is available, shared fallback otherwise. Debian packaging
configures shared output explicitly so existing runtime packages still contain
shared libraries.

## lvr3 package identity

`find_package(lvr3)` is now available as a compatibility package identity that reuses the existing `lvr2` install/export.

Example:

```cmake
find_package(lvr3 REQUIRED)

# Canonical package target
add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE lvr3::lvr3)

# Keep old identity available too
find_package(lvr2 REQUIRED)
target_link_libraries(my_app PRIVATE lvr2::lvr2)
```

Compatibility guarantees:

- Existing `lvr2` install tree and exported target names are unchanged.
- `find_package(lvr2)` and `lvr2::lvr2` continue to work unchanged.
- `find_package(lvr3)` loads the same installed artifacts and exposes:
  - `lvr3::lvr3` as the canonical alias to the aggregate `lvr2::lvr2` target.
  - `lvr3::lvr3_static` when a static target is installed.
- `LVR3_USE_STATIC_LIBS` mirrors `LVR2_USE_STATIC_LIBS` for `LVR3_LIBRARIES`; the canonical target remains the aggregate target.
- Only audited compatibility CMake find modules remain installed; `lvr3` config also exposes the remaining module lookup path so `find_dependency`-based consumers keep working.
- No package names, install directories (`share/lvr2`, `package.xml`), C++ namespaces,
  CLI/tool names, options, or Debian package names are changed.

## Guarded vcpkg presets

The guarded vcpkg presets are superseded by the vcpkg-first dependency policy, which makes vcpkg the primary dependency path.

## vcpkg-first dependency policy

Dependency acquisition is now **vcpkg-first**. A `vcpkg.json` manifest declares the required package set, and configure defaults to `LVR2_WITH_VCPKG=ON`. Set `VCPKG_ROOT`, pass `-DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake`, or set `-DLVR2_VCPKG_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake`.

```bash
cmake --preset vcpkg-release
cmake --preset vcpkg-isolated-release
cmake --preset vcpkg-system-tbb-tl-release
```

Vendored dependencies under `ext/` were removed. `spdlog`, `HighFive`, `rply`, and `LASlib` now come from packages; `spdmon` was replaced by a small LVR-owned progress monitor implementation. The old `ExternalProject_Add` download path for 3D Tiles was also removed; keep `LVR2_WITH_3DTILES=OFF` until a package-backed Cesium Native path is added.

Assimp is now part of the default vcpkg manifest dependency set because mesh asset I/O requires the private backend. Optional vcpkg features must be requested alongside the matching CMake options for other packages. For example, use `-DVCPKG_MANIFEST_FEATURES=pcl -DLVR2_WITH_PCL=ON` for PCL tools and `-DVCPKG_MANIFEST_FEATURES=draco` for optional Draco support.

`LVR2_IGNORE_SYSTEM_PACKAGES` defaults to `ON` when vcpkg is enabled. To intentionally use a system package with the vcpkg toolchain, enable that package's explicit escape hatch. The matching vcpkg installed prefix is ignored for that package lookup so the system package wins instead of acting only as a fallback:

```bash
cmake -S . -B build-mixed \
  -DLVR2_WITH_VCPKG=ON \
  -DLVR2_USE_SYSTEM_TBB=ON \
  -DLVR2_USE_SYSTEM_TL_EXPECTED=ON
```

A distributor may opt out of vcpkg entirely with the system preset, but then all required packages must be available from the host/toolchain:

```bash
cmake --preset system-optout-release
cmake -S . -B build-system -DLVR2_WITH_VCPKG=OFF
```

Common package escape hatches follow the `LVR2_USE_SYSTEM_<PKG>` pattern, including `LVR2_USE_SYSTEM_TL_EXPECTED`, `LVR2_USE_SYSTEM_TBB`, `LVR2_USE_SYSTEM_FMT`, `LVR2_USE_SYSTEM_SPDLOG`, `LVR2_USE_SYSTEM_HIGHFIVE`, `LVR2_USE_SYSTEM_RPLY`, `LVR2_USE_SYSTEM_LASLIB`, `LVR2_USE_SYSTEM_OPENCV`, `LVR2_USE_SYSTEM_HDF5`, and `LVR2_USE_SYSTEM_EIGEN3`. `CMakeSettings.json` is still kept for compatibility. Assimp is required privately for mesh I/O and is intentionally not exposed as a package-specific LVR option.

## Format-style logging facade

The stream-style logging API has been removed. Code such as this no longer compiles:

```cpp
// Removed
lvr2::logout::get() << lvr2::info << "Loaded " << count << " points" << lvr2::endl;
```

Use the format-style facade instead:

```cpp
#include <lvr2/util/Logging.hpp>

lvr2::log::info("Loaded {} points", count);
lvr2::log::warning("Skipping scan {}", scan_index);
lvr2::log::error("Failed to open '{}': {}", path, reason);
```

Runtime/user-provided message text should use the explicit runtime APIs, for example `lvr2::log::info_runtime(message)`. The normal formatted path forwards the original format string and arguments to spdlog, including the optional `LVR2_LOG_INFO(...)`/`LVR2_LOG_WARNING(...)`/`LVR2_LOG_ERROR(...)` source-location macros. Types that only support stream insertion need a real formatter, `format_as`, or an explicit cheap summary string; do not add new stream-format logging shortcuts.

The public logging calls keep LVR-owned signatures and examples: no `fmt::` or `spdlog::` types appear in normal call sites. The installed `lvr2`/`lvr3` CMake configs still declare `fmt` and `spdlog` for the pre-std-format logging facade; the std-format follow-up removes `fmt` and switches spdlog to standard formatting. The ABI fallback remains `lvr2::log::write(Level, std::string_view)`.

## CMake file layout and module audit

The top-level `CMakeLists.txt` is now orchestration only. Build responsibilities live in project-owned files under `cmake/`, while `src/liblvr2`, `src/tools`, `tests`, and `examples` own their target lists. Internal helper/config templates moved out of `CMakeModules/`, and retained audited local `Find*.cmake` compatibility exceptions now live under `cmake/modules/`. See `docs/cmake/module-audit.md` for the remaining modules and removal conditions. Stale local Find modules for unused OpenNI/OpenNI2, GeoTIFF, old Embree, and the legacy TBB shim were removed.

## Bundled viewer removal

The Qt/VTK `lvr2_viewer` and the ncurses/Embree `lvr2_ascii_viewer` are no longer built or installed from the core LVR3 tree. The `LVR2_BUILD_VIEWER` option and the `viewer` vcpkg feature were removed with their viewer-only Qt, QVTK, VTK, and Curses discovery. Use the external viewer project or another mesh/point-cloud viewer for interactive visualization.

## ROS and Debian packaging

ROS and Debian packaging currently preserve the historical `lvr2` identity: `package.xml`, Debian source and binary package names, CLI/tool names, C++ namespaces, and `share/lvr2` installation are unchanged. The `lvr3` identity is available through the installed CMake package facade for CMake consumers.

Debian packaging uses the distributor/system-package escape hatch instead of the vcpkg-first default:

```bash
-DBUILD_SHARED_LIBS=ON -DLVR2_BUILD_STATIC_LIBS=OFF -DLVR2_WITH_VCPKG=OFF
```

The shared-only flags are required because mesh asset I/O uses a required private Assimp backend that must not leak through exported static target interfaces. Development packages no longer install static archives or vendored HighFive artifacts.

Verified ROS/Debian dependency names were added for package-backed dependencies introduced by the modernization work, including Assimp, tl-expected (`libexpected-dev`), spdlog, TBB, TIFF, GDAL, HDF5, OpenCV, Eigen, Boost, YAML-CPP, OpenGL/GLUT, and OpenCL. HighFive, rply, and LASlib/LAStools remain required by the package-backed system build, but verified Lyrical/Resolute Debian package names and rosdep keys are not recorded yet; distributors may need local packages or rosdep rules for those dependencies before full system-package Debian builds pass.

## Opt-in sanitizer, fuzz, and performance baseline hooks

Developer diagnostics are available but remain off by default:

```bash
cmake --preset sanitizer-vcpkg-debug
cmake --build --preset build-vcpkg-sanitizer-debug --target lvr2_mesh_io_facade_header_compile

cmake --preset fuzz-vcpkg-debug
cmake --build --preset build-vcpkg-fuzz-debug --target lvr2_mesh_io_fuzz
ctest --test-dir build-vcpkg-fuzz-debug -R lvr2_mesh_io_fuzz_seed

cmake --preset performance-baseline-vcpkg-release
ctest --test-dir build-vcpkg-performance-baseline-release -R lvr2_performance_baseline_smoke
```

`LVR2_ENABLE_SANITIZERS=ON` applies GCC/Clang `-fsanitize` instrumentation using
`LVR2_SANITIZERS` (default `address;undefined`). `LVR2_ENABLE_FUZZING=ON` builds
fuzz targets and a seed-corpus CTest hook; the default uses a portable standalone
seed runner, while Clang users can set `LVR2_FUZZ_WITH_LIBFUZZER=ON` for
libFuzzer. `LVR2_ENABLE_PERFORMANCE_BASELINES=ON` records JSON baselines under
the build tree. These diagnostics are optional/manual in CI and do not gate the
normal smoke lanes.

## Corrected PCA and RANSAC normals

The default `lvr2_reconstruct --nem 0` normal estimator now performs true
covariance PCA over the local neighborhood and uses the eigenvector with the
smallest eigenvalue as the tangent-plane normal. Previous builds used a
coordinate-dependent directional fit equivalent to `y = f(x, z)` while still
advertising the mode as PCA. That legacy behavior is not preserved as a fallback,
so reconstructed surfaces can change for vertical, tilted, noisy, or otherwise
axis-biased point neighborhoods. Degenerate neighborhoods now produce finite
fallback normals instead of propagating NaN values.

The RANSAC normal estimator selected with `--nem 1` now samples only from each
query point's local neighborhood, initializes its success state, rejects
degenerate triples, refits the winning inliers with the corrected PCA plane, and
falls back to finite PCA normals when no valid RANSAC plane can be found. Use
`--normalSeed <uint32>` to make the stochastic RANSAC path reproducible; the
default seed is `0`.

Scan-pose normal orientation now uses the nearest pose from the scan-pose file
itself instead of accidentally indexing the point cloud with pose-tree result
IDs. Normal interpolation also rejects NaN, infinite, and zero normals, orients
neighbors to a valid reference before averaging, and falls back to a finite unit
normal when no valid neighbor normal exists. Reconstructed surfaces can change
for datasets that rely on `--scanPoseFile` or normal interpolation (`--ki`).

## Mesh I/O facade

A narrow public mesh I/O facade is available under the unified `lvr2::io` C++ namespace:

```cpp
#include <lvr2/io/mesh.hpp>

lvr2::io::mesh::LoadOptions loadOptions;
loadOptions.format = lvr2::io::mesh::Format::Auto; // infer from file suffix

lvr2::io::mesh::Result<lvr2::MeshBufferPtr> mesh =
    lvr2::io::mesh::load("input.obj", loadOptions);
if (!mesh) {
    const lvr2::io::mesh::Error& error = mesh.error();
    // inspect error.code, error.message, error.path, and error.format
}

lvr2::io::mesh::SaveOptions saveOptions;
saveOptions.format = lvr2::io::mesh::Format::Ply;
saveOptions.binary = true;
lvr2::io::mesh::Status saved = lvr2::io::mesh::save(*mesh, "output.ply", saveOptions);
```

The facade exposes LVR-owned `Format`, options, `ErrorCode`, `Error`, `Result<T>`, and `Status` vocabulary. `Result<T>` and `Status` are backed by `tl::expected`, so downstream CMake consumers need the `tl-expected` package available through system packages or the guarded vcpkg path. Installed `lvr2` and `lvr3` CMake configs now declare this public dependency.

The earlier `<lvr2/mesh/io.hpp>` / `lvr2::mesh` facade path has been removed by the unified I/O namespace migration. Include `<lvr2/io/mesh.hpp>` and use `lvr2::io::mesh` instead.

Current mesh-asset facade support is provided by the required private Assimp backend:

- `load`: OBJ, PLY, STL, DAE/Collada, glTF, and glb mesh files.
- `save`: OBJ, PLY, STL, DAE/Collada, glTF, and glb mesh files.
- `SaveOptions::binary` is forwarded to backend formats with binary/text variants such as PLY and STL.

The initial facade used temporary legacy private readers for part of this coverage. Those mesh-asset paths have now been replaced by the private backend; retained point-cloud/scan storage classes are documented below as a bounded exception.

## Mesh reader/writer removal

The mesh-asset-specific legacy reader/writer headers are no longer installed as public C++ API:

- `#include <lvr2/io/ModelFactory.hpp>` / `lvr2::ModelFactory`
- `#include <lvr2/io/modelio/ObjIO.hpp>` / `lvr2::ObjIO`
- `#include <lvr2/io/modelio/PLYIO.hpp>` / `lvr2::PLYIO`
- `#include <lvr2/io/modelio/STLIO.hpp>` / `lvr2::STLIO`

Use the mesh facade instead:

```cpp
#include <lvr2/io/mesh.hpp>

lvr2::io::mesh::LoadOptions loadOptions;
loadOptions.format = lvr2::io::mesh::Format::Auto;
auto mesh = lvr2::io::mesh::load("input.obj", loadOptions);
if (!mesh) {
    // Handle mesh.error().code, message, path, and format.
}

lvr2::io::mesh::SaveOptions saveOptions;
saveOptions.format = lvr2::io::mesh::Format::Ply;
saveOptions.binary = true;
auto saved = lvr2::io::mesh::save(*mesh, "output.ply", saveOptions);
```

Internal LVR tools still keep a private implementation bridge so CLI names, options, and current tool dispatch behavior are unchanged. That private bridge is not installed and must not be included by downstream code.

Current mesh facade coverage routes OBJ, PLY, STL, DAE/Collada, glTF, and glb mesh assets through the required private Assimp backend. Legacy private `ObjIO` and `STLIO` mesh-asset paths have been removed. The private in-tree `ModelFactory` bridge delegates OBJ/STL and mesh PLY cases to the facade; `PLYIO`, `ModelIOBase`, and non-mesh point-cloud `modelio` classes remain only as a bounded exception until a later point-cloud streaming/storage slice replaces them.

Replacement coverage is guarded by `lvr2_removed_public_mesh_io_headers`, `lvr2_no_legacy_mesh_facade_usage`, `lvr2_required_private_assimp_policy`, and the mesh facade GoogleTest coverage (`lvr2_mesh_io_facade_gtest`, including replacement tests).

## Required private Assimp adapter

Assimp is required internally for mesh asset I/O, but it is not exposed as an LVR build option. The backend is intentionally shared-only to keep Assimp out of exported static target interfaces:

```bash
cmake -S . -B build-assimp \
  -DBUILD_SHARED_LIBS=ON \
  -DLVR2_BUILD_STATIC_LIBS=OFF
```

Assimp remains an implementation detail. Public headers, CMake package configs, target interfaces, C++ namespaces, CLI names, CLI options, and LVR build options do not expose Assimp types, flags, errors, or targets. Static/static+shared outputs now fail configure with a privacy-first diagnostic instead of silently falling back to legacy mesh readers.

The facade routes these formats through the private backend:

- `load`: OBJ, PLY, STL, DAE/Collada, glTF, and glb.
- `save`: OBJ, PLY, STL, DAE/Collada, glTF, and glb.

PLY mesh facade load/save no longer uses legacy `PLYIO`; the private `ModelFactory` bridge tries the facade for mesh PLY and falls back to `PLYIO` only for point-cloud PLY retained under the storage exception. Point-cloud PLY handling remains in the retained storage exception. Distributors who ship binaries with required Assimp mesh I/O must preserve Assimp's license notice and runtime dependency requirements.

Private Assimp adapter coverage is guarded by `lvr2_no_public_assimp_leakage`, `lvr2_no_legacy_mesh_facade_usage`, package-identity interface checks, and Assimp-enabled mesh facade tests.

## Removed BaseIO scan-project/storage feature API

The `BaseIO`/feature-template storage stack, public `scanio` and `meshio` storage wrappers, and deprecated HDF5 feature wrappers have been removed. The replacement public API lives under `lvr2::io`:

```cpp
auto opened = lvr2::io::scan::open_directory(
    path,
    lvr2::io::scan::Schema::raw_ply(),
    lvr2::io::storage::LoadMode::Lazy);
if (!opened) {
    // Handle opened.error().message.
}
auto saved = opened->save(*project);
auto loaded = opened->load();
```

One-shot helpers are the intended simple path for tools and examples:

```cpp
auto loaded = lvr2::io::scan::load_project(
    path,
    lvr2::io::scan::LoadOptions::directory_raw_ply());
auto saved = lvr2::io::scan::save_project(
    path,
    *project,
    lvr2::io::scan::SaveOptions::hdf5());
```

Bundled scan-project examples and utility callers use these `ProjectStore`/one-shot helpers instead of manual `DirectoryKernel`/`HDF5Kernel` construction and base-qualified feature calls. `ProjectStore` also exposes narrow non-template `load_position`, `load_lidar`, `load_scan`, `save_position`, `save_lidar`, and `save_scan` methods for tool code that previously needed lower-level scan-project storage operations.

The service path supports point-buffer round trips through the raw-PLY directory layout and HDF5 scan-project layout. Unsupported raw channel-directory schemas and camera/hyperspectral payload saves fail explicitly instead of silently reinterpreting or truncating data. Mesh HDF5/directory compatibility still needed by bundled tools is implemented by private non-template stores, not public feature wrappers. The legacy `lvr2_registration --hdf` path depended on removed feature wrappers and now fails explicitly; use directory scan inputs or convert the project before registration until a `ProjectStore`-backed HDF registration path is added.

The storage implementation uses one `StorageBackend`/`StorageRegistry` path for Directory, HDF5, fake/test, plugin, and future custom/IP backends. It does not provide CRTP compatibility aliases, a built-in-only backend selector, or a second extension path. See `docs/io/baseio-removal-inventory.md` for final guard and validation commands.

## 25.1.0 -> 25.2.0


We switched to more modern CMake. Which is why you'll need to change the old style CMake:

```cmake
find_package(LVR2 REQUIRED)
# ...
target_link_libraries(my_app ${LVR2_LIBRARIES})
```

to


```cmake
find_package(lvr2 REQUIRED)
# ...
target_link_libraries(my_app lvr2::lvr2)
```

> [!NOTE]
> The old-style CMake is still available in 25.2.0 but it's obsolete and you will be forced to update it in the next major release.
