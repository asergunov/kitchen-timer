# Kitchen timer

[esphome](https://github.com/esphome/esphome) kitchen timer project.

## Hardware

 * ESP32c3
 * tm1637 display
 * Rotary encoder with button
 * TPS63021 buck-boost converter module
 * 32768Hz crystal
 * 1S BOM module USB
 * 1500mAh battery
 * Passive buzzer

## Connection

### RTC Clock

The 32768Hz external crystal is used for precise timekeeping. It's connected to GPIO0 and GPIO1 pins. Mine works fine standalone without any resistors and capacitors.

See:

* [RTC Timer Clock Sources](https://docs.espressif.com/projects/esp-idf/en/v5.2/esp32c6/api-reference/system/system_time.html#rtc-timer-clock-sources)
* [Schematic Checklist](https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32c6/schematic-checklist.html#rtc-clock-source-optional)
