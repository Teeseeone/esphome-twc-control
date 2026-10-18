# Norway 230 V IT / Tibber development

Status: telemetry prototype. Not a validated charging controller.

The Waveshare board has not arrived yet (18 October 2026). Hardware and
RS485 tests are pending; this stage is software preparation only.

## Hardware and goal

Waveshare ESP32-S3-RS485-CAN, Tesla Wall Connector Gen 3, 230 V IT,
3 x 63 A main supply, Tibber total power and three line currents at about
10-second intervals. The planned house power ceiling is 9,000 W.
The charger's configured branch/Home Load Management limits must be recorded
separately; do not substitute the 63 A house main rating for them.

The existing fork already contains the RS485 meter-emulation implementation.
This directory provides a standalone first-stage monitor for validating the
power inputs before adapting that controller. No RS485 configuration or commands
are included in `it-monitor.yaml`. Flashing it replaces any previous ESPHome
firmware on that board, including any existing RS485 functionality.

## Verified power discrepancy

On firmware 26.34.0+g1a5f79be65b2b4, session energy increased 215 Wh in
120 seconds: 6,450 W average. Tesla Fleet showed 6,449 W; Home Assistant's
local calculated power was about 7,440 W. Upstream issue:
https://github.com/einarhauks/tesla-wall-connector/issues/24

`it_energy_power.h` calculates delta-energy / delta-session-time, independent
of assumptions about neutral or voltage/current channel topology. It updates
the average after each window of at least 120 s. Startup/recovery requires a
new window; failed requests, counter resets, contactor transitions, and gaps
over 30 s invalidate an in-progress window. A contactor-open response reports
0 W. The sensor returns unavailable for stale data and rejects a session
counter that stops tracking elapsed real time.

This is a delayed measured average, not instantaneous power and not suitable
as the only fast feedback input for load limiting. It does not patch the
built-in Home Assistant integration or apply an unconditional sqrt(3)/2 factor.

## Entities

| Name | Purpose |
| --- | --- |
| Charging Power Energy Average 120s | Measured average from session energy |
| Raw Voltage Current Channel Sum | Original calculation for comparison only |
| Session Energy | Charger session counter, Wh |
| Tibber Total Power | Actual house total, including charging |
| Tibber L1 / L2 / L3 Current | Actual measured line currents |
| Wall Connector Telemetry Fresh | Valid vitals response within 30 s |
| Connected / Uptime / WiFi Signal | ESP32 diagnostics |

Tibber sensors are mirrored values: their presence alone does not prove a
fresh meter reading. A genuine upstream timestamp/heartbeat is needed before
using them to control charging. Unchanged numeric readings are not evidence
of a stopped meter, and connectivity alone does not prove freshness either.

## Configuration and checks

1. Keep the YAML and header together in `experimental/`.
2. Create `experimental/secrets.yaml` containing `wifi_ssid`, `wifi_password`,
   `api_encryption_key` (32 bytes, base64), `ota_password`, and
   `twc_vitals_ip`. Keep it out of Git. Root `secrets.yaml.example` shows the
   naming conventions. Do not copy its placeholder API key unchanged.
3. Verify the four Tibber entity substitutions in the YAML against HA.
4. Validate with `esphome config experimental/it-monitor.yaml` and compile
   with `esphome compile experimental/it-monitor.yaml` before any flash.
5. Enable permission for this ESPHome device to subscribe to Home Assistant
   states when prompted. Compare several stable 120-second sessions at
   different charging currents with Fleet and the charger's counters.

Host-side calculation regression test:

```sh
g++ -std=c++17 -Wall -Wextra -Werror experimental/test_it_energy_power.cpp -o /tmp/test-it-power
/tmp/test-it-power
```

## Next controller work

1. Replace Shelly per-phase power requirements with Tibber total measured W
   and measured line currents. Never invent per-phase power by dividing the
   total into thirds or multiplying an IT phase current by phase-to-neutral V.
2. Establish the relationship between Tesla's API channels, real line currents,
   and RS485 meter registers on this supply. Do not subtract Tesla channel A/B/C
   values directly from Tibber L1/L2/L3 until this mapping is validated.
3. Add user controls: house W ceiling (9,000 W initial project target), manual
   charging-current cap, and monitor/control mode. Keep control disabled at
   startup until all required data and configured limits have been validated.
4. Expose budget W, requested cap A, measured vehicle A, measured house W,
   line headroom A, meter age, RS485 poll age, and limiting/failure reason.
   Distinguish values requested from the charger from values measured at it.
5. Simulate load steps, startup, stale Tibber, HTTP/HA outages, reboot, and
   charger ignoring/lagging meter limits. Verify actual fail-safe behavior on
   firmware 26.34.0 before using the system for automatic control.

RS485 emulates the charger's external meter input; it is not a documented
direct 'set current' command. Firmware behavior described by the upstream
project for 26.26.1 cannot be assumed identical on this charger. Keep only one
active charging-current controller during validation to avoid conflicts with
the existing Tesla Fleet automation.

## Validation status

- Host C++ calculation tests: passed for the recorded 215 Wh / 120 s case,
  data gaps, session/reboot resets, invalid values, frozen counters and
  32-bit timer wrap.
- ESPHome 2026.9.1 configuration validation: passed with temporary test secrets.
- ESPHome C++ source generation: passed. Full ESP32 firmware build is not
  verified; framework/toolchain download is required before installation.
- Hardware/RS485 charging tests on this installation: not performed.
