# Remote Control Plan (Phone + Web UI)

## Goal

Control and read the inclinometer from a smartphone without requiring an app-store app.

Primary approach:
- Device-hosted web UI (PWA-friendly), reachable from phone browser.
- Two network modes:
  - `AP mode` (peer-to-peer, no router required)
  - `STA mode` (join local Wi-Fi, access via hostname)

This matches your requirement to avoid app-store dependency and still support LAN access.

## What We Already Verified

- Current firmware has no Wi-Fi feature yet.
- Board vendor package includes Wi-Fi demos for both Arduino and ESP-IDF:
  - `Arduino/examples/WIFI_AP/WIFI_AP.ino`
  - `Arduino/examples/WIFI_STA/WIFI_STA.ino`
  - `Arduino/examples/WIFI_STA/espwifi.cpp`
- The hardware manual/index also contains `WIFI_STA` and `WIFI_AP` sections.

Reference source bundles:
- `docs/hardware/20241201211113ESP32-S3-AMOLED-1.91.rar`
- extracted paths inside vendor demo zip:
  - `ESP32-S3-AMOLED-1.91-Demo/Arduino/examples/WIFI_AP/WIFI_AP.ino`
  - `ESP32-S3-AMOLED-1.91-Demo/Arduino/examples/WIFI_STA/WIFI_STA.ino`

## Recommended Architecture

## 1) Network Manager

- New module: `src/remote/network_manager.{h,cpp}`
- Responsibilities:
  - Start AP mode (`WiFi.softAP`) for direct phone connection.
  - Start STA mode (`WiFi.begin`) using saved credentials.
  - Fallback rule:
    - If STA fails in N seconds, bring up AP mode automatically.
  - Optional AP+STA combined mode for onboarding.

## 2) Web Server + API

- New module: `src/remote/web_api.{h,cpp}`
- Use Arduino-native HTTP server (`WebServer.h`) first for simplicity.
- Endpoints:
  - `GET /api/state` -> JSON telemetry/state
  - `POST /api/cmd` -> command dispatch (`zero`, `axis`, `mode`, `align`, `rotate`, `freeze`, `confirm`, `cancel`, `capture`)
  - `GET /` -> mobile UI page (embedded HTML/CSS/JS)
  - `GET /health` -> basic status

For near-realtime updates:
- Phase 1: polling (5-10 Hz)
- Phase 2: upgrade to SSE or WebSocket if needed

## 3) Shared Command Router (Critical)

- New module: `src/control_router.{h,cpp}`
- Purpose: one command path for touch, serial, ACTION button, and web.
- Web should not bypass existing logic.

This avoids behavior drift and keeps workflows synchronized.

## 4) Persistence + Hostname

- Store Wi-Fi settings in `Preferences` (NVS), not EEPROM blocks already used by your calibration/settings.
- mDNS hostname in STA mode:
  - `incidence-perfect-ng.local`
- AP defaults:
  - SSID: `IncidencePerfectNG-<chipid4>`
  - WPA2 password (configurable)

## 5) Security Baseline

- Require a simple API token for command endpoints in STA mode.
- In AP mode, still keep WPA2 password.
- Read-only state can stay unauthenticated initially if desired.

## UX Proposal (Phone)

Main screen in browser:
- Large roll/pitch cards
- Status strip: orientation, axis mode, rotate, live/frozen
- Buttons mirroring device:
  - `ZERO`, `AXIS`, `MODE`, `ALIGN`, `ROTATE`, `FREEZE`
- Guided modals reused from current workflow semantics:
  - MODE confirm/cancel + hold-still timer
  - ALIGN step instruction + capture/cancel

No app-store app required.
Optional: Add-to-Home-Screen PWA shortcut.

## Phased Delivery Plan

## Phase A: Foundation (1-2 sessions)

- Add `network_manager` with AP mode only.
- Add minimal web server:
  - `GET /api/state`
  - `POST /api/cmd` for `zero`, `axis`, `freeze`
- Create minimal mobile web page.
- Add docs section: connect phone to AP and open device URL.

Acceptance:
- Phone can connect directly and read roll/pitch live.
- At least 3 commands work remotely.

## Phase B: Full Control + Workflow Sync (1-2 sessions)

- Route all remote actions through shared command router.
- Add MODE and ALIGN actions (`confirm/cancel/capture`).
- Ensure serial/touch/ACTION-button/web stay synchronized.

Acceptance:
- Toggling from any interface is reflected everywhere.
- Guided workflows behave identically across channels.

## Phase C: STA + Hostname + Onboarding (1-2 sessions)

- Add STA credential storage (NVS).
- Add onboarding page for SSID/password.
- Add mDNS hostname support (`incidence-perfect-ng.local`).
- Add AP fallback if STA fails.

Acceptance:
- Device joins home LAN and is reachable by hostname.
- Recovery path exists via AP fallback/reset.

## Phase D: Hardening

- Add timeout/retry logic and clear error states.
- Add beta tests for network flows.
- Update manual and tester handoff checklist.

## Test Matrix Additions

Add to `docs/testing/hardware-validation-checklist.md`:
- AP discovery/connect from iOS + Android
- `/api/state` update rate and stability
- Command latency under motion
- MODE/ALIGN workflow parity from web
- STA connect + mDNS resolution
- AP fallback when STA unavailable

## Why This Is The Right First Solution

- No Apple/Google store friction.
- Works on both iPhone and Android via browser.
- Low implementation risk because it reuses existing firmware workflows.
- Scales later to native app if ever needed.
