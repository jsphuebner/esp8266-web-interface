PlatformIO usage
================

# Table of Contents
<details>
 <summary>Click to open TOC</summary>
<!-- MarkdownTOC autolink="true" levels="1,2,3,4,5,6" bracket="round" style="unordered" indent="    " autoanchor="false" markdown_preview="github" -->

- [Existing targets and environments](#existing-targets-and-environments)
- [Building the code](#building-the-code)
- [Flashing resulting firmware to the ESP8266 board](#flashing-resulting-firmware-to-the-esp8266-board)
- [Check device output](#check-device-output)
    - [ROM bootloader messages](#rom-bootloader-messages)
    - [Normal output to the inverter](#normal-output-to-the-inverter)
- [Building and flashing the filesystem \(optional\)](#building-and-flashing-the-filesystem-optional)
    - [Building the filesystem](#building-the-filesystem)
    - [Flashing the filesystem](#flashing-the-filesystem)
- [Clean build files if needed](#clean-build-files-if-needed)

<!-- /MarkdownTOC -->
</details>

All the following commands assume that you are at the 'root' of the project, i.e.
in the same directory as the `platformio.ini` file.

# Existing targets and environments
You can list existing target and environments with
```
$ pio run --list-targets
Environment    Group     Name         Title                        Description
-------------  --------  -----------  ---------------------------  ----------------------
debug          Platform  buildfs      Build Filesystem Image
debug          Platform  erase        Erase Flash
debug          Platform  size         Program Size                 Calculate program size
debug          Platform  upload       Upload
debug          Platform  uploadfs     Upload Filesystem Image
debug          Platform  uploadfsota  Upload Filesystem Image OTA

release        Platform  buildfs      Build Filesystem Image
release        Platform  erase        Erase Flash
release        Platform  size         Program Size                 Calculate program size
release        Platform  upload       Upload
release        Platform  uploadfs     Upload Filesystem Image
release        Platform  uploadfsota  Upload Filesystem Image OTA
```

Some additional targets exist but are not show in this list:
* `clean` : clean all built files
* `envdump` : dump current build environment
* `monitor` : automatically start pio device monitor after successful build operation.

You will be able to select an environment with the `-e`, or `--environment` command line flag. The default environment is `release`
You will be able to select an target with the `-t`, or `--target` command line flag. With no flag, the default behaviour is to build the sources.

# Building the code
Run this command (need to do this after each update of the files):

```
$ pio run
Processing release (platform: espressif8266; framework: arduino; board: modwifi)
----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
Verbose mode can be enabled via `-v, --verbose` option
CONFIGURATION: https://docs.platformio.org/page/boards/espressif8266/modwifi.html
PLATFORM: Espressif 8266 (4.0.1) > Olimex MOD-WIFI-ESP8266(-DEV)
HARDWARE: ESP8266 80MHz, 80KB RAM, 2MB Flash
PACKAGES:
 - framework-arduinoespressif8266 @ 3.30002.0 (3.0.2)
 - tool-esptool @ 1.413.0 (4.13)
 - tool-esptoolpy @ 1.30300.0 (3.3.0)
 - toolchain-xtensa @ 2.100300.210717 (10.3.0)
Converting FSBrowser.ino
LDF: Library Dependency Finder -> https://bit.ly/configure-pio-ldf
LDF Modes: Finder ~ chain, Compatibility ~ soft
Found 35 compatible libraries
Scanning dependencies...
Dependency Graph
|-- ArduinoOTA @ 1.0
|   |-- ESP8266WiFi @ 1.0
|   |-- ESP8266mDNS @ 1.2
|   |   |-- ESP8266WiFi @ 1.0
|-- ESP8266HTTPUpdateServer @ 1.0
|   |-- ESP8266WebServer @ 1.0
|   |   |-- ESP8266WiFi @ 1.0
|   |-- ESP8266WiFi @ 1.0
|-- ESP8266WebServer @ 1.0
|   |-- ESP8266WiFi @ 1.0
|-- ESP8266WiFi @ 1.0
|-- ESP8266mDNS @ 1.2
|   |-- ESP8266WiFi @ 1.0
|-- Ticker @ 1.0
Building in release mode
Compiling .pio/build/release/src/FSBrowser.ino.cpp.o
Compiling .pio/build/release/src/src/arm_debug.cpp.o
...

...
Compiling .pio/build/release/FrameworkArduino/umm_malloc/umm_malloc.cpp.o
Compiling .pio/build/release/FrameworkArduino/umm_malloc/umm_poison.c.o
Archiving .pio/build/release/libFrameworkArduino.a
Indexing .pio/build/release/libFrameworkArduino.a
Linking .pio/build/release/firmware.elf
Retrieving maximum program size .pio/build/release/firmware.elf
Checking size .pio/build/release/firmware.elf
Advanced Memory Usage is available via "PlatformIO Home > Project Inspect"
RAM:   [====      ]  38.9% (used 31896 bytes from 81920 bytes)
Flash: [====      ]  36.9% (used 385089 bytes from 1044464 bytes)
Building .pio/build/release/firmware.bin
Creating BIN file ".pio/build/release/firmware.bin" using "..../.platformio/packages/framework-arduinoespressif8266/bootloaders/eboot/eboot.elf" and ".pio/build/release/firmware.elf"
====================================================================================================================================== [SUCCESS] Took 9.97 seconds ======================================================================================================================================

Environment    Status    Duration
-------------  --------  ------------
release        SUCCESS   00:00:09.971
======================================================================================================================================
```


# Flashing resulting firmware to the ESP8266 board
Note: you should first setup the ESP8266 in UART mode. (In general, keep the button depressed when applying power, then release the button)

```
$ pio run --target upload
Processing release (platform: espressif8266; framework: arduino; board: modwifi)
----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
Verbose mode can be enabled via `-v, --verbose` option
CONFIGURATION: https://docs.platformio.org/page/boards/espressif8266/modwifi.html
PLATFORM: Espressif 8266 (4.0.1) > Olimex MOD-WIFI-ESP8266(-DEV)
HARDWARE: ESP8266 80MHz, 80KB RAM, 2MB Flash
PACKAGES:
 - framework-arduinoespressif8266 @ 3.30002.0 (3.0.2)
 - tool-esptool @ 1.413.0 (4.13)
 - tool-esptoolpy @ 1.30300.0 (3.3.0)
 - tool-mklittlefs @ 1.203.210628 (2.3)
 - tool-mkspiffs @ 1.200.0 (2.0)
 - toolchain-xtensa @ 2.100300.210717 (10.3.0)
Converting FSBrowser.ino
LDF: Library Dependency Finder -> https://bit.ly/configure-pio-ldf
LDF Modes: Finder ~ chain, Compatibility ~ soft
Found 35 compatible libraries
Scanning dependencies...
Dependency Graph
|-- ArduinoOTA @ 1.0
|   |-- ESP8266WiFi @ 1.0
|   |-- ESP8266mDNS @ 1.2
|   |   |-- ESP8266WiFi @ 1.0
|-- ESP8266HTTPUpdateServer @ 1.0
|   |-- ESP8266WebServer @ 1.0
|   |   |-- ESP8266WiFi @ 1.0
|   |-- ESP8266WiFi @ 1.0
|-- ESP8266WebServer @ 1.0
|   |-- ESP8266WiFi @ 1.0
|-- ESP8266WiFi @ 1.0
|-- ESP8266mDNS @ 1.2
|   |-- ESP8266WiFi @ 1.0
|-- Ticker @ 1.0
Building in release mode
Compiling .pio/build/release/src/FSBrowser.ino.cpp.o
...

...
Retrieving maximum program size .pio/build/release/firmware.elf
Checking size .pio/build/release/firmware.elf
Advanced Memory Usage is available via "PlatformIO Home > Project Inspect"
RAM:   [====      ]  38.9% (used 31896 bytes from 81920 bytes)
Flash: [====      ]  36.9% (used 385089 bytes from 1044464 bytes)
Configuring upload protocol...
AVAILABLE: espota, esptool
CURRENT: upload_protocol = esptool
Looking for upload port...
Using manually specified: /dev/cu.usbserial-DGAJb113318
Uploading .pio/build/release/firmware.bin
esptool.py v3.3
Serial port /dev/cu.usbserial-DGAJb113318
WARNING: Pre-connection option "no_reset" was selected. Connection may fail if the chip is not in bootloader or flasher stub mode.
Connecting....
Chip is ESP8266EX
Features: WiFi
Crystal is 26MHz
MAC: c4:5b:be:76:1b:b8
Uploading stub...
Running stub...
Stub running...
Changing baud rate to 921600
Changed.
Configuring flash size...
Flash will be erased from 0x00000000 to 0x0005ffff...
Compressed 389248 bytes to 277860...
Writing at 0x00000000... (5 %)
Writing at 0x00005b7b... (11 %)
Writing at 0x0000b880... (17 %)
Writing at 0x00011ae4... (23 %)
Writing at 0x00017987... (29 %)
Writing at 0x0001d5ad... (35 %)
Writing at 0x00022dbe... (41 %)
Writing at 0x00028689... (47 %)
Writing at 0x0002df5c... (52 %)
Writing at 0x000330f0... (58 %)
Writing at 0x000380f6... (64 %)
Writing at 0x0003d293... (70 %)
Writing at 0x00042b6d... (76 %)
Writing at 0x000482ec... (82 %)
Writing at 0x0004d696... (88 %)
Writing at 0x00052cb4... (94 %)
Writing at 0x0005986c... (100 %)
Wrote 389248 bytes (277860 compressed) at 0x00000000 in 3.8 seconds (effective 816.7 kbit/s)...
Hash of data verified.

Leaving...
Soft resetting...
====================================================================================================================================== [SUCCESS] Took 8.37 seconds ======================================================================================================================================

Environment    Status    Duration
-------------  --------  ------------
release        SUCCESS   00:00:08.371
====================================================================================================================================== 1 succeeded in 00:00:08.371 ======================================================================================================================================
```


# Check device output

## ROM bootloader messages

Most of the ESP8266 boards have a 26MHz crystal (instead of the standard 40MHz) so the ROM bootloader outputs at 115200 * 26/40 = 74880.

These messages are not very useful but can help you to confirm that the board is working as expected.
```
$ pio device monitor --baud 74880
--- Terminal on /dev/cu.usbserial-DGAJb113318 | 74880 8-N-1
--- Available filters and text transformations: colorize, debug, default, direct, esp8266_exception_decoder, hexlify, log2file, nocontrol, printable, send_on_enter, time
--- More details at https://bit.ly/pio-monitor-filters
--- Quit: Ctrl+C | Menu: Ctrl+T | Help: Ctrl+T followed by Ctrl+H

 ets Jan  8 2013,rst cause:1, boot mode:(3,0)

load 0x4010f000, len 3460, room 16
tail 4
chksum 0xcc
load 0x3fff20b8, len 40, room 4
tail 4
chksum 0xc9
csum 0xc9
v0005f080
~ld
```

## Normal output to the inverter

In normal operations, the Web Interface wants to talk to an inverter. The following will show the messages that it tries to send to the inverter:
```
$ pio device monitor
--- Terminal on /dev/cu.usbserial-DGAJb113318 | 115200 8-N-1
--- Available filters and text transformations: colorize, debug, default, direct, esp8266_exception_decoder, hexlify, log2file, nocontrol, printable, send_on_enter, time
--- More details at https://bit.ly/pio-monitor-filters
--- Quit: Ctrl+C | Menu: Ctrl+T | Help: Ctrl+T followed by Ctrl+H
fastuart

json
```

# Building and flashing the filesystem (optional)
It's also possible to automate the building of the filesystem, and its uploading.

(It's optional as you can also do it by using the `/edit` endpoint of the main application - there's an `upload.sh` file for this.)

## Building the filesystem
This will only build it.
```
$ pio run --target buildfs
Processing release (platform: espressif8266; framework: arduino; board: modwifi)
----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
Verbose mode can be enabled via `-v, --verbose` option
CONFIGURATION: https://docs.platformio.org/page/boards/espressif8266/modwifi.html
PLATFORM: Espressif 8266 (4.0.1) > Olimex MOD-WIFI-ESP8266(-DEV)
HARDWARE: ESP8266 80MHz, 80KB RAM, 2MB Flash
PACKAGES:
 - framework-arduinoespressif8266 @ 3.30002.0 (3.0.2)
 - tool-esptool @ 1.413.0 (4.13)
 - tool-esptoolpy @ 1.30300.0 (3.3.0)
 - tool-mklittlefs @ 1.203.210628 (2.3)
 - tool-mkspiffs @ 1.200.0 (2.0)
 - toolchain-xtensa @ 2.100300.210717 (10.3.0)
Converting FSBrowser.ino
LDF: Library Dependency Finder -> https://bit.ly/configure-pio-ldf
LDF Modes: Finder ~ chain, Compatibility ~ soft
Found 35 compatible libraries
Scanning dependencies...
Dependency Graph
|-- ArduinoOTA @ 1.0
|   |-- ESP8266WiFi @ 1.0
|   |-- ESP8266mDNS @ 1.2
|   |   |-- ESP8266WiFi @ 1.0
|-- ESP8266HTTPUpdateServer @ 1.0
|   |-- ESP8266WebServer @ 1.0
|   |   |-- ESP8266WiFi @ 1.0
|   |-- ESP8266WiFi @ 1.0
|-- ESP8266WebServer @ 1.0
|   |-- ESP8266WiFi @ 1.0
|-- ESP8266WiFi @ 1.0
|-- ESP8266mDNS @ 1.2
|   |-- ESP8266WiFi @ 1.0
|-- Ticker @ 1.0
Building in release mode
Building file system image from 'FSBrowser/data' directory to .pio/build/release/spiffs.bin
/wifi-updated.html
/gauges.html
/ajax-loader.gif
/index.html
/inverter.js
/remote.html
/chart.min.js.gz
/syncofs.html
/gauge.min.js.gz
/log.js
/index.js
/jquery.core.min.js.gz
/chartjs-annotation.min.js.gz
/wifi.html
/log.html
/style.css
/gauges.js
/jquery.knob.min.js.gz
/refresh.png
====================================================================================================================================== [SUCCESS] Took 1.38 seconds ======================================================================================================================================

Environment    Status    Duration
-------------  --------  ------------
release        SUCCESS   00:00:01.383
====================================================================================================================================== 1 succeeded in 00:00:01.383 ======================================================================================================================================
```

## Flashing the filesystem
This action does the build + flash steps in one operation.
```
$ pio run --target uploadfs
Processing release (platform: espressif8266; framework: arduino; board: modwifi)
----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
Verbose mode can be enabled via `-v, --verbose` option
CONFIGURATION: https://docs.platformio.org/page/boards/espressif8266/modwifi.html
PLATFORM: Espressif 8266 (4.0.1) > Olimex MOD-WIFI-ESP8266(-DEV)
HARDWARE: ESP8266 80MHz, 80KB RAM, 2MB Flash
PACKAGES:
 - framework-arduinoespressif8266 @ 3.30002.0 (3.0.2)
 - tool-esptool @ 1.413.0 (4.13)
 - tool-esptoolpy @ 1.30300.0 (3.3.0)
 - tool-mklittlefs @ 1.203.210628 (2.3)
 - tool-mkspiffs @ 1.200.0 (2.0)
 - toolchain-xtensa @ 2.100300.210717 (10.3.0)
Converting FSBrowser.ino
LDF: Library Dependency Finder -> https://bit.ly/configure-pio-ldf
LDF Modes: Finder ~ chain, Compatibility ~ soft
Found 35 compatible libraries
Scanning dependencies...
Dependency Graph
|-- ArduinoOTA @ 1.0
|   |-- ESP8266WiFi @ 1.0
|   |-- ESP8266mDNS @ 1.2
|   |   |-- ESP8266WiFi @ 1.0
|-- ESP8266HTTPUpdateServer @ 1.0
|   |-- ESP8266WebServer @ 1.0
|   |   |-- ESP8266WiFi @ 1.0
|   |-- ESP8266WiFi @ 1.0
|-- ESP8266WebServer @ 1.0
|   |-- ESP8266WiFi @ 1.0
|-- ESP8266WiFi @ 1.0
|-- ESP8266mDNS @ 1.2
|   |-- ESP8266WiFi @ 1.0
|-- Ticker @ 1.0
Building in release mode
Building file system image from 'FSBrowser/data' directory to .pio/build/release/spiffs.bin
/wifi-updated.html
/gauges.html
/ajax-loader.gif
/index.html
/inverter.js
/remote.html
/chart.min.js.gz
/syncofs.html
/gauge.min.js.gz
/log.js
/index.js
/jquery.core.min.js.gz
/chartjs-annotation.min.js.gz
/wifi.html
/log.html
/style.css
/gauges.js
/jquery.knob.min.js.gz
/refresh.png
Looking for upload port...
Using manually specified: /dev/cu.usbserial-DGAJb113318
Uploading .pio/build/release/spiffs.bin
esptool.py v3.3
Serial port /dev/cu.usbserial-DGAJb113318
WARNING: Pre-connection option "no_reset" was selected. Connection may fail if the chip is not in bootloader or flasher stub mode.
Connecting....
Chip is ESP8266EX
Features: WiFi
Crystal is 26MHz
MAC: c4:5b:be:76:1b:b8
Uploading stub...
Running stub...
Stub running...
Changing baud rate to 921600
Changed.
Configuring flash size...
Flash will be erased from 0x00180000 to 0x001f9fff...
Compressed 499712 bytes to 140809...
Writing at 0x00180000... (11 %)
Writing at 0x0018c88a... (22 %)
Writing at 0x001945b8... (33 %)
Writing at 0x0019c553... (44 %)
Writing at 0x001a4604... (55 %)
Writing at 0x001af7a7... (66 %)
Writing at 0x001b9998... (77 %)
Writing at 0x001c5b85... (88 %)
Writing at 0x001cdaa6... (100 %)
Wrote 499712 bytes (140809 compressed) at 0x00180000 in 2.9 seconds (effective 1371.8 kbit/s)...
Hash of data verified.

Leaving...
Soft resetting...
====================================================================================================================================== [SUCCESS] Took 5.93 seconds ======================================================================================================================================

Environment    Status    Duration
-------------  --------  ------------
release        SUCCESS   00:00:05.926
====================================================================================================================================== 1 succeeded in 00:00:05.926 ======================================================================================================================================
```

# Clean build files if needed
```
$ pio run --target clean
Processing release (platform: espressif8266; framework: arduino; board: modwifi)
----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
Build environment is clean
Done cleaning
====================================================================================================================================== [SUCCESS] Took 0.39 seconds ======================================================================================================================================

Environment    Status    Duration
-------------  --------  ------------
release        SUCCESS   00:00:00.389
====================================================================================================================================== 1 succeeded in 00:00:00.389 ======================================================================================================================================
```
