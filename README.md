# Rule Indicators

Arduino-based industrial machine indicator system that displays distance measurements, operating hours, current time, and temperature on 7-segment displays.

## Features

- **Distance Tracking**: Measures distance traveled by a ruler using a rotary encoder
- **Operating Hours**: Tracks motor and spindle running time with persistent storage (EEPROM)
- **Real-Time Clock**: Displays current time in HH:MM:SS format
- **Temperature Monitoring**: Displays temperature with overheat protection (auto-activates at >50°C)
- **Multiple Display Modes**: Cycle through 5 different display modes with a button
- **Configurable Values**: Adjust distance using a secondary encoder with acceleration

## Display Modes

| Mode | Display | Format | Description |
|------|---------|--------|-------------|
| 0 | Distance | ±NNNNN.NN mm | Distance traveled by ruler |
| 1 | Motor Hours | NNNNN.MM | Total motor operating hours |
| 2 | Spindle Hours | NNNNN.MM | Total spindle operating hours |
| 3 | Current Time | HHMMSS | Real-time clock display |
| 4 | Temperature | ±NNN.NN °C | Current temperature reading |

## Hardware Requirements

### Main Components

- Arduino board (Uno/Nano compatible)
- 4× PCF8574AP I2C expanders (addresses: 0x39, 0x3D, 0x3F, 0x3B)
- 8× 7-segment displays with BCD decoders
- DS1307 RTC module with AT24C32 EEPROM
- DS18B20 temperature sensor
- 2× Rotary encoders (one with button)

### Pin Configuration

| Pin | Function |
|-----|----------|
| 2 | Encoder 1 - Channel B (ruler) |
| 3 | Encoder 1 - Channel A (ruler) |
| 4 | Encoder 2 - Channel B (config) |
| 5 | Encoder 2 - Channel A (config) |
| 6 | Encoder 2 - Reset button |
| 7 | Time indicator LED |
| 8 | Distance indicator LED |
| 9 | Mode button |
| 10 | General start signal input |
| 11 | Spindle start signal input |
| 12 | DS18B20 temperature sensor (OneWire) |
| A4 | I2C SDA |
| A5 | I2C SCL |

## Dependencies

Install these libraries via Arduino Library Manager or manually:

- [PCF8574](https://github.com/RobTillaart/PCF8574) - I2C GPIO expander
- [PinChangeInterrupt](https://github.com/NicoHood/PinChangeInterrupt) - Pin change interrupts
- [RTClib](https://github.com/adafruit/RTClib) - RTC DS1307 support
- [AT24CX](https://github.com/cyberp/AT24Cx) - EEPROM read/write
- [OneWire](https://github.com/PaulStoffregen/OneWire) - OneWire protocol
- [DallasTemperature](https://github.com/milesburton/Arduino-Temperature-Control-Library) - DS18B20 sensor

## Installation

1. Clone or download this repository
2. Install required libraries (see Dependencies)
3. Open `sketch.ino` in Arduino IDE
4. Connect hardware according to pin configuration
5. Upload to Arduino

### First-Time RTC Setup

Uncomment the following line in `setupRTCModule()` to set the RTC time (run once, then comment out again):

```cpp
// RTC.adjust(DateTime(__DATE__, __TIME__));
```

## Usage

- **Mode Button (Pin 9)**: Press to cycle through display modes
- **Encoder 2 Button (Pin 6)**: Press to reset distance to zero
- **Encoder 2 Rotation**: Adjust distance value with acceleration
- **Start Signals**: Connect machine signals to pins 10 and 11 to track operating hours

### Temperature Protection

When temperature exceeds 50°C, the display automatically switches to temperature mode and locks mode switching until the temperature drops below the threshold.

### Data Persistence

Motor and spindle operating hours are automatically saved to EEPROM every 60 seconds when the respective start signal is active.

---

## Changelog

### 28.02.24

- Decreased temperature number from 5 to 4 digits to show
- Commented out ability to change motorSeconds and spindelMotorSeconds by second encoder. Saved in code for future

### 13.02.24

- Decreased bytes amount for global variables prevDistanceIncStep, prevMotorIncStep, prevSpindelIncStep (from int64_t to int8_t) to decrease using of dynamic memory of arduino. They were incorrect at start. By my calculations, dynamic memory usage will be reduced from 1622 bytes to 1454 bytes.
- Added commented code that can be used to decrease dynamic memory usage from 64 to 1 byte, but this code depends on the delay of loop function and can be unstable

### 11.02.24

- Fixed incorrect time for RTC. Incorrect time was a result of adjusting RTC with time of compiling of code. This is need to be done only once. https://forum.arduino.cc/t/why-my-rtc-module-is-not-keeping-time/1000098
- Added different delays for different modes. To decrease load on arduino. Delay in 500 ms for distance mode and 1000ms for other modes
- Added a module and mode that shows temperature. In any mode, the temperature should be read and checked. If the temperature is greater than 50 degrees Celsius, then it should showed instead of all modes whenever.
  - If the temperature is greater than 50 degrees Celsius, then you cannot switch modes until the temperature drops
  - Used https://randomnerdtutorials.com/guide-for-ds18b20-temperature-sensor-with-arduino/ guide and `<OneWire.h>` `<DallasTemperature.h>` libs

### 10.02.24

- In mods 2 and 3 (motor hours) turned on p2 and p3 respectively on decoder 0(first lamp). For better definition of millimeters, motor hours and spindle hours.
- Fixed timers changing logic. It was bug that zeroing timer when it is needed to decrease time
- Made a 4th mode of viewing to display current time formatted as hoursminutesseconds
- Renamed setupPCFs to setupModules

### 08.02.24

- Added seconds changing for timers by 2nd rotator. Used the same logic as for distance changing by 2nd rotator
- Added max and min values for timers. min is 0 and max is 359999999("99999.99" on display)

### 07.02.24

- Number showing logic: removed unnecessary code for 0 showing. Changed while state, so now while decoder_i > 0 loop will be worked for showing numbers. In case of number of digits is more then 7, only last 7 digits will be shown
- Limits of timers: zeroing timers if they more then 359996400 sec or "99999.00" hours because of only 5 lamps are to show hours and limited logic of showing 7+ numbers. Zeroing at devices setup
- Added info to console output:
  - Status of 3 buttons - not to pop up, but to be immediately clear
    - general button
    - spindel button
    - mode button
  - Present time (to check the clock module)
- Renamed general* and spindel* variables to more appropriate
- Added delay for modeButton interruption to avoid button rattling. Created varuable that handle previous currTime. If currTime - prev currTime >= 1 then button will change mode to next
- Changed pins of mode button and time dot indicator. For easy soldering

### 04.02.24

- Fixed bug that 1 decoder (first number) is not changed to 0 when number decreased
- Added buttons that simulate signals that will be passed when machine start work and when spindel start work
- Added functionality to increase independently machine work time and spindel work time
  - increasing work time
  - saving work time
  - added mode for showing spindel work time
- Removed unnecessary variables
- Added some debug prints and usefull coments
- Added full wokwi project code

### 03.02.24

- Correct time of working. Done by reading time from RTC and comparing to prev value
- Added indicators for dots in both distance mode and time mode
- Added AR24C32 module support. Time read and write from module.
  - Added AT24CX library from https://github.com/cyberp/AT24Cx/tree/master
- Added useful comments

### Earlier

- Rewrote number showing
- Added clock module
- Added button and mode for showing hours and minutes
