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
- Splash cover image is embedded from the generated `dist/cover.png`.
