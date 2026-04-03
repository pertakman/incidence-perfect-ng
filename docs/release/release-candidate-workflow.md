# Release Candidate Workflow

Use this flow when preparing a new RC so build, validation, docs, and release artifacts stay in sync.

## 1) Freeze The Candidate

1. Confirm scope is frozen for this RC.
2. Make sure the working tree is clean.
3. Decide the version string:
   - firmware version: `YYYY.M.X`
   - optional RC label/tag: `vYYYY.M.X-rcN`
4. Write down what this RC is trying to validate.

## 2) Build And Smoke Check

1. Firmware build:

```powershell
pio run -e esp32s3
```

2. Host-side regressions when applicable:

```powershell
pio test -e native
```

3. Upload to the target board/unit:

```powershell
pio run -e esp32s3 -t upload
```

4. Confirm splash version/branding on the device.

## 3) Run Validation

1. Copy the session template:
   - `docs/testing/validation-session-template.md`
   - -> `docs/testing/session-logs/YYYY-MM-DD-validation-session.md`
2. Run the relevant parts of:
   - `docs/testing/hardware-validation-checklist.md`
   - any feature-specific checks needed for this RC
3. Record:
   - what was executed
   - what was skipped
   - failures/anomalies
   - sign-off recommendation

## 4) Update Documentation

Before shipping the RC, make sure these are aligned with the actual firmware behavior:

- `README.md`
- `docs/manual/user-manual.md`
- `docs/release/tester-handoff-note.md`
- release note under `docs/release/releases/` if creating one

If the manual changed, regenerate the PDF:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build_manual_pdf.ps1
```

If the RC should publish a tracked manual PDF:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build_manual_pdf.ps1 -PublishRelease
```

## 5) Package The RC

1. Commit the final RC state.
2. Create the release tag.
3. If applicable, create/publish the GitHub Release entry.
4. Share only approved artifacts.

Examples:

- approved:
  - manual PDF
  - release notes
  - tester handoff note
- not approved unless explicitly intended:
  - private build artifacts
  - local `.pio` outputs

## 6) Final Sanity Check

Before sending the RC out, verify:

- version on splash matches the intended candidate
- tester handoff note names the right RC/version
- manual PDF exists and matches the documented feature set
- validation session log exists for the candidate
- known limitations/experimental features are called out explicitly

## Related Documents

- `docs/release/beta-checklist.md`
- `docs/testing/hardware-validation-checklist.md`
- `docs/testing/validation-session-template.md`
- `docs/manual/README.md`
