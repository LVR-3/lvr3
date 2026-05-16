# BaseIO removal inventory and contracts

This inventory records the remaining CRTP storage I/O surface before replacing it
with the unified `lvr2::io` storage services. It is intentionally a transition
artifact: the final migration must delete the old public surface instead of
wrapping it indefinitely.

The modern mesh facade now lives at `<lvr2/io/mesh.hpp>` and `lvr2::io::mesh`.
The remaining `baseio`, `scanio`, `meshio`, and `modelio` CRTP/storage families
are not being renamed into `lvr2::io`; they remain deletion targets for the
storage service slices.

Refreshed from implementation commit `631fa7a049e4b01324a2d58b4ae8f651200452b1`.
Regenerate the counts from the repository root with:

```bash
python3 - <<'PY'
from pathlib import Path
terms = ['BaseIO','FeatureBuild','FeatureConstruct','AddFeatures','Merge<','ScanProjectIO::','DirectoryIO','HDF5IO']
roots = ['include','src','examples','tests','CMakeLists.txt','cmake']
skip = {Path('tests/storage_io_contract_guard.cmake')}
files=[]
for root in roots:
    p=Path(root)
    if p.is_file():
        files.append(p)
    elif p.exists():
        files += [f for f in p.rglob('*') if f.is_file() and f not in skip]
for term in terms:
    matches=[]
    for f in files:
        text=f.read_text(errors='ignore')
        count=text.count(term)
        if count:
            matches.append((f,count))
    print(term, sum(count for _, count in matches), len(matches))
PY
```

## Replacement contracts

The replacement design is constrained by the unified I/O namespace and one-path
backend policy:

- Public replacement names live under `lvr2::io`, specifically
  `lvr2::io::storage` for storage primitives and `lvr2::io::scan` for scan
  project services.
- `lvr2::io::storage::StorageRegistry` opens every backend. Built-in Directory,
  built-in HDF5, fake/test, plugin, and future custom/IP backends all register a
  factory function and return a `std::unique_ptr<lvr2::io::storage::StorageBackend>`.
- There is no built-in-only selector, no second custom-backend extension path,
  no `std::variant` backend switch, and no CRTP compatibility alias.
- `StorageBackend` dispatch stays at the dataset/group/attribute boundary. It
  must not dispatch per point, vertex, pixel, scalar, or channel element.
- Domain services such as `ProjectStore`, `ScanPositionStore`, `ScanStore`,
  `ChannelStore`, and `MetaStore` are final/non-template services composed from a
  `StorageContext`.
- New result-returning C++17 APIs use LVR-owned aliases backed by
  `tl::expected`; do not introduce another expected implementation.

## Term inventory

The numeric totals are a historical baseline captured before later storage-service and bundled-viewer removal work. Regenerate the counts before using them for current totals; the migration targets and contracts below remain authoritative.

| Term | Matches | Files | Main concentration |
|---|---:|---:|---|
| `BaseIO` | 464 | 59 | `baseio`, `scanio`, `meshio`, public utility bridges |
| `FeatureBuild` | 25 | 12 | `BaseIO.hpp`, `scanio`/`meshio` typedefs, historical viewer state removed with the bundled viewer |
| `FeatureConstruct` | 81 | 22 | dependency wiring in `BaseIO.hpp`, `scanio`, `meshio` |
| `AddFeatures` | 7 | 6 | old HDF5 tool/builders and label scan-project bridge |
| `Merge<` | 31 | 20 | feature dependency composition in `scanio`, `meshio`, deprecated HDF5 |
| `ScanProjectIO::` | 23 | 8 | examples, `ScanProjectUtils`, largescale reconstruct, chunking pipeline |
| `DirectoryIO` | 60 | 14 | examples, `ScanProjectUtils`, viewer, scan-project wrappers |
| `HDF5IO` | 163 | 29 | legacy HDF5 utility class, examples/tools, scan-project wrappers |

| Area | Files with terms | Matches | Removal target |
|---|---:|---:|---|
| `include/lvr2/io/baseio` | 11 | 118 | Delete after storage services replace primitive array/channel/matrix/meta operations. |
| `include/lvr2/io/scanio` | 33 | 386 | Rewrite as `lvr2::io::scan` services and delete CRTP feature headers. |
| `include/lvr2/io/meshio` | 12 | 100 | Delete or route through existing mesh facade/storage services; do not keep public `meshio`. |
| `include/lvr2/io/deprecated/hdf5` | 8 | 18 | Delete or rewrite only reachable helpers; no deprecated CRTP carry-forward. |
| `src/tools` | 15 | 82 | Migrate remaining non-viewer tools to `ProjectStore`/one-shot helpers; bundled viewer paths were removed. |
| `examples/scan_projects` | 4 | 41 | Rewrite examples to show `lvr2::io::scan` usage only. |
| `src/liblvr2` utility/bridge files | 6 | 97 | Convert utility functions to call the new one-shot helpers internally. |

## Complete file inventory

- `examples/scan_projects/compression/Main.cpp` — `ScanProjectIO::`:1, `DirectoryIO`:2, `HDF5IO`:3
- `examples/scan_projects/loadpartial/Main.cpp` — `ScanProjectIO::`:4, `DirectoryIO`:5, `HDF5IO`:6
- `examples/scan_projects/schema/Main.cpp` — `ScanProjectIO::`:3, `DirectoryIO`:5, `HDF5IO`:2
- `examples/scan_projects/simple/Main.cpp` — `ScanProjectIO::`:2, `DirectoryIO`:5, `HDF5IO`:3
- `include/lvr2/algorithm/ChunkingPipeline.tcc` — `ScanProjectIO::`:2, `DirectoryIO`:2, `HDF5IO`:3
- `include/lvr2/io/ChunkIO.hpp` — `HDF5IO`:2
- `include/lvr2/io/baseio/ArrayIO.hpp` — `BaseIO`:5
- `include/lvr2/io/baseio/ArrayIO.tcc` — `BaseIO`:24
- `include/lvr2/io/baseio/BaseIO.hpp` — `BaseIO`:21, `FeatureBuild`:3, `FeatureConstruct`:5, `AddFeatures`:1
- `include/lvr2/io/baseio/BaseIO.tcc` — `BaseIO`:8
- `include/lvr2/io/baseio/ChannelIO.hpp` — `BaseIO`:3
- `include/lvr2/io/baseio/ChannelIO.tcc` — `BaseIO`:16
- `include/lvr2/io/baseio/MatrixIO.hpp` — `BaseIO`:5
- `include/lvr2/io/baseio/MatrixIO.tcc` — `BaseIO`:8
- `include/lvr2/io/baseio/MetaIO.hpp` — `BaseIO`:4
- `include/lvr2/io/baseio/MetaIO.tcc` — `BaseIO`:6
- `include/lvr2/io/baseio/VariantChannelIO.hpp` — `BaseIO`:6, `FeatureConstruct`:3
- `include/lvr2/io/deprecated/hdf5/ChunkIO.hpp` — `Merge<`:2
- `include/lvr2/io/deprecated/hdf5/HDF5FeatureBase.hpp` — `AddFeatures`:1, `HDF5IO`:3
- `include/lvr2/io/deprecated/hdf5/HyperspectralCameraIO.hpp` — `Merge<`:1
- `include/lvr2/io/deprecated/hdf5/MeshIO.hpp` — `Merge<`:2
- `include/lvr2/io/deprecated/hdf5/ScanIO.hpp` — `Merge<`:1
- `include/lvr2/io/deprecated/hdf5/ScanImageIO.hpp` — `Merge<`:1
- `include/lvr2/io/deprecated/hdf5/ScanPositionIO.hpp` — `Merge<`:4
- `include/lvr2/io/deprecated/hdf5/VariantChannelIO.hpp` — `BaseIO`:3
- `include/lvr2/io/meshio/ClusterIO.hpp` — `BaseIO`:3
- `include/lvr2/io/meshio/ClusterIO.tcc` — `BaseIO`:4
- `include/lvr2/io/meshio/DirectoryIO.hpp` — `BaseIO`:1, `FeatureBuild`:2, `FeatureConstruct`:1, `DirectoryIO`:7
- `include/lvr2/io/meshio/FaceIO.hpp` — `BaseIO`:3
- `include/lvr2/io/meshio/FaceIO.tcc` — `BaseIO`:20
- `include/lvr2/io/meshio/HDF5IO.hpp` — `BaseIO`:1, `FeatureBuild`:1, `HDF5IO`:7
- `include/lvr2/io/meshio/MaterialIO.hpp` — `BaseIO`:6, `FeatureConstruct`:3
- `include/lvr2/io/meshio/MaterialIO.tcc` — `BaseIO`:8
- `include/lvr2/io/meshio/MeshIO.hpp` — `BaseIO`:10, `FeatureConstruct`:4, `Merge<`:2
- `include/lvr2/io/meshio/MeshIO.tcc` — `BaseIO`:8
- `include/lvr2/io/meshio/TextureIO.hpp` — `BaseIO`:4, `FeatureConstruct`:1
- `include/lvr2/io/meshio/TextureIO.tcc` — `BaseIO`:4
- `include/lvr2/io/modelio/ModelIOBase.hpp` — `BaseIO`:1
- `include/lvr2/io/scanio/CameraIO.hpp` — `BaseIO`:7, `FeatureConstruct`:5, `Merge<`:2
- `include/lvr2/io/scanio/CameraIO.tcc` — `BaseIO`:10
- `include/lvr2/io/scanio/CameraImageGroupIO.hpp` — `BaseIO`:8, `FeatureConstruct`:5, `Merge<`:1
- `include/lvr2/io/scanio/CameraImageGroupIO.tcc` — `BaseIO`:12
- `include/lvr2/io/scanio/CameraImageIO.hpp` — `BaseIO`:8, `FeatureBuild`:1, `FeatureConstruct`:5, `Merge<`:1
- `include/lvr2/io/scanio/CameraImageIO.tcc` — `BaseIO`:16, `FeatureBuild`:1
- `include/lvr2/io/scanio/ChunkIO.hpp` — `BaseIO`:13, `FeatureConstruct`:5, `Merge<`:2
- `include/lvr2/io/scanio/ChunkIO.tcc` — `BaseIO`:20
- `include/lvr2/io/scanio/DirectoryIO.hpp` — `BaseIO`:1, `FeatureBuild`:1, `DirectoryIO`:8
- `include/lvr2/io/scanio/HDF5IO.hpp` — `BaseIO`:1, `FeatureBuild`:1, `HDF5IO`:7
- `include/lvr2/io/scanio/HyperspectralCameraIO.hpp` — `BaseIO`:7, `FeatureConstruct`:4, `Merge<`:1
- `include/lvr2/io/scanio/HyperspectralCameraIO.tcc` — `BaseIO`:6
- `include/lvr2/io/scanio/HyperspectralPanoramaChannelIO.hpp` — `BaseIO`:7, `FeatureConstruct`:5, `Merge<`:1
- `include/lvr2/io/scanio/HyperspectralPanoramaIO.hpp` — `BaseIO`:9, `FeatureConstruct`:4, `Merge<`:2
- `include/lvr2/io/scanio/ImageIO.hpp` — `BaseIO`:3
- `include/lvr2/io/scanio/ImageIO.tcc` — `BaseIO`:8
- `include/lvr2/io/scanio/LIDARIO.hpp` — `BaseIO`:7, `FeatureConstruct`:4, `Merge<`:1
- `include/lvr2/io/scanio/LIDARIO.tcc` — `BaseIO`:6
- `include/lvr2/io/scanio/LabelHDF5IO.hpp` — `BaseIO`:1, `HDF5IO`:2
- `include/lvr2/io/scanio/LabelIO.hpp` — `BaseIO`:7, `FeatureConstruct`:4, `Merge<`:1
- `include/lvr2/io/scanio/LabelScanProjectIO.hpp` — `BaseIO`:7, `FeatureConstruct`:3, `Merge<`:1
- `include/lvr2/io/scanio/LabelScanProjectIO.tcc` — `BaseIO`:6
- `include/lvr2/io/scanio/PointCloudIO.hpp` — `BaseIO`:5, `FeatureConstruct`:2
- `include/lvr2/io/scanio/PointCloudIO.tcc` — `BaseIO`:20
- `include/lvr2/io/scanio/PolygonIO.hpp` — `BaseIO`:5, `FeatureConstruct`:2
- `include/lvr2/io/scanio/PolygonIO.tcc` — `BaseIO`:6
- `include/lvr2/io/scanio/ScanIO.hpp` — `BaseIO`:9, `FeatureConstruct`:4, `Merge<`:1
- `include/lvr2/io/scanio/ScanIO.tcc` — `BaseIO`:16, `FeatureBuild`:1
- `include/lvr2/io/scanio/ScanPositionIO.hpp` — `BaseIO`:11, `FeatureConstruct`:5, `Merge<`:3
- `include/lvr2/io/scanio/ScanPositionIO.tcc` — `BaseIO`:14
- `include/lvr2/io/scanio/ScanProjectIO.hpp` — `BaseIO`:7, `FeatureConstruct`:3, `Merge<`:1
- `include/lvr2/io/scanio/ScanProjectIO.tcc` — `BaseIO`:12, `HDF5IO`:1
- `include/lvr2/io/scanio/WaveformIO.hpp` — `BaseIO`:6, `FeatureConstruct`:4
- `include/lvr2/reconstruction/BigGrid.tcc` — `HDF5IO`:2
- `src/liblvr2/CMakeLists.txt` — `HDF5IO`:1
- `src/liblvr2/io/HDF5IO.cpp` — `HDF5IO`:63
- `src/liblvr2/io/ModelFactory.cpp` — `HDF5IO`:1
- `src/liblvr2/io/scanio/LabelHDF5IO.cpp` — `AddFeatures`:2, `ScanProjectIO::`:2, `HDF5IO`:5
- `src/liblvr2/util/Hdf5Util.cpp` — `HDF5IO`:2
- `src/liblvr2/util/ScanProjectUtils.cpp` — `ScanProjectIO::`:7, `DirectoryIO`:7, `HDF5IO`:7
- `src/tools/lvr2_3dtiles/Main.cpp` — `HDF5IO`:3
- `src/tools/lvr2_hdf5_builder/HDF5Tool.cpp` — `HDF5IO`:2
- `src/tools/lvr2_hdf5_builder_2/Main.cpp` — `AddFeatures`:1, `HDF5IO`:4
- `src/tools/lvr2_hdf5_convert_old/Main.cpp` — `DirectoryIO`:2, `HDF5IO`:2
- `src/tools/lvr2_hdf5_inspect/Main.cpp` — `HDF5IO`:1
- `src/tools/lvr2_hdf5togeotiff/Main.cpp` — `HDF5IO`:1
- `src/tools/lvr2_largescale_reconstruct/Main.cpp` — `AddFeatures`:1, `ScanProjectIO::`:2, `DirectoryIO`:3, `HDF5IO`:6
- `src/tools/lvr2_largescale_reconstruct_mpi/Main.cpp` — `AddFeatures`:1, `HDF5IO`:6
- `src/tools/lvr2_reconstruct/Main.cpp` — `DirectoryIO`:2, `HDF5IO`:3
- `src/tools/lvr2_scanproject_parser/Main.cpp` — `BaseIO`:1, `DirectoryIO`:1
- `src/tools/lvr2_slam2hdf5/Main.cpp` — `HDF5IO`:2
- Historical `src/tools/lvr2_viewer/*` entries were removed; BaseIO migration work must not target viewer internals.

## Public and bundled migration map

| Current surface | Representative files | Target shape | Migration owner |
|---|---|---|---|
| Manual `DirectoryKernel` + `ScanProjectSchemaRaw` + `scanio::DirectoryIO` | `examples/scan_projects/simple/Main.cpp`, `examples/scan_projects/schema/Main.cpp`, `src/liblvr2/util/ScanProjectUtils.cpp` | `auto store = lvr2::io::scan::open_directory(path, lvr2::io::scan::Schema::raw_ply(), lvr2::io::storage::LoadMode::Lazy); store.save(project); auto loaded = store.load();` for the minimal point-buffer path; raw channel-directory storage remains a later migration step. | Storage service implementation, then example/tool migration. |
| Manual `HDF5Kernel` + `ScanProjectSchemaHDF5` + `scanio::HDF5IO` | `examples/scan_projects/simple/Main.cpp`, `examples/scan_projects/compression/Main.cpp`, `src/tools/lvr2_hdf5_convert_old/Main.cpp` | `auto store = lvr2::io::scan::open_hdf5(path, lvr2::io::scan::Schema::hdf5(), lvr2::io::storage::LoadMode::Lazy);` | Storage service implementation, then example/tool migration. |
| Base-qualified `ScanProjectIO::load/save/loadMeta` calls | `examples/scan_projects/*/Main.cpp`, `src/liblvr2/util/ScanProjectUtils.cpp`, `include/lvr2/algorithm/ChunkingPipeline.tcc` | `ProjectStore::load()`, `ProjectStore::save(project)`, `ProjectStore::load_meta()` or one-shot helpers. | Tool/example migration before CRTP deletion. |
| Existing `loadScanProject`/`saveScanProject` utility functions | `src/liblvr2/util/ScanProjectUtils.cpp`, `src/tools/lvr2_reconstruct/Main.cpp`, `src/tools/lvr2_scanproject_parser/Main.cpp` | Keep CLI behavior by delegating utilities to `lvr2::io::scan::load_project` / `save_project` until callers can use the new API directly. | Utility bridge migration. |
| Tool-local `AddFeatures`/deprecated HDF5 builders | `src/tools/lvr2_hdf5_builder_2/Main.cpp`, `src/tools/lvr2_largescale_reconstruct_mpi/Main.cpp`, `src/liblvr2/io/scanio/LabelHDF5IO.cpp` | Replace with final scan/storage services or delete unreachable deprecated paths; do not expose feature composition. | Tool migration and deprecated wrapper deletion. |
| Historical `FeatureBuild<scanio::ScanProjectIO>` viewer state and dynamic casts | Removed `src/tools/lvr2_viewer/*` files | No migration inside the core repo. The bundled viewer was removed; do not spend storage migration effort on viewer internals. | Complete. |
| `meshio::DirectoryIO` / `meshio::HDF5IO` storage wrappers | `include/lvr2/io/meshio/*`, `src/tools/lvr2_reconstruct/Main.cpp`, `src/tools/lvr2_3dtiles/Main.cpp` | Route mesh assets through `lvr2::io::mesh` facade and any retained point-cloud/storage work through `lvr2::io::storage`; do not keep public `meshio`. | Unified I/O namespace and CRTP deletion. |

## Scan-project smoke commands

These are the current old-path smoke checks to run before deleting the CRTP path
and to compare against the new service path after implementation. Use a clean
build tree and the same dependency mode for before/after runs.

```bash
# Configure with examples and tests so scan-project examples can be built.
cmake --preset vcpkg-release \
  -DLVR2_BUILD_EXAMPLES=ON \
  -DLVR2_BUILD_TESTS=ON
cmake --build --preset build-vcpkg-release --target lvr2_examples_scanprojects_simple

# Directory and HDF5 smoke in one generated-fixture example.
rm -rf build/baseio-smoke && mkdir -p build/baseio-smoke
(cd build/baseio-smoke && ../../build-vcpkg-release/bin/lvr2_examples_scanprojects_simple)
test -d build/baseio-smoke/examples_sp_simple/dirio_data
test -f build/baseio-smoke/examples_sp_simple/hdf5io_data.h5
```

System-package builds can use the existing opt-out preset when the host provides
all required packages:

```bash
cmake --preset system-optout-release \
  -DLVR2_BUILD_EXAMPLES=ON \
  -DLVR2_BUILD_TESTS=ON
cmake --build --preset build-system-optout-release --target lvr2_examples_scanprojects_simple
```

After the service replacement lands, run the focused CTest coverage with these
names or equivalent labels:

```bash
ctest --test-dir build-vcpkg-release --output-on-failure -R 'storage.*project|storage_io_contract_guard|unified_io_namespace_guard'
ctest --test-dir build-vcpkg-release --output-on-failure -L 'io;scan;storage'
```

The required scenarios are:

1. Directory backend registers through `StorageRegistry` and saves/loads a
   generated minimal scan project.
2. HDF5 backend registers through the same `StorageRegistry` path and saves/loads
   the same generated minimal scan project.
3. Fake/in-memory test backend registers through the same registry and exercises
   the same `ProjectStore` path.

The new public usage is intentionally shorter than manual kernel/schema setup:

```cpp
auto opened = lvr2::io::scan::open_directory(
    path, lvr2::io::scan::Schema::raw_ply(), lvr2::io::storage::LoadMode::Eager);
if (opened) {
    opened->save(project);
    auto loaded = opened->load();
}
```

One-shot helpers use the same open path:

```cpp
auto loaded = lvr2::io::scan::load_project(
    path, lvr2::io::scan::LoadOptions::directory_raw_ply());
auto saved = lvr2::io::scan::save_project(
    h5_path, project, lvr2::io::scan::SaveOptions::hdf5());
```

## Performance baseline commands

Use the existing non-gating JSON recorder. Record old CRTP and new service runs
with the same compiler, dependency mode, build type, and fixture size.

```bash
# Old CRTP Directory+HDF5 generated-fixture baseline.
python3 tests/performance/record_baseline.py \
  --output build/performance-baselines/baseio-crtp-scanproject-simple.json \
  --label baseio-crtp-scanproject-simple -- \
  bash -lc 'rm -rf build/perf-baseio-old && mkdir -p build/perf-baseio-old && cd build/perf-baseio-old && ../../build-vcpkg-release/bin/lvr2_examples_scanprojects_simple'

# New StorageBackend/ProjectStore Directory+HDF5 generated-fixture baseline.
python3 tests/performance/record_baseline.py \
  --output build/performance-baselines/storage-projectstore-scanproject-simple.json \
  --label storage-projectstore-scanproject-simple -- \
  ctest --test-dir build-vcpkg-release --output-on-failure -R 'storage.*project'
```

For larger local data, wrap the existing CLI route and the new helper route with
matching inputs:

```bash
python3 tests/performance/record_baseline.py \
  --output build/performance-baselines/baseio-crtp-scanproject-parser.json \
  --label baseio-crtp-scanproject-parser -- \
  build-vcpkg-release/bin/lvr2_scanproject_parser \
    --inputSource /path/to/raw-scan-project \
    --inputSchema RAW \
    --outputSource build/perf-baseio-parser/out.h5 \
    --outputSchema HDF5 \
    --convert
```

Do not gate CI on these numbers until a later owner-approved performance
threshold exists; use them to block only obvious regressions found in slice
review.
