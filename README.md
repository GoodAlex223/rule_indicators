# Changes

## 07.02.24

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
  - Added AT24CX library from <https://github.com/cyberp/AT24Cx/tree/master>
- Added useful comments

## ?

- Rewrote number showing
- Added clock module
- Added button and mode for showing hours and minutes