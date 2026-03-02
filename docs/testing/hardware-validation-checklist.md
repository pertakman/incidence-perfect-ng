# Hardware Validation Checklist

Use this checklist against the current firmware build to establish a known-good baseline.

## Preconditions

- Firmware uploaded from PlatformIO (`env: esp32s3`).
- Board powered over USB and responsive.
- Serial Monitor open at `115200`.
- Device resting on a stable, mostly level surface.
- For each run, create a dated protocol: `docs/testing/session-logs/YYYY-MM-DD-validation-session.md` (from `docs/testing/validation-session-template.md`).

## Known Current UI Limitations

- Battery no-pack detection (`BAT?`) is heuristic on this board and may not trigger in all USB-only scenarios.

## Test Cases

| ID | Action | Expected Result | Pass/Fail | Notes |
|---|---|---|---|---|
| T01 | Power cycle board | UI boots, shows roll/pitch readout, touch responsive |  |  |
| T02 | Observe serial startup | Mode banner prints with orientation and sign conventions |  |  |
| T03 | Tilt right edge down slowly | `ROLL` moves positive, smooth update, color warning after ~30 deg |  |  |
| T04 | Tilt left edge down slowly | `ROLL` moves negative |  |  |
| T05 | Tilt top toward user slowly | `PITCH` moves positive |  |  |
| T06 | Tap `ZERO` once on stable surface | Enters guided ZERO (`ZERO`->`CANCEL`, `MODE`->`CONFIRM`) with hint/progress support |  |  |
| T07 | In guided ZERO press `CONFIRM` and hold still | ZERO completes; values recenter near `+0.00 deg` |  |  |
| T08 | In guided ZERO press `CANCEL` | Returns to normal mode without applying new zero |  |  |
| T09 | Long-press `ZERO` in normal mode | Enters guided OFFSET CAL (`ZERO`->`CANCEL`, `MODE`->`CONFIRM`) |  |  |
| T10 | In guided OFFSET CAL press `CONFIRM` and hold still | OFFSET CAL completes with progress bar; readings settle |  |  |
| T11 | Tap `AXIS` repeatedly | Layout cycles: both -> roll-only -> pitch-only -> both |  |  |
| T12 | Tap `ALIGN`, then `CANCEL` | Enters alignment modal, then returns to normal UI |  |  |
| T13 | Tap `ALIGN`, capture 1-2 steps, then `CANCEL` | Step text updates clearly and flow can be aborted safely |  |  |
| T14 | Tap `MODE` once | Orientation toggles immediately (`SCREEN UP` <-> `SCREEN VERTICAL`) |  |  |
| T15 | In serial monitor send `z` | Starts guided ZERO workflow |  |  |
| T16 | In serial monitor send `c` while ZERO pending | Confirms ZERO workflow and starts hold/sampling |  |  |
| T17 | In serial monitor send `o` | Starts guided OFFSET CAL workflow |  |  |
| T18 | In serial monitor send `c` while OFFSET CAL pending | Confirms OFFSET CAL workflow and starts hold/sampling |  |  |
| T19 | In serial monitor send `u` | Orientation switches immediately to `SCREEN UP` |  |  |
| T20 | In serial monitor send `v` | Orientation switches immediately to `SCREEN VERTICAL` |  |  |
| T21 | In serial monitor send `m` | Orientation toggles immediately |  |  |
| T22 | In serial monitor send `x` during ZERO/OFFSET CAL/ALIGN | Active workflow is canceled |  |  |
| T23 | Tap `ROTATE` | Full UI rotates 180 deg and status `ROT` updates |  |  |
| T24 | In normal mode ACTION short press | Toggles freeze (`LIVE` <-> `FROZEN`) |  |  |
| T25 | In normal mode ACTION long press (~1.2s) | Triggers `AXIS` cycle |  |  |
| T26 | In normal mode ACTION very long press (~2.2s) | Toggles orientation mode |  |  |
| T27 | In normal mode ACTION ultra long press (~3.2s) | Starts guided OFFSET CAL workflow |  |  |
| T27b | In normal mode ACTION super long press (~5.0s) | Device enters deep sleep and wakes on ACTION press/reboot |  |  |
| T28 | In guided ZERO use ACTION short press | Acts as `CONFIRM` |  |  |
| T29 | In guided ZERO use ACTION long press | Acts as `CANCEL` |  |  |
| T30 | In guided OFFSET CAL use ACTION short press | Acts as `CONFIRM` |  |  |
| T31 | In guided OFFSET CAL use ACTION long press | Acts as `CANCEL` |  |  |
| T32 | Web UI press `ZERO` | Starts guided ZERO on device; web shows `CONFIRM/CANCEL` and progress after confirm |  |  |
| T33 | Web UI press `OFFSET CAL` | Starts guided OFFSET CAL on device; web shows `CONFIRM/CANCEL` and progress after confirm |  |  |
| T34 | Tap readout area | Freeze toggles immediately; displayed values hold instantly (no lag) |  |  |
| T35 | In serial monitor send `d` twice | Raw IMU stream toggles ON then OFF |  |  |
| T36 | Power cycle after orientation/rotation/alignment/offset cal/zero changes | Persisted settings restore correctly from EEPROM for current mode |  |  |
| T37 | Send `h` in serial monitor | Help prints and live stream pauses |  |  |
| T38 | After T37 press `Enter` (or send `g`) | Live stream resumes (`Roll/Pitch` lines continue) |  |  |
| T39 | Open web diagnostics panel | Raw/remapped/corrected values and refs update live without UI disconnect |  |  |
| T40 | Open web `Network` panel, set STA credentials + custom hostname + save | Device attempts STA while keeping AP fallback |  |  |
| T41 | With valid LAN credentials, wait for connect | Device gets STA IP and is reachable via `http://<hostname>.local` |  |  |
| T42 | Power cycle after T41 | Device reconnects to saved STA network and hostname persists |  |  |
| T43 | Upload firmware `.bin` via web `OTA Update` panel | Upload completes; device reboots and comes back with expected FW |  |  |
| T44 | Force STA failure (wrong SSID or AP offline) | AP fallback remains available for recovery/config edits |  |  |
| T45 | Observe battery line in device UI and web UI | Shows battery percent + voltage (or `BAT --` if unavailable) |  |  |
| T46 | Connect/disconnect charger power while monitoring UI/web | Charging indication (`CHG`) appears when charging is detected and clears when charging stops |  |  |
| T47 | Disconnect battery pack while USB power remains connected | Best-effort: battery line may switch to `BAT?` after stabilization window; if it remains `BAT/CHG`, record as known limitation for this hardware and continue |  |  |
| T48 | From normal mode enter deep sleep via ACTION super long press, then wake via ACTION | Touch works after wake (UI taps/buttons respond without reboot) |  |  |
| T49 | Repeat T48 and observe boot UX | Splash screen appears after wake and firmware version overlay is visible |  |  |
| T50 | OTA upload with target version equal/older than running FW (`force` unchecked) | OTA is rejected by version gate; running firmware stays unchanged |  |  |
| T51 | OTA upload with intentionally wrong SHA-256 | OTA is rejected by checksum gate; running firmware stays unchanged |  |  |
| T52 | In web `Network` panel trigger `Recover AP Mode` | Device switches to AP-only, STA credentials are cleared, AP remains reachable |  |  |
| T53 | Reboot device, then immediately press+hold ACTION continuously through startup for ~2 s (do not hold before power-on) | Boot-time network recovery applies AP-only defaults and STA credentials are cleared |  |  |
| T54 | If two or more units are available, boot each in AP mode and compare SSIDs | Each unit reports a different `IncidencePerfectNG-XXXX` suffix (board-unique identity) |  |  |

## Optional Deep Check (Serial Alignment Flow)

1. Send `C` in serial monitor.
2. Follow prompts and press `c` at each requested orientation.
3. Confirm `Alignment complete` prints.
4. Verify return to normal operation immediately after the 6th capture.

Expected:

- No lockup during prompts.
- Alignment values persist after power cycle.

## Exit Criteria for Baseline Complete

- T01-T54 pass.
- Any failures are captured with reproducible steps and serial output snippets.
