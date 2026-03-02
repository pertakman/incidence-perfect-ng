# Manual Releases

This folder contains curated, tracked manual PDFs for official release points.

Policy:

- `docs/manual/dist/` is ignored and used for local/generated build artifacts.
- `docs/manual/releases/` is tracked for the current published manual PDF.
- Older release PDFs should be attached to GitHub Releases (or external artifact storage), not kept in-tree.

Generate and publish in one step:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build_manual_pdf.ps1 -PublishRelease
```
