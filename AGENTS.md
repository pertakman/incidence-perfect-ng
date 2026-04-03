# AGENTS.md

Repo-specific guidance for Codex and other coding agents working in this project.

## Project Overview

Incidence Perfect NG is an ESP32-S3 based 2-axis incidence meter with:

- touch UI on a `240x536` AMOLED display
- web UI for remote control, setup helpers, and OTA
- PlatformIO-based firmware build
- markdown manual + generated PDF release artifacts

Primary source folders:

- `src/`: firmware source
- `test/`: native regression tests
- `docs/`: manual, release, testing, architecture, and legal docs
- `scripts/`: build/version/manual helper scripts

## Build And Test

Use these as the canonical checks unless the task clearly does not require them.

- Firmware build:
  - `platformio run -e esp32s3`
- Native regression tests:
  - `platformio test -e native`
- Serial monitor:
  - `platformio device monitor -b 115200`

If you change firmware behavior, prefer running the firmware build at minimum.
If you change `src/remote_protocol_utils.cpp` or related parsing/version logic, also run the native tests.

## Local Dependency Note

`QMI8658` is not fetched from the public registry. It is resolved by `scripts/configure_qmi8658_lib.py`.

Search order:

1. `QMI8658_LIB_PATH`
2. `lib/QMI8658`
3. `C:/dev/Arduino/Libraries/QMI8658`
4. `~/Documents/Arduino/libraries/QMI8658`

Do not reintroduce a hardcoded `file://...` library dependency in `platformio.ini`.

## Generated And Curated Artifacts

Treat these paths carefully:

- `docs/manual/dist/`
  - local generated manual build output
  - ignored in git
- `docs/manual/releases/`
  - curated release PDFs intended to be tracked/shared
- `.pio/`
  - build output; do not commit firmware artifacts

The firmware `.bin` should remain out of git. This repo tracks release documentation and manual PDFs, not compiled firmware deliverables.

## Manual And PDF Workflow

Manual source:

- `docs/manual/user-manual.md`

Build local PDF:

- `powershell -ExecutionPolicy Bypass -File .\scripts\build_manual_pdf.ps1`

Publish tracked release PDF:

- `powershell -ExecutionPolicy Bypass -File .\scripts\build_manual_pdf.ps1 -PublishRelease`

If manual content changes, update the markdown source first. Only generate/publish a PDF when the task actually calls for refreshed release documentation.

## Release Workflow

Use these docs as the release source of truth:

- `docs/release/release-candidate-workflow.md`
- `docs/release/beta-checklist.md`
- `docs/testing/validation-session-template.md`
- `docs/testing/hardware-validation-checklist.md`

When preparing a release candidate, keep these aligned:

- firmware version / tag
- release notes
- tester handoff note
- manual source
- published manual PDF

## Editing Expectations

- Prefer small, behavior-preserving refactors over broad churn.
- Preserve existing UI/workflow behavior unless the task explicitly changes it.
- Keep browser-only preferences separate from persistent device settings unless there is a strong product reason to merge them.
- Keep generated/manual-dist files out of normal code edits.
- Prefer updating docs when product behavior or workflow meaningfully changes.

## File Link Formatting

When referencing files in responses, use explicit markdown links in this format:

- `[file.ts](/absolute/path/file.ts:123)`

Rules:

- Prefer absolute filesystem paths.
- On this Windows repo, use a leading slash before the drive letter, for example:
  - `[README.md](/c:/dev/IncidencePerfectNG/README.md:1)`
- Include a line number when it is useful and known.
- Do not use inline code formatting instead of a clickable markdown link for file references.

## Current Architecture Hotspots

Be aware of these maintenance-sensitive areas:

- `src/remote_control.cpp` and the split remote-control modules
  - page: `src/remote_control_page.cpp`
  - http/api: `src/remote_control_http.cpp`
  - network: `src/remote_control_network.cpp`
  - ota: `src/remote_control_ota.cpp`
  - config: `src/remote_control_config.cpp`
- `src/ui_lvgl.cpp`
  - touch UI behavior and screen rendering
- `src/inclinometer.cpp`
  - core device behavior, persistence, and workflows

Try to keep responsibilities separated when extending these areas.

## Recommended Validation After Changes

Choose the smallest sensible validation set:

- Docs-only changes:
  - no build required, but keep references and filenames consistent
- Web UI / API changes:
  - `platformio run -e esp32s3`
- Remote parsing / version logic changes:
  - `platformio run -e esp32s3`
  - `platformio test -e native`
- Release/manual workflow changes:
  - sanity-check referenced scripts and output paths

## Backlog Awareness

For current priorities and open product questions, check:

- `README.md` -> `Active Backlog`

Do not treat old roadmap items as active work if they are already marked done.
