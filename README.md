# Kitchen timer

[esphome](https://github.com/esphome/esphome) kitchen timer project.

## Hardware

 * ESP32c3
 * tm1637 display
 * Rotary encoder with button
 * TPS63021 buck-boost converter module
 * 32768Hz crystal
 * 1S BOM module USB (TP4056)
 * 1500mAh battery
 * Passive buzzer
 * VEML7700 light sensor

## Schematics

### RTC Clock

The 32768Hz external crystal is used for precise timekeeping. It's connected to GPIO0 and GPIO1 pins. Mine works fine standalone without any resistors and capacitors.

See:

* [RTC Timer Clock Sources](https://docs.espressif.com/projects/esp-idf/en/v5.2/esp32c6/api-reference/system/system_time.html#rtc-timer-clock-sources)
* [Schematic Checklist](https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32c6/schematic-checklist.html#rtc-clock-source-optional)

### TM1637 display

* clk - GPIO21
* dio - GPIO10

### Passive buzzer

Connected VIA NPN transitor. The N-channel mosfet could be used

* buzzer+ - VCC
* buzzer- - transistor collector
* transistor emmiter - GND
* transistor base - GPIO6

### TP4056

Detecting charging, charge done and charge current

* PROG - GPIO4
* CHRG - GPIO5
* STDBY - GPIO7
* BAT - GPIO3 via 1/2 voltage divider and avaraging capacitor between GPIO3 and GND. Devider resistance could be around 500K - 1M. Bigger values will drain less power but will be noisy.

### Rotary Encoder

If your encoder has any pull up or pull down resistor it's good idea to remove them to save power.

I run out of pins so reused the USB pins. So don't touch encoder while flashing.

* A - GPIO18
* B - GPIO19
* Button - GPIO2

### TPS63021 buck-boost converter module

Configure it for 3.3v output. Maybe add extra output capacitor.

We use it in power safe mode most of the time. For flashing and while it's WiFi connected it's in normal mode

* PS - GPIO20
* OUT - 3.3v
* GND - GND

### VEML7700 light sensor

The only compnent on I2C bus

* SDA - GPIO8
* SCL - GPIO9
