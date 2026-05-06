# Dependencies / Build Recon

Scope: static audit of CMake, package metadata, and `ext/`. No configure/build run.

## Build surface

- Root CMake: `cmake_minimum_required(VERSION 3.16)`, project `lvr2 25.2.3`, C++17 required. Refs: `CMakeLists.txt:1-2`, `CMakeLists.txt:17-18`.
- Default build: tools on, examples/viewer/experimental/KinFu/3DTiles/PCL/Freenect off; CUDA/OpenCL probes on. Refs: `CMakeLists.txt:5-15`.
- Main artifacts always build both static and shared `lvr2`, plus vendored `rply`/`laslib` static+shared. Refs: `src/liblvr2/CMakeLists.txt:228`, `src/liblvr2/CMakeLists.txt:260`, `ext/rply/CMakeLists.txt:12-14`, `ext/laslib/CMakeLists.txt:46-48`.

## Required deps by current CMake

| Dep | Evidence / notes |
|---|---|
| CMake + C++17 compiler | `CMakeLists.txt:1`, `CMakeLists.txt:17-18`; README says C++17 required at `README.md:42`. |
| TBB | Required at `CMakeLists.txt:134`; package deps list `libtbb-dev` at `CMakeModules/lvr2-packaging.cmake:44-60`. |
| TIFF | Required at `CMakeLists.txt:142`; CPack has `libtiff-dev` at `CMakeModules/lvr2-packaging.cmake:44-60`. |
| GDAL | Required at `CMakeLists.txt:148`; CPack/package.xml list it: `CMakeModules/lvr2-packaging.cmake:47`, `package.xml:38`. |
| OpenCV | Requires OpenCV 3 or 4 components `core;imgproc;imgcodecs;features2d;calib3d`: `CMakeLists.txt:154-164`; public headers include OpenCV, e.g. `include/lvr2/types/ScanTypes.hpp:19`, `include/lvr2/types/CameraModels.hpp:7-8`. |
| FLANN | Required at `CMakeLists.txt:173`; custom finder only locates headers (`CMakeModules/FindFLANN.cmake:1-8`). |
| LZ4 | Required at `CMakeLists.txt:180`; custom finder creates `LZ4::LZ4` if found (`CMakeModules/FindLZ4.cmake:1-56`). |
| GSL | Required at `CMakeLists.txt:187`. |
| Eigen3 | Required at `CMakeLists.txt:194`; exported target links `Eigen3::Eigen`: `src/liblvr2/CMakeLists.txt:218-221`. |
| Boost | Required in CONFIG mode with `program_options filesystem thread serialization timer iostreams date_time`, plus `mpi` if MPI was found. Refs: `CMakeLists.txt:203-217`. Public headers use Boost widely, e.g. `include/lvr2/util/IOUtils.hpp:38`, `include/lvr2/config/BaseOption.hpp:38`. |
| HDF5 | Required C/CXX/HL at `CMakeLists.txt:227`; public headers include HDF5/HighFive at `include/lvr2/util/Hdf5Util.hpp:7-11`. |
| OpenGL + GLUT/freeglut | `find_package(OpenGL COMPONENTS OpenGL)` is not marked REQUIRED (`CMakeLists.txt:234`), but GLUT/freeglut is required by CMake fallback (`CMakeLists.txt:249-258`) and public headers include GL/GLUT (`include/lvr2/display/Renderable.hpp:45-49`, `include/lvr2/algorithm/Tesselator.hpp:44-49`). |
| yaml-cpp | Required at `CMakeLists.txt:518`; public headers include yaml-cpp, e.g. `include/lvr2/util/YAMLUtil.hpp:5`, `include/lvr2/types/ScanTypes.hpp:26`. |
| pthread/m | Appended on Unix / non-MSVC link lists: `CMakeLists.txt:683-706`, `CMakeLists.txt:724-726`. |

## Optional / feature deps

| Feature | Deps | Evidence / notes |
|---|---|---|
| MPI | MPI C++ and Boost.MPI | Optional probe at `CMakeLists.txt:121`; if found, Boost.MPI becomes required (`CMakeLists.txt:211-214`) and MPI libs are appended (`CMakeLists.txt:733-734`). |
| Embree raycasting / ASCII viewer | Embree 4 or 3 | Optional probe before module path at `CMakeLists.txt:66-80`; enables source at `src/liblvr2/CMakeLists.txt:123-127`; ASCII viewer needs Curses+Embree at `CMakeLists.txt:842-848`. |
| CUDA | CUDAToolkit or legacy FindCUDA; NVRTC required once CUDA found | Default probe on except MSVC/Apple (`CMakeLists.txt:270-338`); builds `lvr2cuda` static/shared and requires NVRTC (`src/liblvr2/CMakeLists.txt:346-353`, `src/liblvr2/CMakeLists.txt:356-392`). |
| OpenCL | OpenCL or CUDA-provided OpenCL | Default probe on: `CMakeLists.txt:360-370`; wrapper fallback in `CMakeModules/FindOpenCL2.cmake:1-28`; sources/tools gated by `OPENCL_FOUND` at `src/liblvr2/CMakeLists.txt:177-183`, `CMakeLists.txt:806-808`. |
| PCL | PCL + MPI | Off by default; PCL is accepted only when MPI is found: `CMakeLists.txt:428-447`. |
| Viewer | VTK, Qt5 Core/Widgets/Xml/OpenGL, QVTK, optional Curses | Viewer gate at `CMakeLists.txt:372-425`, `CMakeLists.txt:821-848`; patched QVTK headers vendored in `ext/QVTKOpenGLWidget`. |
| OpenMP | OpenMP | Optional probe and flags at `CMakeLists.txt:452-466`. |
| Freenect | pkg-config `libfreenect` | Off by default, probed at `CMakeLists.txt:470-477`. Source gate uses stale variable; see risks. |
| RDB / RiVLib | Riegl RDB, optional `ext/rivlib` | RDB probe at `CMakeLists.txt:343-357`; RiVLib searched only if ignored `ext/rivlib/cmake` exists (`CMakeLists.txt:527-536`, `.gitignore:65`). |
| 3D Tiles / Draco | `cesium-native` ExternalProject, Draco; or system Draco when 3DTiles off | Network fetch pinned to Cesium Native `v0.16.0`: `CMakeLists.txt:540-571`; system Draco probe at `CMakeLists.txt:573-582`. |
| Docs | Doxygen + GraphViz | Optional doc target at `CMakeLists.txt:918-935`. |
| KinFu | Intended: OpenNI2, OpenCV 2.4.8 with `viz`/`nonfree`, CUDA | `LVR2_WITH_KINFU` exists but is unused (`CMakeLists.txt:9` only). Dead vendored CMake expects old stack: `ext/kintinuous/CMakeLists.txt:4`, `ext/kintinuous/CMakeLists.txt:30-40`. |

## Vendored deps in `ext/`

| Path | Kind | License / version clues | Build use |
|---|---|---|---|
| `ext/spdlog` | Vendored logging lib | MIT, version 1.15.0: `ext/spdlog/include/spdlog/version.h:6-8` | Added unconditionally (`CMakeLists.txt:484`), tests/bench off by default (`ext/spdlog/CMakeLists.txt:71-75`), linked as build-interface private dependency. |
| `ext/spdmon` | Vendored progress/monitor helper | MIT, version 1.0.0: `ext/spdmon/include/spdmon/version.hpp:4-8` | Added unconditionally as header-only (`CMakeLists.txt:487-491`); CMake includes Conan/Vcpkg/CTest and writes a version header into the source tree (`ext/spdmon/CMakeLists.txt:65-79`). |
| `ext/nanoflann` | Header-only nearest-neighbor helper | BSD notice in header (`ext/nanoflann/nanoflann.hpp:2-6`) | Installed under `include/lvr2/ext/nanoflann`: `ext/nanoflann/CMakeLists.txt:5-11`. |
| `ext/psimpl` | Header-only polyline simplification | MPL 1.1: `ext/psimpl/psimpl.h:1-5` | Installed under `include/lvr2/ext/psimpl`: `ext/psimpl/CMakeLists.txt:4-7`. |
| `ext/rply` | Vendored PLY C lib | MIT, RPly 1.1.1: `ext/rply/rply.h:8-17` | Builds static+shared and exports targets: `ext/rply/CMakeLists.txt:12-25`. |
| `ext/laslib` | Vendored LASlib/LASzip | LGPL-2.1: `ext/laslib/COPYING.txt:1`; README notes LGPL at `ext/laslib/laslib_README.txt:3-4` | Builds static+shared and exports targets: `ext/laslib/CMakeLists.txt:46-58`. |
| `ext/HighFive` | Vendored HDF5 C++ headers | Boost Software License, version 2.2.2: `ext/HighFive/LICENSE:1`, `ext/HighFive/CMakeLists.txt:8` | Not added as subdir; manually included/installed because subdir “crashs”: `CMakeLists.txt:513-515`, `CMakeLists.txt:893-895`. |
| `ext/CTPL` | Header-only thread pool | Apache-2.0: `ext/CTPL/LICENSE:1-3` | Installed under `include/lvr2/ext/CTPL`: `ext/CTPL/CMakeLists.txt:3-11`. |
| `ext/QVTKOpenGLWidget` | Patched VTK/Qt headers | VTK compatibility patch | Viewer-only add for VTK 8/9: `CMakeLists.txt:379-389`; install path in `ext/QVTKOpenGLWidget/CMakeLists.txt:4-10`. |
| `ext/kintinuous` | Old KinFu app/library | Separate license in `ext/kintinuous/LICENSE.md` | Currently unreachable from root CMake; its own CMake needs OpenNI2/OpenCV 2.4/CUDA and links stale `lvr_static`: `ext/kintinuous/CMakeLists.txt:4-40`, `ext/kintinuous/kfusion/CMakeLists.txt:52-68`. |

Also in-tree non-`ext`: PMP code carries separate MIT-style license in `PMP_LICENSE.txt:1-20` and is compiled into `src/liblvr2` (`src/liblvr2/CMakeLists.txt:5-21`).

## Package / docs surfaces

- CPack emits TGZ+DEB and hard-codes dev deps including OpenCL/OpenMPI/toolchain-style packages: `CMakeModules/lvr2-packaging.cmake:36-60`; adds Boost component dev packages dynamically at `CMakeModules/lvr2-packaging.cmake:69-72`.
- Installed CMake config re-finds many deps and unconditionally calls `find_dependency(MPI)`: `CMakeModules/lvr2-config.cmake.in:45-62`; optional PCL/Embree/RDB blocks at `CMakeModules/lvr2-config.cmake.in:68-96`.
- ROS `package.xml` lists cmake and broad deps: `package.xml:34-51`.
- README Ubuntu deps include stale versioned GUI packages (`libvtk7-*`, `qt5-default`): `README.md:32-40`; Debian metadata still uses `libvtk6-qt-dev` and mandatory CUDA toolkit in Build-Depends: `debian/control:4-11`.
- Legacy Debian rules build two trees and use obsolete `-DWITH_CUDA=Off`: `debian/rules:22-27`.

## Main risks

1. **Option drift / dead knobs.** Root options use `LVR2_WITH_*`, but code still checks `WITH_3DTILES` and `WITH_FREENECT`; Debian uses `WITH_CUDA`; KinFu option is never consumed. Refs: `CMakeLists.txt:9-15`, `src/liblvr2/CMakeLists.txt:130`, `src/liblvr2/CMakeLists.txt:157-211`, `src/tools/lvr2_3dtiles/CMakeLists.txt:1-3`, `debian/rules:25`.
2. **3DTiles likely broken.** `LVR2_WITH_3DTILES` fetches Cesium and sets `3DTILES_LIBRARIES`, but link append checks `3DTILES_FOUND` which is never set; sources/tool check legacy `WITH_3DTILES`. Refs: `CMakeLists.txt:540-571`, `CMakeLists.txt:749-751`, `src/liblvr2/CMakeLists.txt:157-211`, `src/tools/lvr2_3dtiles/CMakeLists.txt:1-3`.
3. **Installed config over/understates deps.** MPI is optional in the build but unconditional for consumers; OpenCL/CUDA/Draco/RDB handling is incomplete or relies on raw exported paths/custom modules not all installed. Refs: `CMakeLists.txt:121`, `CMakeModules/lvr2-config.cmake.in:45-62`, `CMakeLists.txt:885-890`.
4. **Package metadata drift.** README, CPack, `package.xml`, Debian control, and CI disagree on VTK/TBB/TIFF/CUDA/OpenCL/MPI surface. Refs: `README.md:32-40`, `CMakeModules/lvr2-packaging.cmake:44-60`, `package.xml:36-51`, `debian/control:4-11`, `.github/workflows/cmake-multi-platform-build.yml:49-51`.
5. **OpenGL is not declared required, but public API needs it.** `find_package(OpenGL COMPONENTS OpenGL)` lacks REQUIRED while public headers include GL/GLUT. Refs: `CMakeLists.txt:234`, `include/lvr2/display/Renderable.hpp:45-49`, `include/lvr2/algorithm/Tesselator.hpp:44-49`.
6. **Legacy global CMake style.** Heavy `include_directories()`/`link_directories()` and raw `${Boost_*_LIBRARY}`/`${OPENGL_LIBRARIES}` lists make exported targets fragile and non-relocatable. Refs: `CMakeLists.txt:92`, `CMakeLists.txt:220-221`, `CMakeLists.txt:683-706`, `src/liblvr2/CMakeLists.txt:268-272`.
7. **Vendored maintenance/license load.** `laslib`/LASzip is LGPL-2.1 and built into shipped libs; `psimpl` is MPL-1.1; `kintinuous` is obsolete/dead; `HighFive` is vendored but intentionally not built as a subdir. Refs above.
8. **Network build path.** `LVR2_WITH_3DTILES` downloads Cesium Native during build via `ExternalProject_Add`; no offline source or hash check visible beyond tag pin. Refs: `CMakeLists.txt:542-545`.
9. **Windows/MSVC path assumes missing vcpkg checkout.** `CMAKE_TOOLCHAIN_FILE` points to `${source}/vcpkg/...`, but no `vcpkg/` exists in repo. Ref: `CMakeLists.txt:28-30`.
10. **Legacy Debian install snippets likely stale.** They expect `usr/share/HighFive/*`, while current CMake installs HighFive headers into `include`. Refs: `debian/liblvr2-dev.install:2-5`, `CMakeLists.txt:893-897`.

## Strip / simplify ideas

- Normalize feature flags: support only `LVR2_WITH_*`, optionally map old `WITH_*` names with deprecation warnings, then fix 3DTiles/Freenect/Debian callers.
- Make optional deps target-local and feature-local. Core library should not probe CUDA/OpenCL/viewer/3DTiles by default unless the feature is requested.
- Add `LVR2_BUILD_SHARED` / `LVR2_BUILD_STATIC` and similar for vendored `rply`/`laslib`; stop always building both.
- Replace raw global include/link dirs with imported targets (`TBB::tbb`, `LZ4::LZ4`, `Boost::...`, `OpenGL::GL`, `HDF5::...`, `yaml-cpp::yaml-cpp`) and export only true public deps.
- Decide vendoring policy: use system packages for spdlog/HighFive/nanoflann where available, or add explicit `LVR2_USE_BUNDLED_*` switches and license/security tracking.
- Remove or quarantine `ext/kintinuous` + `LVR2_WITH_KINFU` if not supported; it pulls old OpenCV 2.4/nonfree/OpenNI2/CUDA assumptions.
- For 3DTiles, prefer a documented system/pinned package path or FetchContent/ExternalProject with hash/offline mirror support.
- Generate package dependencies from one source (CMake options/features) and sync README, CPack, `package.xml`, Debian, and CI.
- Fix installed `lvr2-config.cmake`: no unconditional optional deps; install any custom find modules needed by enabled optional features, or do not export raw optional paths.
