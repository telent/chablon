# Chablon

> "the French term for a watch movement that is not completely assembled yet."


Today it's a Lua interpreter and a program that consumes almost all of
the available RAM to draw rectangles on the display of a PineTime
smart watch.

Some day, maybe, it's a smartwatch OS


## Ideas, plans

* SPI is currently polling, which is probably a battery killer. Switch
to interrupt-driven and tie it into some kind of scheduler (freertos
or lua coroutines) so we don't end up in callback hell

* add the lua interpreter [done]

* expose GPIO, SPI to lua [done]

* expose TWI for sensors/inputs

* can we trim the image size?

* some kind of font so we can write text to the screen

* screensaver

* figure out how to store lua scripts on the device (littlefs on
  spi flash?)

* figure out how to send lua scripts to the device w/o rebuilding (ble,
  but how?)

* scripts can get touch/ble/sensor events as a lua iterator

* drivers for buttons, sensors, touchscreen

* "background tasks" to set alarms, record sensor data, etc

* ble support for DFU



## Credits

* Infinitime, of which this was originally a fork

* Wasp-OS, as a reference source for the St7789 and an existence proof that
these devices can run high-level languages

* Lup Yuen Lee, for https://lupyuen.github.io/articles/optimising-pinetimes-display-driver-with-rust-and-mynewt and may other articles
