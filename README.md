# PLIQtronics

PLIQtronics is an Arduino library containing classes for pressure and temperature
sensors, load cells, relays, motorized ball valves, heaters, serial output, and
threshold-triggered callbacks. It was developed for Project Liquid's ground
support and vehicle electronics. The `examples/mega` sketch is a system-specific
integration example, not a generic wiring guide.

## Installation

Install the library using one of these methods:

- **Arduino IDE:** Add the PLIQtronics folder to your Arduino `libraries`
  directory, then restart the IDE.
- **Arduino CLI:** Copy the folder into your sketchbook's `libraries` directory.

Install the third-party libraries listed below using the Arduino IDE Library
Manager or the corresponding library's installation instructions.

### Dependencies

`library.properties` declares these dependencies:

- HX711 Arduino Library
- Encoder
- Adafruit MCP9600 Library
- SparkFun Qwiic Scale NAU7802 Arduino Library

The Encoder dependency is used by `mbvContinuous.h`; the other hardware-specific
dependencies are used by their corresponding component classes. Arduino core
libraries such as `Wire` are also used where required.

## Include the components you use

Include individual component headers in a sketch, for example:

```cpp
#include <transducer.h>
#include <relay.h>
```

The library contains two alternative `MBV` implementations:

- `mbv.h` is for a motor driver with direction outputs and open/close limit
  switches.
- `mbvContinuous.h` is for a PWM motor with an encoder.

Both declare a class named `MBV`. Choose one implementation for a sketch; do not
include both headers in the same program.

## Quick start

This example reads a pressure transducer. Set the analog pin and pressure range
to match the sensor and board in your hardware design.

```cpp
#include <transducer.h>

const int pressurePin = A0;
Transducer pressureSensor(pressurePin, 0.0, 100.0);

void setup() {
  Serial.begin(115200);
}

void loop() {
  Serial.println(pressureSensor.readPressure());
  delay(100);
}
```

`readPressure()` returns a `float`. The pressure range passed to the constructor
is used for a linear conversion; `status()` and `value()` provide formatted
`String` results. The implementation assumes a 5 V reference, a 250 ohm shunt,
and a 10-bit analog reading. Check those assumptions against the board and
transducer circuit before using the result.

## Component API

| Header / class | Purpose | Main methods |
|---|---|---|
| `transducer.h` / `Transducer` | Analog pressure transducer conversion | `readPressure()`, `status()`, `value()`, `barToPSI()`, `PSIToBar()` |
| `relay.h` / `Relay` | Digital output relay with a timed actuation queue | `open()`, `close()`, `toggle()`, `state()`, `setNextActuation()`, `checkScheduledActuation()`, `clearSchedule()`, `neutralize()` |
| `mbv.h` / `MBV` | Directional motor control with limit switches | `open()`, `close()`, `jog()`, `setSpeed()`, `setDirection()`, `run()`, `brake()`, `coast()`, `setNextActuation()`, `checkScheduledActuation()`, `check_limits()` |
| `mbvContinuous.h` / `MBV` | Encoder-based motor positioning | `move_degrees()`, `next_90()`, `update()`, `reset()`, `getCurrentPosition()`, `getCurrentDegrees()`, `isOpen()`, `setNextActuation()`, `checkScheduledActuation()` |
| `loadCell.h` / `LoadCell` | HX711 load-cell measurement and calibration | `read()`, `getAverage()`, `calibrateCell()`, `setCalFactor()`, `tare()`, `join()`, `readJoint()` |
| `thrustCell.h` / `ThrustCell` | Qwiic Scale NAU7802 measurement and calibration | `read()`, `getAverageReading()`, `calibrate()`, `setCalFactor()`, `tare()` |
| `thermocouple.h` / `Thermocouple` | MCP960x thermocouple and ambient readings | `checkConnection()`, `readHot()`, `readCold()`, `readADC()`, `cToF()` |
| `heater.h` / `Heater` | Relay output controlled using a `Transducer` reading | `setTarget()`, `update()`, `on()`, `off()`, `isOn()` |
| `redline.h` / `Redline` | Consecutive-condition counter that invokes a callback | `checkTrigger()`, `setTriggerCondition()`, `setResponse()`, `setThreshold()` |
| `serial.h` / `SerialDualClass` | `Print` implementation forwarding output to two `HardwareSerial` ports | `begin()`, `setActive()`, `print()`, `println()` |
| `cmdBuffer.h` / `CmdBuffer` | Read a newline-terminated command from a `Stream` | `feed(Stream&, String&)` |

### Notes on component behavior

- Timed `Relay` actions are not automatic background tasks. Call
  `checkScheduledActuation()` regularly from `loop()` to process due actions.
  The queue holds up to 10 actions.
- The encoder-based `MBV` also requires regular calls to `update()` while
  moving. Its scheduled-action queue must be serviced with
  `checkScheduledActuation()`. In the current implementation, scheduled
  encoder-based actions call `next_90()`; the `degrees` argument to
  `setNextActuation()` is not used.
- `LoadCell::read()` and `ThrustCell::read()` return values converted to pounds
  by the current implementations. Calibration factors must match the installed
  sensor and the units used during calibration.
- `Thermocouple::readHot()` and `readCold()` return the MCP sensor readings;
  use `cToF()` if Fahrenheit is needed.

## Example sketch

See [`examples/mega`](examples/mega) for a larger integration example. It uses
Arduino Mega-specific pins, peripherals, calibration values, and application
logic; adapt it only after confirming compatibility with the actual board and
hardware.

## Safety and validation

This library is not safety-certified and does not replace independent hardware
interlocks, operating procedures, or engineering review. Software callbacks and
redline checks must not be treated as the sole means of preventing hazardous
conditions. Validate pin assignments, electrical levels, sensor scaling,
actuator behavior, and failure handling on an appropriate non-operational test
setup before connecting real equipment.
