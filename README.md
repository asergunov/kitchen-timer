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

### ESP32c3 Super Mini

Made few modifications on dev board:

* removed the power LED. Crashed it carefully with screw driver, checked it doesn't have any resistace or continuity.
* soldered wires on GPIO18, GPIO19 (USB data). Not to pins itself but to resistors connected to these pins. Dirty.

### Pinout conciderations

Anything working in deep sleep should be in RTC domain:

* Fixed pins GPIO0 GPIO1 for external RTC clock crystal
* Rotary encder button to wake it up
* Charge in progress and Charge Done sensors

ADC1 pins (because ADC2 can't work simultanuiosly wit WiFi)

* Charge current senesor
* Battery voltage

Power Saving pin could be just pulled by default. We need to switch to normal mode only when controller is active.

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

Module is powered by battry and produced 3.3v output.

To not waste battery when it's on cable the converter powered by charger USB input. There are guides how to achive that with single MOSFET. I had to add extra diode to prevent charger powering from battery.

We use it in power safe mode most of the time. For flashing and while it's WiFi connected it's in normal mode

* PS - GPIO20
* OUT - 3.3v
* GND - GND

### VEML7700 light sensor

The only compnent on I2C bus

* SDA - GPIO8
* SCL - GPIO9

## Calibraion

For calibration is nice to habe Home Assitant instance you can export sensor data as csv. Then it can be analyzed in Calc or any other software.

### ADC and voltage Dividers

It's good idea to calibrate your voltage dividers by known voltage sources.

### Battery level

I've made full charge/discharge cycle with Home Assistant tracking voltage and current.

Once data exported I found out charge percentage by remain time.

Aproximated the `discharge_percentage(battery_voltage)` and `charge_percentage(battery_volatage, charge_current)` functions in linear intervals.

The discharge curve I've seen was almost linear in first 90% then goes down really fast.

The charging there is two phases:

1. Current limited charging. When it decreases voltage in time by current limit you set.
2. Constant voltage charging. When it keeps the voltage the same but tracking current. Once the current is less then it's threshhold it stops charging.

So first half of charging percntage depends on voltage, second part of charging percentage depends on current.

## Minimize power consmption in sleep mode

I was using the Nordic Power Profiler Kit II.

Powering by 3.3v rail made sure nothing was missing like power LED or leakage of battery current into charger module VCC.

Powering by conveter input with different voltages I figured out the best output capacitor to minimize power consumtion in sleep mode of my devce and with power safe mode of buck converter active. It was not even about capacitace but about to find exact capacitor giving the best results.

By the end I had about 100uA consumption from battery in deep sleep mode.
