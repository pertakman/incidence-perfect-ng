# Beta Release Checklist

Use this checklist before delivering firmware to external beta testers.

## 1) Freeze Candidate

- [ ] No pending feature work in this build.
- [ ] Build is from a clean, committed git state.
- [ ] Firmware version is decided (`YYYY.M.X` or `YYYY.M.X-betaN`).

## 2) Build And Flash

- [ ] Build succeeds for `esp32s3` (`pio run -e esp32s3`).
- [ ] Upload succeeds on each beta unit (`pio run -e esp32s3 -t upload`).
- [ ] Splash screen shows expected branding + FW version overlay.

## 3) Functional Validation (Per Unit)

- [ ] `ZERO` applies and readings stabilize.
- [ ] `AXIS` cycles `BOTH -> ROLL -> PITCH`.
- [ ] `MODE` workflow works from touch, serial, and ACTION-button interactions.
- [ ] `ROTATE` toggles 180 degrees and persists over reboot.
- [ ] `ALIGN` 6-step capture flow completes (touch + ACTION-button capture).
- [ ] Freeze toggle works from touch readout and ACTION-button short press.
- [ ] Serial and on-screen workflows stay synchronized.

## 4) Persistence And Reboot

- [ ] Power-cycle test: settings persist as expected.
- [ ] USB disconnect/reconnect test: device recovers cleanly.
- [ ] Serial monitor reconnect behavior is understood/documented.

## 5) Documentation Pack

- [ ] `README.md` reflects actual behavior in shipped build.
- [ ] Tester handoff note is updated with FW version and known issues.
- [ ] Test session logged in `docs/testing/validation-session-template.md`.

## 6) Ship

- [ ] Create git tag for shipped beta build.
- [ ] Share firmware version + known issues + feedback template with tester.
