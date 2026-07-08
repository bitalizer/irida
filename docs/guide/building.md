# Building Irida

## Prerequisites
- Visual Studio 2022 (Desktop development with C++ workload) — provides MSVC, CMake, Ninja.
- Qt 6 (MSVC 2022 64-bit) — install via the Qt Online Installer to `C:\Qt`.

## Configure & build (x64 Native Tools Command Prompt for VS 2022)
```bat
cmake --preset gui-debug
cmake --build --preset gui-debug
```
Run the app:
```bat
build\gui-debug\frontends\gui\irida_gui.exe
```

## Headless (no Qt) build
```bat
cmake --preset headless-debug
cmake --build --preset headless-debug
```

## Tests
```bat
ctest --preset gui-debug --output-on-failure
```

## In Visual Studio
File → Open → Folder → select the repo. Pick the `gui-debug` or `headless-debug`
preset in the configuration dropdown; Build All.

## Layering guarantee
The headless preset (`-DIRIDA_GUI=OFF`) builds core/libs/capi/mock with no Qt
dependency — the `irida_gui` target is not configured and Qt is never searched
for. If a non-GUI module ever pulls in Qt, this configuration fails. That is
the enforcement, not a convention.
