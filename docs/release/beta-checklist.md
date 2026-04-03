# Beta Release Checklist

Use this checklist before delivering firmware to external beta testers.

## 1) Freeze Candidate

- [ ] No pending feature work in this build.
- [ ] Build is from a clean, committed git state.
- [ ] Firmware version is decided (`YYYY.M.X` or `YYYY.M.X-betaN`).
- [ ] Release scope is written down:
  - what changed
  - what is intentionally still experimental
  - what should be explicitly exercised by tester(s)

## 2) Build And Flash

- [ ] Build succeeds for `esp32s3` (`pio run -e esp32s3`).
- [ ] Host-side regression checks succeed if applicable (`pio test -e native`).
- [ ] Upload succeeds on each beta unit (`pio run -e esp32s3 -t upload`).
- [ ] Splash screen shows expected branding + FW version overlay.

## 3) Functional Validation (Per Unit)

- [ ] `ZERO` applies and readings stabilize.
- [ ] `Startup ZERO` behaves as configured on cold boot (`Enabled` / `Disabled`).
- [ ] `AXIS` cycles `BOTH -> ROLL -> PITCH`.
- [ ] `MODE` toggle works from touch, serial, web, and ACTION-button interactions.
- [ ] `ROTATE` toggles 180 degrees and persists over reboot.
- [ ] Boot splash follows stored rotation on both `ROT 0` and `ROT 180`.
- [ ] `ALIGN` 6-step capture flow completes (touch + ACTION-button capture).
- [ ] Freeze toggle works from touch readout and ACTION-button short press.
- [ ] Serial and on-screen workflows stay synchronized.
- [ ] Web live readout remains responsive during a realistic usage session (not just a quick smoke test).
- [ ] Web `Device Settings` save/load round-trip works:
  - battery mode
  - startup ZERO
  - readout decimals
  - touch lock / persistence
  - brightness
- [ ] Web `Network` save/recovery flow works.
- [ ] Web setup helpers are sanity-checked if they are part of the shipped scope:
  - `Surface Displacement`
  - `Linearity` (if included/advertised)

## 4) Persistence And Reboot

- [ ] Power-cycle test: settings persist as expected.
- [ ] USB disconnect/reconnect test: device recovers cleanly.
- [ ] Serial monitor reconnect behavior is understood/documented.
- [ ] Touch-lock persistence behavior is explicitly verified:
  - temporary lock clears after reboot when persistence is off
  - persistent lock survives reboot when persistence is on

## 5) Documentation Pack

- [ ] `README.md` reflects actual behavior in shipped build.
- [ ] `docs/manual/user-manual.md` reflects actual behavior in shipped build.
- [ ] Manual PDF regenerated (`docs/manual/dist/...pdf`).
- [ ] If publishing a tracked RC PDF, release copy is refreshed under `docs/manual/releases/`.
- [ ] Tester handoff note is updated with FW version and known issues.
- [ ] Test session logged in `docs/testing/session-logs/YYYY-MM-DD-validation-session.md` (using `docs/testing/validation-session-template.md`).

## 6) Ship

- [ ] Create git tag for shipped beta build.
- [ ] Create/publish GitHub Release if this RC is externally shared.
- [ ] Share firmware version + known issues + feedback template with tester.
- [ ] Attach/share only approved artifacts for this release (for example manual PDF, not private build outputs).
