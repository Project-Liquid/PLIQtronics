# PLIQtronics Mega Example

This directory contains a system-specific Arduino Mega sketch for integrating
PLIQtronics sensors, valves, heaters, and telemetry. It is an example of the
firmware structure and command interface, not a generic wiring guide or a
safety-certified controller. Confirm every pin, electrical interface, sensor
calibration, and actuator behavior against the actual hardware before use.

## Files

- `mega.ino` — setup, main loop, telemetry, command parsing, scheduling, and
  redline polling.
- `main.h` — pin assignments, thresholds, timings, global declarations, and
  shared includes.
- `procedures.cpp` / `procedures.h` — automated procedures and redline
  conditions/responses.

The sketch uses the limit-switch `MBV` implementation from `mbv.h`. Build it
with the PLIQtronics library and its dependencies installed; select an Arduino
Mega-compatible board and compile before connecting actuators. The library
dependencies are listed in the parent [PLIQtronics README](../../README.md).

## I/O assignments

Pins below are the assignments in `main.h`. Verify them against the specific
board revision and wiring.

| Function | Arduino Mega pin(s) | Notes |
|---|---:|---|
| Ethane upstream pressure | A12 | Analog transducer |
| Ethane downstream pressure | A11 | Analog transducer |
| Nitrous upstream pressure | A10 | Analog transducer |
| Nitrous downstream pressure | A9 | Analog transducer |
| Reroute pressure | A8 | Analog transducer |
| Chamber pressure | A13 | Analog transducer |
| Ethane run valve | 47 | Relay output |
| Ethane vent valve | 46 | Relay output |
| Nitrous run valve | 36 | Relay output |
| Nitrous vent valve | 40 | Relay output |
| Ethane MBV motor | 1, 2, 3 | Constructor order: speed/PWM, direction 1, direction 2 |
| Ethane MBV limit switches | 44, 42 | Open, close |
| Nitrous MBV motor | 1, 2, 3 | Constructor order: speed/PWM, direction 1, direction 2 |
| Nitrous MBV limit switches | 38, 48 | Open, close |
| Ethane load cells 1/2/3 | 22/23, 24/25, 26/27 | Each pair is passed to a `LoadCell` |
| Nitrous load cells 1/2/3 | 34/35, 32/33, 30/31 | Each pair is passed to a `LoadCell` |
| Ethane heaters 1/2 | 43, 45 | Heater outputs |
| Nitrous heaters 1/2 | 49, 41 | Heater outputs |
| Ignitor | 39 | Relay output |
| Chamber thermocouple chip select | 53 | Defined but the active MCP9600 setup uses I2C addresses |
| Serial console | `Serial` | Hardline serial |
| Radio serial | `Serial2` | Mega hardware serial |
| Thermocouples | I2C | MCP9600 addresses `0x66`, `0x65`, `0x67` |
| Thrust cell | I2C | Constructed as `ThrustCell`; uses the library's Qwiic Scale driver |

### Pin-map and setup caveats

- Both MBV motor definitions currently use pins 1, 2, and 3. That assigns the
  same motor outputs to both valves, so confirm whether this is intentional
  before wiring or operating the system.
- Mega pin 1 is the `Serial` transmit pin, and this sketch also uses `Serial`.
  Avoid sharing it with an MBV motor driver; resolve the assignment conflict
  in the hardware map and firmware first.
- The thermocouple objects are created using I2C addresses; `CHAMBER_TC_PIN`
  and the ADS1118 setup shown in the sketch are not used by the active setup.
- Load-cell calibration factors, pressure ranges, and heater targets are
  project-specific values, not defaults suitable for other hardware.

## Serial interface

The sketch starts both `Serial` and `Serial2` at **57600 baud**. It accepts
newline-terminated commands from either port, trims whitespace, and converts
commands to uppercase. Commands are handled one at a time.

### Commands implemented

| Command | Action |
|---|---|
| `STATUS` | Emit a status report in the configured transmission format |
| `START` / `STOP` | Enable / disable telemetry printing |
| `ETHANE_VENT_ON` / `ETHANE_VENT_OFF` | Open / close ethane vent relay |
| `NITROUS_VENT_ON` / `NITROUS_VENT_OFF` | Open / close nitrous vent relay |
| `ETHANE_RUN_ON` / `ETHANE_RUN_OFF` | Open / close ethane run relay |
| `NITROUS_RUN_ON` / `NITROUS_RUN_OFF` | Open / close nitrous run relay |
| `ETHANE_MBV_OPEN` / `ETHANE_MBV_CLOSE` | Drive ethane MBV toward its open / close limit |
| `NITROUS_MBV_OPEN` / `NITROUS_MBV_CLOSE` | Drive nitrous MBV toward its open / close limit |
| `ETHANE_MBV_JOG_<speed>` | Jog ethane MBV; numeric speed is passed to the MBV driver |
| `NITROUS_MBV_JOG_<speed>` | Jog nitrous MBV; numeric speed is passed to the MBV driver |
| `ETHANE_MBV_BRAKE` / `ETHANE_MBV_COAST` | Brake / coast ethane MBV |
| `NITROUS_MBV_BRAKE` / `NITROUS_MBV_COAST` | Brake / coast nitrous MBV |
| `E_STOP` | Run the `EMERGENCY_VENT` procedure |
| `COLD_FLOW_ETHANE_<ms>` | Start scheduled ethane cold-flow sequence |
| `COLD_FLOW_NITROUS_<ms>` | Start scheduled nitrous cold-flow sequence |
| `STATIC_FIRE_<ms>` | Request the static-fire sequence with the given duration |
| `ETHANE_TARGET_PRESSURE_<value>` | Set the ethane target-pressure variable |
| `NITROUS_TARGET_PRESSURE_<value>` | Set the nitrous target-pressure variable |
| `ETHANE_LC_TARE` / `NITROUS_LC_TARE` | Tare all three load cells for that propellant |
| `THRUST_LC_TARE` | Tare the thrust cell |
| `ETHANE_H1_ON/OFF`, `ETHANE_H2_ON/OFF` | Control the ethane heater outputs |
| `NITROUS_H1_ON/OFF`, `NITROUS_H2_ON/OFF` | Control the nitrous heater outputs |
| `HEATERS_OFF` | Turn off all heaters and disable scheduled heater updates |

The `*_MBV_JOG_<speed>` parser takes the substring after the command prefix;
use an integer suffix such as `ETHANE_MBV_JOG_120`. Test jog direction and
speed only with the motor disconnected from hazardous equipment.

The target-pressure commands update global variables, but the sketch only
passes their values to heater `setTarget()` during setup. They do not update
the heater targets after setup in the current implementation.

## Automated procedures

Procedure delays are in milliseconds. Scheduled actions are serviced from the
main loop on a nominal 50 ms interval.

| Procedure | Current sketch behavior |
|---|---|
| Ethane cold flow | Opens ethane run relay immediately; schedules ethane MBV open after 1 s, then closes the MBV and run relay after the requested duration plus 1 s. |
| Nitrous cold flow | Opens nitrous run relay immediately; schedules nitrous MBV open after 1 s, then closes the MBV and run relay after the requested duration plus 1 s. |
| Static fire | Opens ethane run relay and nitrous MBV immediately; schedules ethane MBV open after 200 ms. Closes nitrous MBV/run relay after the requested duration plus 200 ms; closes ethane MBV/run relay after an additional 500 ms. Ignitor activation is scheduled 5 s after the procedure initializes. |
| Ethane / nitrous vent | Closes the corresponding run relay immediately; opens the vent after 1 s and closes it 3 s later. |
| Emergency vent (`E_STOP`) | Clears queued actions and neutralizes relays, MBVs, and heaters. It then conditionally closes MBVs reported open, closes the run relays, and schedules both vents to open after 1 s and close 3 s later. |
| `neutralizeAll()` | Clears/neutralizes valves and turns off all heaters. It does not itself perform venting. |

These are descriptions of the code paths, not guarantees about physical
actuation. In particular, verify valve direction, limit-switch behavior,
interlocks, and the intended safe state independently.

## Redlines

The sketch polls its redline objects nominally every 50 ms. Configured
pressure thresholds are **1100 PSIG** for both upstream pressure transducers;
the reroute thermocouple threshold is **550 °C**. Weight thresholds are
configured as **-27 lbs** for ethane underweight and **19.5 lbs** for nitrous
overweight. All redline count thresholds
are set to 20; `Redline::checkTrigger()` invokes a response only when its count
is greater than the configured threshold. Higher numeric priority values are
treated as higher priority by the redline class.

| Redline | Condition | Response in the sketch |
|---|---|---|
| Ethane / nitrous overpressure | Upstream pressure exceeds its threshold | Clears that propellant's run/vent schedules, opens its run relay, and schedules its vent open immediately and close after 3 s |
| Combustion propagation | During the steady-fire flag, selected pressure differences/ratios exceed the coded limits | Neutralizes all; closes both MBVs and run relays; opens both vents and schedules them to close after 3 s |
| Inline thermal decomposition | Reroute thermocouple exceeds 550 °C | Neutralizes all; closes nitrous MBV; schedules ethane MBV and run-relay close actions after 1 s |
| Ethane underweight | Ethane load-cell joint reading falls below -27 | Neutralizes/closes ethane run relay |
| Nitrous overweight | Nitrous load-cell joint reading exceeds 19.5 | Neutralizes/closes nitrous run relay |

## Telemetry

The default format is `COMPRESSED`; records are newline-terminated and start
with `DATA|<elapsed-seconds>`. Fields are emitted at configured polling
intervals, so one record may contain only a subset:

```text
DATA|12.345|PT_EU:250.125|PT_ED:248.900|PT_NU:300.500|PT_ND:299.750|PT_RR:280.000|PT_CH:275.250|LC_E1:12.3|LC_E2:12.4|LC_E3:12.2|LC_N1:18.1|LC_N2:18.0|LC_N3:18.2|LC_T:0.8|ERV:1|EV:0|NRV:1|NV:0|MBV_E:0|MBV_N:1|TC_C:24.5|TC_R:25.0|NH_1:0|NH_2:0|EH_1:1|EH_2:1
```

Illustrative values only; actual records may contain fewer fields depending on
which polling intervals are due.

- Pressure: `PT_EU`, `PT_ED`, `PT_NU`, `PT_ND`, `PT_RR`, `PT_CH`
- Load / thrust: `LC_E1`, `LC_E2`, `LC_E3`, `LC_N1`, `LC_N2`, `LC_N3`, `LC_T`
- Relay / MBV status: `ERV`, `EV`, `NRV`, `NV`, `MBV_E`, `MBV_N`
- Thermocouple: `TC_C`, `TC_R`
- Heater state: `NH_1`, `NH_2`, `EH_1`, `EH_2`
- Redline status, when reported: `RD_H`, `RD_L`

`DIAGNOSTIC` and `RAW` output formats are also present. Diagnostics produce
more serial output and can affect loop timing; use the actual implementation
and downstream logger to confirm field units and interpretation.

## Safety and validation

This example controls physical valves, heaters, and an ignitor. Do not treat
serial commands, software redlines, or `E_STOP` as substitutes for independent
hardware interlocks and approved operating procedures. Before connecting any
hazardous system:

1. Resolve the duplicate MBV motor pins and the pin 1 / serial conflict.
2. Verify sensor wiring, ranges, units, calibration, and thermocouple addresses.
3. Compile the exact sketch and confirm each redline response with simulated
   inputs or a non-operational test setup.
4. Test motor directions, limit switches, relay polarity, schedules, and
   emergency behavior with actuators isolated from hazardous equipment.
5. Have the system reviewed and approved by the responsible engineering and
   safety authorities.
