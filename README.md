# DMXMapper
An open source, real-time DMX lookup table.

## Purpose
In many legacy DMX systems, the lighting controller is limited by the amount of channels it can control. For example, the Scene Setter-48 can only control the first 48 channels. For some reason, most of these controllers cannot map the sliders to any specific channels, and only sends data to the first *n* channels.

This is not a problem for small DMX universes, but when 50 or more devices are involved, it becomes increasingly more difficult to control. The main limitations are:
1. The user cannot control all of the lights simulaneously, and therefore must set the address of only the most important devices to something in the beginning of the address space.
2. Many devices, like RGB lights, may have unneeded channels (like a strobe or dimmer) that only take up valuable channels. It may be desirable to keep these channels at a constant value to make room for more lights with the freed sliders.
3. If a lot of small, individual devices are working together to accomplish one task, like an array of single-channel incandescent lights, it may be desirable to control the brightness of all of them with one slider on the controller, in order to free up even more sliders.

## Solution
The DMX Mapper is a small, headless device which features a DMX input and a DMX output. A lookup-table (LUT) can be used to map certain input channels to output channels, or write a constant value into an output channel. The LUT is written onto a microSD card and inserted into the device, which then maps the channels in real-time.

## Features
- Buffers DMX input, so it will continue to output the previous data even if the input cable is removed.
- Saves LUT into EEPROM, so there is no need for the microSD card during operation.
- Does not require any PC software, which makes it extremely versatile and cross-platform.
- Powerered by USB, which can be plugged into the wall or a PC, due to its low power comsumption.
- Writes a detailed logfile to the microSD card, which can be used to easily diagnose any issues.

## Design
### Hardware
The DMX Mapper is based on the Teensy 4.1 microcontroller, which is an extremely fast Arduino-compatible MCU. For sending and receiving DMX signals, the device uses two MAX485 chips. The schematics and the EasyEDA PCB layout are in the [PCB](https://github.com/MaxPastushkov/DMXMapper/tree/master/PCB) folder.

### Software
The code is written using the [TeensyDMX](https://github.com/ssilverman/TeensyDMX) library written by Shawn Silverman and the [CSV-Parser](https://github.com/michalmonday/CSV-Parser-for-Arduino) library written by Michal Borowski. While the project is written in C++, my part of the code is mostly (if not completely) compatible with C. With some modification, it would be possible to write all of the software in C, if needed.

---
### Special thanks to Michael Khitrov and Theater You for the idea and motivation for this project!
