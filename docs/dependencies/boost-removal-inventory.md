# Boost retirement inventory

Status: complete for active LVR-owned build, package, and implementation surfaces.

The ROS 2 Lyrical / C++20 pivot removes Boost as an LVR dependency. Active public headers, implementation sources, CMake/package exports, vcpkg metadata, ROS/package metadata, Debian packaging, and CI smoke surfaces no longer require Boost.

## Final snapshot

Regenerate from the repository root with:

```bash
rg -n --glob '!docs/**' --glob '!tests/boost_retirement_inventory.cmake' --glob '!tests/boost_stdlib_replacements.cmake' --glob '!tests/no_boost_cli_parser_dependency.cmake' --glob '!tests/no_boost_dependency.cmake' '#\s*include\s*[<"]boost/|\bboost::|\bBOOST_|find_(package|dependency)\s*\(\s*Boost|Boost_(COMPONENTS|LIBRARIES|INCLUDE_DIRS|LIBRARY_DIR|LOG_LIBRARY|DIAGNOSTIC_DEFINITIONS)|boost-[A-Za-z0-9.+-]+|libboost-[A-Za-z0-9.+-]+|<depend>boost</depend>' include src examples tests cmake package.xml vcpkg.json CMakePresets.json Singularity.def debian .github .gitlab-ci.yml
```

Expected result: no matches outside historical docs and Boost-specific policy guard names.

## Standard vocabulary replacements retained

Earlier cleanup replaced standard-equivalent families with C++20 vocabulary: `std::filesystem`, `std::optional`, `std::variant`, `std::shared_ptr<T[]>`, `std::span`, and `std::chrono`.

## Removed hard cases

| Former Boost surface | Replacement |
|---|---|
| `boost::iostreams::mapped_file` in BigGrid and scan-project PLY export scratch buffers | `lvr2::util::MappedFile`, a small C++20/POSIX file-backed byte-span scratch buffer |
| Unused Boost archive includes in `BigVolumen` | Removed; remaining BigGrid persistence is explicit binary value reads/writes |
| Boost property-tree XML parsing in the Riegl project converter | Tool-local XML parser for the specific RiSCAN project fields consumed by the converter |
| Boost.MPI package path | Removed; no active `boost/mpi` or `boost::mpi` code exists |
| Boost.DateTime / Boost.Log link remnants | Removed from reconstruction tool CMake links after std-format spdlog migration |
| Generic Boost package/export plumbing | Removed from CMake dependency discovery, installed config, vcpkg, package.xml, CPack, Debian control, and CI package installs |

## Guards

- `lvr2_no_boost_dependency` bans active Boost include, namespace, package, CMake, and CI dependency tokens.
- `lvr2_boost_stdlib_replacements` remains as a targeted regression guard for standard-equivalent Boost families.
- `lvr2_no_boost_cli_parser_dependency` remains as a targeted CLI parser guard.
- `lvr2_boost_retirement_inventory` keeps this final inventory present and checks the completed hard-case replacement summary.

## Historical notes

Earlier Boost retirement work removed standard-equivalent Boost usage and Boost.Program_options first. This final cleanup removed the remaining hard cases and package/export dependency surfaces. Future Boost reintroduction requires a new ADR-approved exception.
