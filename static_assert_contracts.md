# Static assertion contract inventory

This inventory records compile-time contracts that protect modernization
assumptions while keeping the public build default at C++17.

## Mesh facade public header

File: `include/lvr2/mesh/io.hpp`

- `mesh::Format` and `mesh::ErrorCode` remain scoped enum vocabularies and do
  not implicitly convert to integers.
- `mesh::Result<T>` and `mesh::Status` remain aliases backed by
  `tl::expected`, matching the accepted C++17 result policy.
- `LoadOptions` and `SaveOptions` remain default-constructible and copyable.
- Default load options keep suffix-based format detection.
- Default save options keep suffix-based format detection and binary output.

## Private mesh backend

File: `src/liblvr2/io/AssimpMeshAdapter.cpp`

- `MeshBuffer` face indices remain compatible with the private backend index
  type.
- Assimp mesh vertex counts, face index counts, and face indices remain
  `unsigned int` compatible with the current `MeshBuffer` index guard.
- Assimp vertex coordinates remain floating-point values before conversion to
  LVR mesh-buffer floats.

## Geometry aliases

File: `include/lvr2/types/MatrixTypes.hpp`

- Row-major matrix aliases stay fixed-size and row-major for serialized
  transform/rotation data.
- Transform and rotation aliases stay fixed-size 4x4 and 3x3 matrices.
- Vector, distortion, and 6D matrix aliases keep their compile-time dimensions.

## Compile coverage

File: `tests/test_static_assert_contracts.cpp`

- Compiles the contract-bearing public headers with `cxx_std_17`.
- Rechecks representative mesh result and geometry layout assumptions in a
  compile-only CTest target.
