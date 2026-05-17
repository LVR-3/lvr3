# BaseIO removal inventory and final contracts

Status: final deletion slice applied. The old CRTP storage feature surface has been removed from the implementation include tree.

## Final state

Deleted public/implementation feature roots:

- `include/lvr2/io/baseio`
- `include/lvr2/io/meshio`
- `include/lvr2/io/scanio`
- `include/lvr2/io/deprecated/hdf5`
- `include/lvr2/io/ChunkIO.hpp`
- unbuilt legacy `src/liblvr2/io/ChunkIO.cpp`, `src/liblvr2/io/HDF5IO.cpp`, and label scan-project CRTP sources

Neutral helpers kept after moving out of split namespace paths:

- YAML conversions now live under `include/lvr2/io/yaml`.
- Directory-kernel helper data/meta readers now live under `include/lvr2/io/detail` and `src/liblvr2/io/detail`.

Replacement services kept under the unified namespace:

- `lvr2::io::storage::StorageBackend`
- `lvr2::io::storage::StorageRegistry`
- `lvr2::io::storage::StorageContext`
- `lvr2::io::scan::ProjectStore`
- `lvr2::io::scan::{open_project, open_directory, open_hdf5, load_project, save_project}`

## Contract

- Directory, HDF5, fake/test, and future custom/IP backends use the same `StorageRegistry` factory path.
- Backend dispatch remains at group/dataset/metadata boundaries, not per point, vertex, pixel, scalar, or channel element.
- The C++20 storage contract uses `std::span<std::byte>` / `std::span<const std::byte>` for raw byte dataset boundaries and `TypedArrayView<T>`/`FloatArrayView` typed spans for contiguous arrays.
- `StorageRegistry::add(...)` accepts concept-checked, function-pointer-compatible factories; it does not add a second custom/plugin backend path.
- Public scan-project usage is `ProjectStore`/one-shot helper based.
- No compatibility CRTP aliases are provided.
- Mesh HDF5/directory tool compatibility that was still reachable is served by small non-template stores in the private build include tree, not by public `meshio` feature templates.
- Chunk persistence now uses the non-template public header `lvr2::io::storage::ChunkStore`; it opens the HDF5 backend through `StorageRegistry` for point chunks and float metadata, while mesh chunks and size metadata use narrow HDF5 compatibility glue.

## Guard coverage

`tests/storage_io_contract_guard.cmake` is now a final-state guard. It fails if removed feature roots reappear or if code under `include`, `src`, `examples`, or `tests` reintroduces the old feature-composition vocabulary, base-qualified scan-project calls, split public storage namespaces, split built-in/custom backend guidance, `std::function` storage factories, or pointer-only storage array views.

`tests/test_storage_backend_span_contracts.cpp` covers byte-span read/write lifetime and typed-array span copying through a fake backend opened by `StorageRegistry`. `tests/test_storage_project_store.cpp` remains the focused runtime contract for fake, directory, and HDF5 scan-project storage through the same registry path.

## Validation commands

```bash
python3 -m json.tool CMakePresets.json >/dev/null
cmake -S . -B build-storage-final \
  -DBUILD_SHARED_LIBS=ON \
  -DLVR2_BUILD_STATIC_LIBS=OFF \
  -DLVR2_WITH_VCPKG=OFF \
  -DLVR2_BUILD_TESTS=ON \
  -DLVR2_BUILD_TOOLS=ON
cmake --build build-storage-final --target \
  lvr2_storage_io_header_compile \
  lvr2_storage_backend_span_contracts \
  lvr2_storage_project_store_gtest \
  lvr2_reconstruct \
  lvr2_hdf5_mesh_tool
ctest --test-dir build-storage-final --output-on-failure \
  -R 'storage_io_contract_guard|unified_io_namespace_guard|storage_project_store|storage_io_header_compile|storage_backend_span_contracts'
```

When host dependencies are unavailable, run a Python mirror of the final grep guard and record the missing dependency/cmake blockers in progress.

## Performance baseline commands

Use the non-gating JSON recorder before/after representative storage operations:

```bash
python3 tests/performance/record_baseline.py \
  --output build-storage-final/performance-baselines/storage-projectstore-directory-hdf5.json \
  --label storage-projectstore-directory-hdf5
```

Representative manual timing commands should exercise:

1. generated directory scan-project save/load via `lvr2::io::scan::save_project/load_project`,
2. generated HDF5 scan-project save/load through the same helpers, and
3. chunk/mesh HDF5 operations used by `lvr2_reconstruct` and `lvr2_hdf5_mesh_tool`.
