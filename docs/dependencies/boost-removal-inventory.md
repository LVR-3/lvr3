# Boost retirement inventory

Date: 2026-05-17

## Purpose

This inventory classifies the active Boost surface before the C++20 Boost-removal work. The target end state is no Boost in public headers, implementation code, CMake exports, vcpkg manifests, ROS/Debian metadata, or CI package installation lists unless a future ADR accepts a narrow exception.

The inventory deliberately maps work to public workstreams rather than repository-internal planning identifiers:

- standard-library replacement;
- buffer ownership/span replacement;
- CLI parser replacement;
- hard-case storage/reconstruction cleanup;
- final build/package dependency cleanup.

## Scan scope and snapshot

Scanned active roots: `include`, `src`, `examples`, `tests`, `cmake`, `package.xml`, `vcpkg.json`, `CMakePresets.json`, `debian`, `.github`, and `.gitlab-ci.yml`. Historical Markdown migration notes and the inventory guard script itself are excluded.

- Active files with Boost tokens: 224
- Public headers with active Boost tokens: 108
- `#include <boost/...>` directives: 191 across 30 distinct Boost headers
- Active `boost::...` references: 968 across 48 distinct namespace names
- Active `BOOST_*` macro references: 2
- Build/package/CI Boost references: 67

| Root | Active files |
|---|---:|
| `include` | 108 |
| `src` | 102 |
| `cmake` | 4 |
| `tests` | 3 |
| `examples` | 2 |
| `debian` | 2 |
| `package.xml` | 1 |
| `vcpkg.json` | 1 |
| `.github` | 1 |

## Classification by replacement family

| Family | Replacement | Owner workstream | Matches | Files | Public headers | `src` | examples | tests | CMake | package/CI |
|---|---|---|---:|---:|---:|---:|---:|---:|---:|---:|
| filesystem | `std::filesystem` | standard-library replacement | 379 | 71 | 26 | 42 | 0 | 0 | 1 | 2 |
| optional | `std::optional` | standard-library replacement | 303 | 86 | 62 | 22 | 2 | 0 | 0 | 0 |
| variant / visitor | `std::variant` + `std::visit` | standard-library replacement | 41 | 14 | 13 | 1 | 0 | 0 | 0 | 0 |
| shared_array | explicit owner + `std::span` view | buffer ownership/span replacement | 220 | 49 | 36 | 11 | 0 | 2 | 0 | 0 |
| shared_ptr | `std::shared_ptr` / `std::make_shared` | standard-library replacement | 61 | 21 | 13 | 8 | 0 | 0 | 0 | 0 |
| program_options | LVR-owned typed CLI parser | CLI parser replacement | 79 | 44 | 1 | 40 | 0 | 0 | 1 | 2 |
| iostreams / mapped file | byte-span memory-map adapter or simpler file I/O | hard-case storage/reconstruction cleanup | 26 | 9 | 4 | 2 | 0 | 0 | 1 | 2 |
| serialization / archive | explicit versioned serializers | hard-case storage/reconstruction cleanup | 3 | 2 | 1 | 0 | 0 | 0 | 0 | 1 |
| thread / mutex | `std::thread`, `std::jthread`, `std::mutex`, `std::scoped_lock` | standard-library replacement; hard-case audit for deprecated/device paths | 16 | 7 | 3 | 1 | 0 | 0 | 1 | 2 |
| timer | `std::chrono` timers | standard-library replacement | 9 | 4 | 2 | 0 | 0 | 0 | 0 | 2 |
| algorithm / range helpers | standard/ranges helpers | standard-library replacement | 30 | 5 | 0 | 5 | 0 | 0 | 0 | 0 |
| type_index / typeinfo | `std::type_index` or explicit channel type vocabulary | standard-library replacement; channel vocabulary review | 9 | 5 | 4 | 1 | 0 | 0 | 0 | 0 |
| property_tree | explicit XML/config parser | hard-case tool cleanup | 3 | 1 | 0 | 1 | 0 | 0 | 0 | 0 |
| format | `std::format` or logging facade | standard-library replacement | 3 | 1 | 0 | 1 | 0 | 0 | 0 | 0 |
| foreach | range-for | standard-library replacement | 1 | 1 | 0 | 1 | 0 | 0 | 0 | 0 |
| boost.system | `std::error_code` / filesystem errors | standard-library replacement; Riegl tool audit | 10 | 4 | 0 | 2 | 0 | 0 | 1 | 1 |
| Boost.MPI package path | remove package if no Boost.MPI code remains; otherwise isolate optional MPI path | hard-case dependency cleanup | 5 | 3 | 0 | 0 | 0 | 0 | 1 | 2 |
| DateTime / Boost.Log link remnants | remove stale link references or replace with std/spdlog path | hard-case dependency cleanup | 5 | 4 | 0 | 3 | 0 | 0 | 0 | 1 |
| generic Boost build dependency | delete after all code/package component owners are removed | final build/package dependency cleanup | 18 | 9 | 0 | 2 | 0 | 1 | 3 | 3 |

## Owner path groups

| Owner path group | Files with Boost | Main replacement families |
|---|---:|---|
| I/O storage / schemas / kernels | 45 | filesystem (28), optional (25), shared_array (16), shared_ptr (4), algorithm / range helpers (2), type_index / typeinfo (1), thread / mutex (1), format (1) |
| algorithms | 21 | optional (15), variant / visitor (4), filesystem (3), shared_ptr (3), shared_array (2), type_index / typeinfo (1) |
| attribute maps | 10 | optional (10), shared_array (4) |
| build / package / CI | 7 | generic Boost build dependency (6), iostreams / mapped file (3), thread / mutex (3), Boost.MPI package path (3), filesystem (3), program_options (3), boost.system (2), timer (2), serialization / archive (1), DateTime / Boost.Log link remnants (1) |
| configuration / CLI base | 1 | program_options (1) |
| examples | 2 | optional (2) |
| geometry | 5 | optional (4), shared_ptr (1) |
| other public/private code | 5 | optional (3), generic Boost build dependency (1), shared_ptr (1) |
| reconstruction | 18 | shared_ptr (8), shared_array (7), filesystem (6), iostreams / mapped file (3), timer (2), thread / mutex (1), optional (1), serialization / archive (1) |
| tests | 3 | shared_array (2), generic Boost build dependency (1) |
| tools / CLI frontends | 66 | program_options (40), filesystem (22), optional (8), shared_array (3), DateTime / Boost.Log link remnants (3), boost.system (2), algorithm / range helpers (2), property_tree (1), generic Boost build dependency (1), foreach (1), type_index / typeinfo (1), iostreams / mapped file (1), shared_ptr (1) |
| types / channels / buffers | 20 | shared_array (11), optional (11), variant / visitor (10), type_index / typeinfo (2), shared_ptr (1), filesystem (1) |
| utilities | 19 | filesystem (8), optional (7), shared_array (4), thread / mutex (2), iostreams / mapped file (2), shared_ptr (2), algorithm / range helpers (1) |

## Priority order

1. **Public header leakage first.** Remove Boost from `include/lvr2/types`, `include/lvr2/io`, `include/lvr2/config`, and public reconstruction headers before deleting build-package dependencies. These headers currently force consumers to have Boost even when they only use the modern facade targets.
2. **Standard-equivalent replacements next.** `boost::filesystem`, `boost::optional`, `boost::variant`, `boost::shared_ptr`, thread/mutex, timer, algorithm/range helpers, type-index helpers, `boost::format`, `boost::foreach`, and Boost.System should move to C++20 standard facilities without inventing a new utility layer.
3. **Buffer ownership/span replacement.** `boost::shared_array` is widespread in channel and buffer APIs. Replace it with explicit owners (`std::vector<T>` or `std::unique_ptr<T[]>`) plus `std::span` views, and add lifetime tests with the API-changing work.
4. **CLI parser replacement.** `boost::program_options` is concentrated in `BaseOption` and tool option classes. Replace it with a typed C++20 parser while preserving CLI behavior unless a test-backed ADR changes it.
5. **Hard cases.** `boost::iostreams::mapped_file`, Boost archive serialization, Boost.MPI packaging, stale Boost.Log/DateTime link references, and Riegl Boost.System errors need focused review because they touch file layout, binary data, optional MPI/tool paths, or external SDK behavior.
6. **Final build/package cleanup.** Remove `find_package(Boost)`, installed `find_package(Boost ...)`, vcpkg Boost ports, ROS/Debian Boost dependencies, and CI package installs only after their code owners are gone.

## Public header leakage list

The list below is generated from active code tokens after stripping comments and string literals from C++ files. Each row gives the replacement families present in that public header.

| Public header | Families |
|---|---|
| `include/lvr2/algorithm/BaseBufferManipulators.hpp` | shared_array; type_index / typeinfo; variant / visitor |
| `include/lvr2/algorithm/ChunkHashGrid.hpp` | optional; variant / visitor |
| `include/lvr2/algorithm/ChunkHashGrid.tcc` | optional; variant / visitor |
| `include/lvr2/algorithm/ChunkManager.tcc` | shared_array |
| `include/lvr2/algorithm/ChunkingPipeline.hpp` | filesystem |
| `include/lvr2/algorithm/ChunkingPipeline.tcc` | filesystem |
| `include/lvr2/algorithm/ClusterAlgorithms.tcc` | optional |
| `include/lvr2/algorithm/ColorAlgorithms.hpp` | optional |
| `include/lvr2/algorithm/ColorAlgorithms.tcc` | optional |
| `include/lvr2/algorithm/FinalizeAlgorithms.hpp` | optional; shared_ptr |
| `include/lvr2/algorithm/GeometryAlgorithms.tcc` | optional |
| `include/lvr2/algorithm/HLODTree.hpp` | optional |
| `include/lvr2/algorithm/Materializer.hpp` | optional; shared_ptr |
| `include/lvr2/algorithm/Materializer.tcc` | optional |
| `include/lvr2/algorithm/NormalAlgorithms.hpp` | optional |
| `include/lvr2/algorithm/NormalAlgorithms.tcc` | optional |
| `include/lvr2/algorithm/ReductionAlgorithms.hpp` | optional |
| `include/lvr2/algorithm/ReductionAlgorithms.tcc` | optional |
| `include/lvr2/algorithm/Texturizer.hpp` | shared_ptr |
| `include/lvr2/attrmaps/AttrMaps.hpp` | optional |
| `include/lvr2/attrmaps/AttributeMap.hpp` | optional |
| `include/lvr2/attrmaps/HashMap.hpp` | optional |
| `include/lvr2/attrmaps/HashMap.tcc` | optional |
| `include/lvr2/attrmaps/ListMap.hpp` | optional |
| `include/lvr2/attrmaps/ListMap.tcc` | optional |
| `include/lvr2/attrmaps/StableVector.hpp` | optional; shared_array |
| `include/lvr2/attrmaps/StableVector.tcc` | optional; shared_array |
| `include/lvr2/attrmaps/VectorMap.hpp` | optional; shared_array |
| `include/lvr2/attrmaps/VectorMap.tcc` | optional; shared_array |
| `include/lvr2/config/BaseOption.hpp` | program_options |
| `include/lvr2/geometry/BaseMesh.hpp` | optional |
| `include/lvr2/geometry/BoundingBox.tcc` | shared_ptr |
| `include/lvr2/geometry/HalfEdgeMesh.tcc` | optional |
| `include/lvr2/geometry/PMPMesh.hpp` | optional |
| `include/lvr2/geometry/pmp/SurfaceMesh.h` | optional |
| `include/lvr2/io/AttributeMeshIOBase.hpp` | optional; shared_array; shared_ptr |
| `include/lvr2/io/AttributeMeshIOBase.tcc` | optional |
| `include/lvr2/io/DataStruct.hpp` | shared_array |
| `include/lvr2/io/LineReader.hpp` | shared_array; shared_ptr |
| `include/lvr2/io/ScanDirectoryParser.hpp` | filesystem |
| `include/lvr2/io/Tiles3dIO.tcc` | filesystem |
| `include/lvr2/io/deprecated/KinectGrabber.hpp` | thread / mutex |
| `include/lvr2/io/detail/DirectoryDataIO.hpp` | shared_array |
| `include/lvr2/io/detail/DirectoryDataIO.tcc` | shared_array |
| `include/lvr2/io/detail/MetaFormatFactory.hpp` | filesystem |
| `include/lvr2/io/kernels/DirectoryKernel.hpp` | filesystem; optional; shared_array |
| `include/lvr2/io/kernels/FileKernel.hpp` | optional; shared_array |
| `include/lvr2/io/kernels/FileKernel.tcc` | shared_array; type_index / typeinfo |
| `include/lvr2/io/kernels/HDF5Kernel.hpp` | optional; shared_array |
| `include/lvr2/io/kernels/HDF5Kernel.tcc` | optional; shared_array |
| `include/lvr2/io/schema/LabelScanProjectSchemaHDF5V2.hpp` | filesystem; optional |
| `include/lvr2/io/schema/ScanProjectSchema.hpp` | filesystem; optional |
| `include/lvr2/io/schema/ScanProjectSchemaEuRoC.hpp` | filesystem; optional |
| `include/lvr2/io/schema/ScanProjectSchemaHDF5.hpp` | filesystem; optional |
| `include/lvr2/io/schema/ScanProjectSchemaHDF5V2.hpp` | filesystem; optional |
| `include/lvr2/io/schema/ScanProjectSchemaHyperlib.hpp` | filesystem; optional |
| `include/lvr2/io/schema/ScanProjectSchemaRaw.hpp` | filesystem; optional |
| `include/lvr2/io/schema/ScanProjectSchemaRdbx.hpp` | filesystem; optional |
| `include/lvr2/io/schema/ScanProjectSchemaSlam6D.hpp` | filesystem; optional |
| `include/lvr2/reconstruction/AdaptiveKSearchSurface.tcc` | filesystem |
| `include/lvr2/reconstruction/BigGrid.hpp` | filesystem; iostreams / mapped file |
| `include/lvr2/reconstruction/BigGrid.tcc` | iostreams / mapped file; shared_array; shared_ptr |
| `include/lvr2/reconstruction/BigVolumen.hpp` | iostreams / mapped file; serialization / archive |
| `include/lvr2/reconstruction/BigVolumen.tcc` | shared_ptr |
| `include/lvr2/reconstruction/DualOctree.hpp` | thread / mutex |
| `include/lvr2/reconstruction/HashGrid.tcc` | shared_array |
| `include/lvr2/reconstruction/LargeScaleReconstruction.tcc` | filesystem; optional |
| `include/lvr2/reconstruction/NodeData.hpp` | timer |
| `include/lvr2/reconstruction/NodeData.tcc` | filesystem; timer |
| `include/lvr2/reconstruction/SearchTreeFlann.hpp` | shared_array |
| `include/lvr2/reconstruction/SearchTreeFlann.tcc` | shared_array |
| `include/lvr2/reconstruction/cuda/CudaSurface.hpp` | shared_array; shared_ptr |
| `include/lvr2/reconstruction/opencl/ClStatisticalOutlierFilter.hpp` | filesystem; shared_array; shared_ptr |
| `include/lvr2/reconstruction/opencl/ClSurface.hpp` | filesystem; shared_array; shared_ptr |
| `include/lvr2/texture/ClusterTexCoordMapping.hpp` | optional |
| `include/lvr2/texture/Material.hpp` | optional |
| `include/lvr2/types/BaseBuffer.hpp` | optional; shared_array; variant / visitor |
| `include/lvr2/types/BaseBuffer.tcc` | optional; shared_array; variant / visitor |
| `include/lvr2/types/ByteEncoding.hpp` | optional; shared_array |
| `include/lvr2/types/CameraModels.hpp` | optional |
| `include/lvr2/types/Channel.hpp` | optional; shared_array; type_index / typeinfo |
| `include/lvr2/types/CustomChannelTypes.hpp` | optional; shared_array |
| `include/lvr2/types/ElementProxy.hpp` | optional |
| `include/lvr2/types/Model.hpp` | shared_ptr |
| `include/lvr2/types/MultiChannelMap.hpp` | optional |
| `include/lvr2/types/PointBuffer.hpp` | shared_array; variant / visitor |
| `include/lvr2/types/PolygonBuffer.hpp` | shared_array |
| `include/lvr2/types/ScanTypes.hpp` | filesystem; optional; variant / visitor |
| `include/lvr2/types/Variant.hpp` | type_index / typeinfo; variant / visitor |
| `include/lvr2/types/Variant.tcc` | variant / visitor |
| `include/lvr2/types/VariantChannel.hpp` | optional; shared_array; variant / visitor |
| `include/lvr2/types/VariantChannel.tcc` | shared_array; variant / visitor |
| `include/lvr2/types/VariantChannelMap.hpp` | variant / visitor |
| `include/lvr2/types/VariantChannelMap.tcc` | variant / visitor |
| `include/lvr2/types/WaveformBuffer.hpp` | shared_array |
| `include/lvr2/util/BaseHandle.hpp` | optional |
| `include/lvr2/util/ConvertShared.hpp` | shared_ptr |
| `include/lvr2/util/Debug.hpp` | iostreams / mapped file |
| `include/lvr2/util/Hdf5Util.hpp` | filesystem; optional; shared_array |
| `include/lvr2/util/Hdf5Util.tcc` | optional; shared_array |
| `include/lvr2/util/IOUtils.hpp` | filesystem |
| `include/lvr2/util/IOUtils.tcc` | filesystem |
| `include/lvr2/util/Logging.hpp` | filesystem |
| `include/lvr2/util/Meap.hpp` | optional |
| `include/lvr2/util/Meap.tcc` | optional |
| `include/lvr2/util/Progress.hpp` | thread / mutex |
| `include/lvr2/util/ScanProjectUtils.hpp` | optional |
| `include/lvr2/util/Util.hpp` | shared_array |

## Build, package, and CI owner map

| File | Current Boost role | Cleanup owner |
|---|---|---|
| `.github/workflows/build-and-attach-release-assets.yml` | legacy release-asset workflow installs `libboost-all-dev` | update or remove with CI/package cleanup |
| `cmake/Lvr3Dependencies.cmake` | required Boost component discovery, global include/link variables, optional MPI component, and component libraries for core/tools | final build/package dependency cleanup after code owners land |
| `cmake/Lvr3DependencyProvider.cmake` | keeps Boost in the package/system opt-out vocabulary | remove once no Boost package is discoverable |
| `cmake/Lvr3Packaging.cmake` | generates Debian package dependencies from `Boost_COMPONENTS` | remove with CMake dependency cleanup |
| `cmake/lvr2-config.cmake.in` | installed package still runs `find_package(Boost COMPONENTS ...)` | remove after public headers and exported/static link interfaces no longer need Boost |
| `debian/README.Debian` | documents Boost as an installation dependency | update with packaging cleanup |
| `debian/control` | declares Boost development packages for build/runtime surfaces | remove with packaging cleanup |
| `package.xml` | declares generic `boost` ROS dependency | remove when public and implementation code are Boost-free |
| `src/liblvr2/CMakeLists.txt` | propagates `${Boost_INCLUDE_DIRS}` to the library target | remove after public/private headers stop including Boost |
| `src/tools/lvr2_chunking_server/CMakeLists.txt` | links `${Boost_LIBRARIES}` and program-options library | remove with CLI parser replacement |
| `src/tools/lvr2_dmc_reconstruction/CMakeLists.txt` | links stale `${Boost_LOG_LIBRARY_RELEASE}` | remove with hard-case dependency cleanup |
| `src/tools/lvr2_fastsense_reconstruction/CMakeLists.txt` | links `boost_date_time` and stale `${Boost_LOG_LIBRARY_RELEASE}` | remove with hard-case dependency cleanup |
| `src/tools/lvr2_gs_reconstruction/CMakeLists.txt` | links stale `${Boost_LOG_LIBRARY_RELEASE}` | remove with hard-case dependency cleanup |
| `tests/CMakeLists.txt` | uses `${Boost_INCLUDE_DIRS}` for focused legacy-private compile paths | remove after affected private includes are Boost-free |
| `vcpkg.json` | declares boost-date-time, filesystem, iostreams, mpi, program-options, serialization, thread, and timer ports | remove each port as its owner workstream lands; remove all in final cleanup |

## Hard-case assignments

| Hard case | Current locations | Required decision before deletion |
|---|---|---|
| `boost::iostreams::mapped_file` | `include/lvr2/reconstruction/BigGrid.hpp`, `include/lvr2/reconstruction/BigGrid.tcc`, `include/lvr2/reconstruction/BigVolumen.hpp`, `include/lvr2/util/Debug.hpp`, plus private reconstruction code | Choose an internal byte-span mmap adapter or simpler buffered file I/O; preserve large-grid behavior with smoke tests. |
| Boost archive serialization | `include/lvr2/reconstruction/BigVolumen.hpp` and package metadata | Replace with explicit versioned binary records or delete unreachable serialization path. |
| `boost::program_options` | `include/lvr2/config/BaseOption.hpp` and many `src/tools/*/Options.*` files | Introduce a typed parser over `std::span<char const* const>` / `std::string_view`; preserve current help, defaults, aliases, and parse errors unless test-backed migration notes say otherwise. |
| Boost.MPI packaging | `cmake/Lvr3Dependencies.cmake`, `vcpkg.json`, `debian/control` | No active `boost/mpi` or `boost::mpi` code was found; either remove the package path or isolate an optional MPI path if follow-up review finds one. |
| stale Boost.Log / DateTime links | `src/tools/lvr2_dmc_reconstruction`, `src/tools/lvr2_fastsense_reconstruction`, `src/tools/lvr2_gs_reconstruction` CMake files | Verify the tools no longer need Boost.Log/DateTime after std-format spdlog migration; remove direct link variables. |
| Riegl Boost.System errors | `src/tools/lvr2_riegl_project_converter/RieglProject.hpp` and `.cpp` | Replace with `std::error_code` or SDK-native errors without changing external converter behavior. |

## Regeneration commands

Quick active-token inventory:

```bash
rg -n --glob '!docs/**' --glob '!tests/boost_retirement_inventory.cmake' '#\s*include\s*[<"]boost/|\bboost::|\bBoost\b|\bBOOST_|boost-[A-Za-z0-9.+-]+|libboost-[A-Za-z0-9.+-]+|Boost_[A-Za-z0-9_]+|<depend>boost</depend>|libboost-all-dev' include src examples tests cmake package.xml vcpkg.json CMakePresets.json debian .github .gitlab-ci.yml
```

The snapshot counts above use the same roots but strip C++ comments/string literals, CMake/hash comments, and XML comments before counting, so raw `rg` output may report a few extra historical comment strings.
