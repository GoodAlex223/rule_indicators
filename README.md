# Changes

## 04.02.24

- Fixed bug that 1 decoder (first number) is not changed to 0 when number decreased
- Added buttons that simulate signals that will be passed when machine start work and when spindel start work
- Added functionality to increase independently machine work time and spindel work time
    - increasing work time
    - saving work time
    - added mode for showing spindel work time
- Removed unnecessary variables
- Added some debug prints and usefull coments
- Added full wokwi project code


## 03.02.24

- Correct time of working. Done by reading time from RTC and comparing to prev value
- Added indicators for dots in both distance mode and time mode
- Added AR24C32 module support. Time read and write from module.
    - Added AT24CX library from https://github.com/cyberp/AT24Cx/tree/master
- Added useful comments

## ?

- Rewrote number showing
- Added clock module
- Added button and mode for showing hours and minutes