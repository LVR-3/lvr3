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
