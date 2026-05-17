# Boost retirement inventory

Date: 2026-05-17

## Purpose

This inventory tracks the active Boost surface during the C++20 retirement work. The target end state is no Boost dependency in public headers, implementation code, CMake exports, vcpkg manifests, ROS/Debian metadata, or CI package installation lists unless a future ADR accepts a narrow exception.

## Current status after standard-library replacements

The standard-equivalent Boost families have been replaced in active LVR-owned code. Remaining Boost usage is intentionally limited to hard cases assigned to follow-up cleanup:

- `boost::iostreams::mapped_file` and related streams/code-converter use in large-grid/storage utilities;
- Boost archive serialization in `BigVolumen`;
- Boost property-tree XML parsing in the Riegl project converter;
- Boost.MPI / DateTime package/link remnants that still need dependency isolation or deletion;
- generic Boost package/export plumbing needed only while the hard cases remain.

Scanned active roots: `include`, `src`, `examples`, `tests`, `cmake`, `package.xml`, `vcpkg.json`, `CMakePresets.json`, `debian`, `.github`, and `.gitlab-ci.yml`. Historical Markdown migration notes and guard scripts are excluded from semantic counts.

- Active files with Boost tokens: 21
- Public headers with active Boost tokens: 3
- `#include <boost/...>` directives: 10 across 7 distinct Boost headers
- Active `boost::...` references: 14 across 3 distinct namespace names
- Active `BOOST_*` macro references: 0
- Build/package/CI Boost references: 41

## Replacement families completed in this slice

| Family | Replacement | Status |
|---|---|---|
| filesystem | `std::filesystem` | complete in active code; `boost-filesystem` package metadata removed |
| optional | `std::optional` | complete; reference optionals use `std::optional<std::reference_wrapper<T>>` |
| variant / visitor | `std::variant` + `std::visit` | complete for LVR `Variant` and `VariantChannel` wrappers |
| shared_array | `std::shared_ptr<T[]>` owner + `std::span` view | complete for current array-owner APIs; prefer vector/span in new APIs |
| shared_ptr | `std::shared_ptr` / `std::make_shared` | complete |
| thread / mutex | `std::thread`, `std::jthread`, `std::mutex`, `std::scoped_lock` | mutex usage replaced; no active Boost thread code remains |
| timer | `std::chrono` | complete for `NodeData` timers; `boost-timer` package metadata removed |
| algorithm / range helpers | standard/ranges helpers | complete for active string/range helpers |
| type_index / typeinfo | `std::type_index` or explicit type vocabulary | complete |
| format | `std::format` or logging facade | complete |
| foreach | range-for | no active use remains |
| boost.system | `std::error_code` / filesystem errors | complete in active code |
| program_options | LVR-owned typed CLI parser | complete in earlier CLI parser cleanup |

## Remaining hard-case owner map

| Hard case | Current locations | Required decision before deletion |
|---|---|---|
| `boost::iostreams::mapped_file` | `include/lvr2/reconstruction/BigGrid.hpp`, `include/lvr2/reconstruction/BigGrid.tcc`, `include/lvr2/reconstruction/BigVolumen.hpp`, `src/liblvr2/util/ScanProjectUtils.cpp`, `src/tools/lvr2_hdf5_convert_old/Main.cpp` | Choose an internal byte-span mmap adapter or simpler buffered file I/O; preserve large-grid behavior with smoke tests. |
| Boost archive serialization | `include/lvr2/reconstruction/BigVolumen.hpp` and package metadata | Replace with explicit versioned binary records or delete unreachable serialization path. |
| Boost property-tree XML parsing | `src/tools/lvr2_riegl_project_converter/RieglProject.hpp` | Replace with explicit XML parsing or an approved package-backed parser without changing converter behavior. |
| Boost.MPI packaging | `cmake/Lvr3Dependencies.cmake`, `vcpkg.json`, `debian/control` | No active `boost/mpi` or `boost::mpi` code was found; either remove the package path or isolate an optional MPI path if follow-up review finds one. |
| DateTime / Boost.Log link remnants | `src/tools/lvr2_dmc_reconstruction`, `src/tools/lvr2_fastsense_reconstruction`, `src/tools/lvr2_gs_reconstruction` CMake files plus `boost-date-time` package metadata | Verify the tools no longer need Boost.Log/DateTime after std-format spdlog migration; remove direct link variables. |
| Generic Boost package/export plumbing | `cmake/Lvr3Dependencies.cmake`, `cmake/lvr2-config.cmake.in`, `cmake/Lvr3Packaging.cmake`, `package.xml`, `debian/control`, `vcpkg.json`, CI release workflow | Remove once hard-case code/package owners are gone. |

## Public header leakage that remains

| Public header | Families |
|---|---|
| `include/lvr2/reconstruction/BigGrid.hpp` | mapped file |
| `include/lvr2/reconstruction/BigGrid.tcc` | mapped file params |
| `include/lvr2/reconstruction/BigVolumen.hpp` | mapped file; Boost archive serialization |

No public header retains Boost filesystem, optional, variant, shared-array, shared-pointer, thread/mutex, timer, algorithm/range-helper, format, type-index, or Boost.System leakage.

## Active guards

- `lvr2_boost_retirement_inventory` keeps this inventory present and checks required owner categories.
- `lvr2_boost_stdlib_replacements` bans reintroduction of standard-equivalent Boost code/package metadata and catches common migration hazards such as `std::optional<T&>` or stale Boost-optional `.get()` call sites.
- `lvr2_no_boost_cli_parser_dependency` keeps `boost::program_options` removed.

## Regeneration commands

Raw active-token inventory:

```bash
rg -n --glob '!docs/**' --glob '!tests/boost_retirement_inventory.cmake' --glob '!tests/boost_stdlib_replacements.cmake' --glob '!tests/no_boost_cli_parser_dependency.cmake' '#\s*include\s*[<"]boost/|\bboost::|\bBoost\b|\bBOOST_|boost-[A-Za-z0-9.+-]+|libboost-[A-Za-z0-9.+-]+|Boost_[A-Za-z0-9_]+|<depend>boost</depend>|libboost-all-dev' include src examples tests cmake package.xml vcpkg.json CMakePresets.json debian .github .gitlab-ci.yml
```

Standard-equivalent guard mirror:

```bash
python3 - <<'PY'
from pathlib import Path
import re
banned = re.compile(r'#\s*include\s*[<"]boost/(filesystem|optional|shared_array|shared_ptr|smart_ptr|variant|type_index|core/typeinfo|format|foreach|thread|timer|system|algorithm)|boost::(filesystem|optional|none|shared_array|shared_ptr|make_shared|static_pointer_cast|variant|get|apply_visitor|static_visitor|typeindex|format|thread|mutex|timer|system|algorithm|split|is_any_of|to_upper_copy|core)|BOOST_(FOREACH|CORE)|make_shared_array|std::optional<[^>;]+&|getAttribute<[^;\r\n]+\.get\(')
for root in ['include', 'src', 'examples', 'tests']:
    for path in Path(root).rglob('*'):
        if path.is_file() and path.name not in {'boost_retirement_inventory.cmake', 'boost_stdlib_replacements.cmake', 'no_boost_cli_parser_dependency.cmake'}:
            if banned.search(path.read_text(errors='ignore')):
                raise SystemExit(f'Banned standard-equivalent Boost token in {path}')
print('standard-equivalent Boost guard passed')
PY
```
