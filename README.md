# Chablon

> "the French term for a watch movement that is not completely assembled yet."


Today it's a Lua interpreter and a program that consumes almost all of
the available RAM to draw rectangles on the display of a
[PineTime](https://wiki.pine64.org/wiki/PineTime)
smart watch.

Some day, maybe, it's a smartwatch OS which you can extend using
[Fennel](https://fennel-lang.org/)

## How to build it

### Before you start

_There is no support for Bluetooth yet, so you can't install this OTA. You need to take the watch apart._

Specifically, you'll need to
[solder wires to the SWD pins of your watch](https://wiki.pine64.org/wiki/PineTime_Devkit_Wiring) and use
some kind of programmer hardware (e.g. ST-Link or Black Magic Probe)
to upload the binary to the device. I take no responsibility for any
untoward consqeuences: if you melt the watch, short the battery or set
fire to your furnishings, it's on you.

### For Nix users

If you have Nix, it should be relatively straightforward

    $ NIXPKGS_ALLOW_UNSUPPORTED_SYSTEM=1 nix-build .


### For everyone else

Otherwise, there are some prerequisites: consult the `default.nix`
file for up-to-date information (this README *might* not be updated
religiously, the `default.nix` is canonical), but the summary is

* you need the bare metal [arm-none-eabi toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads/product-release)

* nRF SDK 15.3: this is a source distribution, which you need to untar
  somewhere. https://developer.nordicsemi.com/nRF5_SDK/nRF5_SDK_v15.x.x/nRF5_SDK_15.3.0_59ac345.zip

* liblua.a from Lua 5.4.3, cross-compiled for ARM bare metal. Refer to `default.nix` for the compilation options which you should edit into `luaconf.h` and flags that you pass to its `make`.

* an installation of Lua 5.4.3 compiled for the machine you're
  building on. We need this for compiling Fennel sources into Lua,
  which happens at build time.

* Fennel 1.0, from https://fennel-lang.org/downloads/fennel-1.0.0

Once you've obtained and built the prerequisites, you can run `make`
in this directory. There are some defines at the top that you should
pass to it to say where all those other things are.

### Installing to the watch

The build process creates a file called `chablon.elf`, which I flash
using GDB. The process may differ depeinding on your programmer; for the [Black Magic Probe](https://1bitsquared.com/products/black-magic-probe) I do something like this:

    target extended-remote /dev/ttyBMP0
	monitor swdp_scan
	attach 1
	monitor rtt
	file /home/dan/src/pinetime/chablon/chablon.elf
	load

Your mileage may differ: consult a local expert (if you're fortunate
enough to have one), or the documentation for your programmer, or the
internet, or - I dunno, mythic runes or something.


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
