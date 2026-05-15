# Migration Guide

## Static-first CMake baseline

CMake minimum for this branch is now **3.22**.
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
- Custom CMake find modules remain installed; `lvr3` config also exposes module
  lookup path so `find_dependency`-based consumers keep working.
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

Assimp is now part of the default vcpkg manifest dependency set because mesh asset I/O requires the private backend. Optional vcpkg features must be requested alongside the matching CMake options for other packages. For example, use `-DVCPKG_MANIFEST_FEATURES=viewer -DLVR2_BUILD_VIEWER=ON` for the viewer, `-DVCPKG_MANIFEST_FEATURES=pcl -DLVR2_WITH_PCL=ON` for PCL tools, and `-DVCPKG_MANIFEST_FEATURES=draco` for optional Draco support.

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

Common package escape hatches follow the `LVR2_USE_SYSTEM_<PKG>` pattern, including `LVR2_USE_SYSTEM_TL_EXPECTED`, `LVR2_USE_SYSTEM_TBB`, `LVR2_USE_SYSTEM_SPDLOG`, `LVR2_USE_SYSTEM_HIGHFIVE`, `LVR2_USE_SYSTEM_RPLY`, `LVR2_USE_SYSTEM_LASLIB`, `LVR2_USE_SYSTEM_OPENCV`, `LVR2_USE_SYSTEM_HDF5`, and `LVR2_USE_SYSTEM_EIGEN3`. `CMakeSettings.json` is still kept for compatibility. Assimp is required privately for mesh I/O and is intentionally not exposed as a package-specific LVR option.

## Corrected PCA normals

The default `lvr2_reconstruct --nem 0` normal estimator now performs true
covariance PCA over the local neighborhood and uses the eigenvector with the
smallest eigenvalue as the tangent-plane normal. Previous builds used a
coordinate-dependent directional fit equivalent to `y = f(x, z)` while still
advertising the mode as PCA. That legacy behavior is not preserved as a fallback,
so reconstructed surfaces can change for vertical, tilted, noisy, or otherwise
axis-biased point neighborhoods. Degenerate neighborhoods now produce finite
fallback normals instead of propagating NaN values.

## Mesh I/O facade

A narrow public mesh I/O facade is available in the existing `lvr2` C++ namespace:

```cpp
#include <lvr2/mesh/io.hpp>

lvr2::mesh::LoadOptions loadOptions;
loadOptions.format = lvr2::mesh::Format::Auto; // infer from file suffix

lvr2::mesh::Result<lvr2::MeshBufferPtr> mesh =
    lvr2::mesh::load("input.obj", loadOptions);
if (!mesh) {
    const lvr2::mesh::Error& error = mesh.error();
    // inspect error.code, error.message, error.path, and error.format
}

lvr2::mesh::SaveOptions saveOptions;
saveOptions.format = lvr2::mesh::Format::Ply;
saveOptions.binary = true;
lvr2::mesh::Status saved = lvr2::mesh::save(*mesh, "output.ply", saveOptions);
```

The facade exposes LVR-owned `Format`, options, `ErrorCode`, `Error`, `Result<T>`, and `Status` vocabulary. `Result<T>` and `Status` are backed by `tl::expected`, so downstream CMake consumers need the `tl-expected` package available through system packages or the guarded vcpkg path. Installed `lvr2` and `lvr3` CMake configs now declare this public dependency.

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
#include <lvr2/mesh/io.hpp>

lvr2::mesh::LoadOptions loadOptions;
loadOptions.format = lvr2::mesh::Format::Auto;
auto mesh = lvr2::mesh::load("input.obj", loadOptions);
if (!mesh) {
    // Handle mesh.error().code, message, path, and format.
}

lvr2::mesh::SaveOptions saveOptions;
saveOptions.format = lvr2::mesh::Format::Ply;
saveOptions.binary = true;
auto saved = lvr2::mesh::save(*mesh, "output.ply", saveOptions);
```

Internal LVR tools still keep a private implementation bridge so CLI names, options, and current tool dispatch behavior are unchanged. That private bridge is not installed and must not be included by downstream code.

Current mesh facade coverage routes OBJ, PLY, STL, DAE/Collada, glTF, and glb mesh assets through the required private Assimp backend. Legacy private `ObjIO` and `STLIO` mesh-asset paths have been removed. The private in-tree `ModelFactory` bridge delegates OBJ/STL and mesh PLY cases to the facade; `PLYIO`, `ModelIOBase`, BaseIO, and non-mesh/point-cloud/scan `modelio` classes remain only as a bounded exception for scan-project and point-cloud storage until a later streaming/storage slice replaces them.

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
