# Local CMake finder exceptions

This directory contains only audited project-local `Find*.cmake` fallbacks that remain after the CMake file-layout split. Prefer config packages, vcpkg metadata, standard CMake modules, and pkg-config imported targets before adding anything here.

Current exceptions are limited to FLANN, LZ4, and optional proprietary RDB discovery. See `../../docs/cmake/module-audit.md` for the required audit table and removal conditions.
