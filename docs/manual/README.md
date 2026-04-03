# Incidence Perfect NG Manual

Start here:

- `user-manual.md` (end-user guide)

Manual artifact layout:

- `docs/manual/user-manual.md`: editable manual source
- `docs/manual/dist/`: local generated build output (`.md`, `.html`, `.pdf`, cover assets); ignored in git
- `docs/manual/releases/`: curated release PDFs intended for sharing and release publishing

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
- Use this for working drafts and validation copies, not for release distribution.

## Publish A Tracked Release PDF

Use this to copy the generated PDF into the tracked release folder:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build_manual_pdf.ps1 -PublishRelease
```

Output:

- `docs/manual/releases/Incidence-Perfect-NG-Manual-<FW_VERSION>.pdf`

Use this when preparing a release candidate or publishable build so the generated PDF lands in the tracked release-artifact folder.
