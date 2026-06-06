# Repository Guidelines

## Project Structure & Module Organization

This repository is a GD32F470 embedded C firmware project configured for EIDE. `src/` owns the application entry point: keep `main.c` focused on high-level initialization and application flow. `Driver/bsp/` is the board-support layer; add board-level drivers and APIs here for `main.c` to call as hardware bring-up expands. `Protocol/` owns frame parsing, response framing, ASCII-hex conversion, and CRC helpers. `Function/` owns boot/update and application business logic; `Function/boot/` contains Bootloader support code and `Function/app/` contains application-side update logic. `startup/` contains startup and system files, including project configuration, interrupt vector management, clock/SysTick support, and other system-level parameters. MCU headers and peripheral drivers are under `drivers/`; treat `GD32F4xx_Firmware_Library_V3.3.3/` as upstream vendor reference material if it is added back. If hardware manuals, schematics, or contest documents are restored, keep them in `docs/`. Build products are generated in `build/` and should not be edited by hand.

## Build, Test, and Development Commands

Use VS Code with the recommended `cl.eide` extension from `CMIS.code-workspace`.

- `Tasks: Run Task > build`: compile the active EIDE target.
- `Tasks: Run Task > rebuild`: clean and compile the target from scratch.
- `Tasks: Run Task > clean`: remove generated build artifacts.
- `Tasks: Run Task > flash`: upload with the configured J-Link target.
- `Tasks: Run Task > build and flash`: compile, then program the board.

The App and Bootloader targets use ARMCC5, `GD32F470`, MicroLIB, Cortex-M4 single-precision FPU settings, and output to `build/App/` and `build/Bootloader/`.

## Coding Style & Naming Conventions

Follow `.clang-format`: Microsoft base style, 4-space indentation, spaces only, no column limit, Linux braces for C/C++, unsorted includes, and aligned consecutive macros/assignments. Keep C source and header pairs named by module, for example `bsp_led.c` and `bsp_led.h`. Use lowercase snake_case for files and functions where possible, and keep MCU/vendor symbols unchanged.

## Testing Guidelines

No automated unit-test framework is currently present. Validate changes by building the active `App` or `Bootloader` EIDE target and, for hardware-facing changes, flashing to the GD32F470 board and checking the affected peripheral behavior. Add focused test hooks or host-side tests only when introducing logic that can run independently of hardware.

## Commit & Pull Request Guidelines

No Git history is available in this checkout, so use concise imperative commit subjects such as `Add LED board support`. Pull requests should describe the target hardware, summarize firmware behavior changes, list build and flash verification, and include logs or screenshots when debugging output or hardware state is relevant.

## Agent-Specific Instructions

Avoid broad edits to vendor libraries, generated files, or documentation bundles unless explicitly requested. Prefer small, reviewable firmware changes in `src/`, `Driver/bsp/`, `Protocol/`, `Function/`, `startup/`, or a clearly named feature module.



## Project Memory

- Before broad codebase analysis, read `graphify-out/GRAPH_REPORT.md`.
- When locating cross-module relationships, query Graphify before doing broad text search.
- Use `rg` to verify exact file paths and implementation details after Graphify suggests likely modules.
- If code structure changes substantially, run `graphify . --update`.
```
