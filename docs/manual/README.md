# Incidence Perfect NG Manual

Start here:

- `user-manual.md` (end-user guide)

## Build PDF

Generate a stamped PDF (includes FW version + git revision):

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build_manual_pdf.ps1
```

Output:

- `docs/manual/dist/Incidence-Perfect-NG-Manual-<FW_VERSION>.pdf`

Notes:

- The pipeline uses local `pandoc` from `tools/` and headless Chrome/Edge for PDF rendering.
- PowerShell is required to run the build scripts.
- `scripts/ensure_pandoc.ps1` auto-downloads `pandoc` into `tools/pandoc/` if missing.
- A local Chrome or Edge install is required for `--headless` PDF export.
- Cover page is injected from `docs/manual/cover-template.html`.
- Print styling is controlled by `docs/manual/manual.css`.
- Splash cover image is embedded from generated `docs/manual/dist/cover.png`.
- `docs/manual/dist/` is build output and stays ignored in git.

## Publish A Tracked Release PDF

Use this to copy the generated PDF into the tracked release folder:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build_manual_pdf.ps1 -PublishRelease
```

Output:

- `docs/manual/releases/Incidence-Perfect-NG-Manual-<FW_VERSION>.pdf`
