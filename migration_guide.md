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

Dependency acquisition policy now uses **system packages by default**.
The legacy implicit MSVC vcpkg hook was removed.
To use vcpkg, set `LVR2_WITH_VCPKG=ON` explicitly (optionally via preset):

```bash
cmake --preset system-release
cmake --preset vcpkg-release
cmake --preset vcpkg-ignore-system-release
cmake -S . -B build -DLVR2_WITH_VCPKG=ON -DLVR2_VCPKG_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
```

`LVR2_IGNORE_SYSTEM_PACKAGES=ON` disables CMake system and system-environment package search paths while still using the configured toolchain, and `CMakeSettings.json` is still kept for compatibility.

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

Current facade support is intentionally conservative while broader writer validation and the private Assimp adapter are deferred to later slices:

- `load`: OBJ and PLY mesh files.
- `save`: binary PLY mesh files.
- STL loading/saving, OBJ saving, DAE/Collada, glTF, and glb currently return `ErrorCode::UnsupportedFormat` through the facade.
- `SaveOptions::binary=false` is not silently ignored; it returns `UnsupportedFormat` until text/ASCII output is implemented and tested.

Existing public readers and writers such as `ModelFactory`, `ModelIOBase`, `ObjIO`, `PLYIO`, and `STLIO` are **not removed** by the initial facade. Their removal is covered by the mesh reader/writer removal notes and guarded by replacement tests.

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

Current facade coverage remains: OBJ/PLY load and binary PLY save are supported; STL load/save, OBJ save, DAE/Collada, glTF, and glb return structured `ErrorCode::UnsupportedFormat` until later test-backed slices. `ModelIOBase` and non-mesh/point-cloud/scan `modelio` classes remain public for now and are deferred to later I/O/streaming slices.

Replacement coverage is guarded by `lvr2_removed_public_mesh_io_headers` and the mesh facade GoogleTest coverage (`lvr2_mesh_io_facade_gtest`, including replacement tests).

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
