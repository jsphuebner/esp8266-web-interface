Arduino IDE setup
=================

# Table of Contents
<details>
 <summary>Click to open TOC</summary>
<!-- MarkdownTOC autolink="true" levels="1,2,3,4,5,6" bracket="round" style="unordered" indent="    " autoanchor="false" markdown_preview="github" -->

- [About Arduino IDE](#about-arduino-ide)
- [Installing Arduino IDE and plugins](#installing-arduino-ide-and-plugins)
- [Configuring Arduino IDE](#configuring-arduino-ide)

<!-- /MarkdownTOC -->
</details>

# About Arduino IDE

Arduino IDE is an open-source integrated development environment with support for multiple platforms.  

Learn more : https://www.arduino.cc/en/software

# Installing Arduino IDE and plugins

[Download](https://www.arduino.cc/en/software#download) the IDE, and follow the [Getting Started](https://www.arduino.cc/en/Guide)
guide.

Additionally, install (by following the instructions in the following links) the 2 following IDE plugins:
* https://github.com/esp8266/arduino-esp8266fs-plugin
* https://github.com/earlephilhower/arduino-esp8266littlefs-plugin

When you start the Arduino IDE, you should now have two additional options in the `Tools` menu:
* ESP8266 LittleFS Data Upload
* ESP8266 Sketch Data Upload

# Configuring Arduino IDE

In the `Preferences` pane for the IDE, look for `Additional Boards Manager URLs`, click on the button on the right, and append the following URL:
`https://arduino.esp8266.com/stable/package_esp8266com_index.json`

In the `Tools` menu, select the `Board` entry, click on the `Boards Manager...` submenu, enter `esp8266` in the search box and press Enter.

You should have one entry named `esp8266 by ESP8266 Community` ; click on `Install` and wait for installation.

Open the project `File` > `Open` and navigate to the `FSBrowser.ino` file, and open it.

Go back to the `Tools` menu, and in the `Board` entry select the `ESP8266 Boards` choose your board (`Olimex MOD-WIFI-ESP8266(-DEV))

Configure the other parameters the following way:

* Upload Speed : 921600
* CPU Frequency : 80MHz
* Flash Size: 2MB (FS:512KB OTA:~768KB)
* Debug port: Disabled
* Debug level: None
* lwIP Variant: v2 Lower Memory
* VTables: Flash
* C++ Exceptions: Disabled (new aborts on oom)
* Stack Protection: Disabled
* Erase Flash: Only Sketch
* SSL Support: All SSL Ciphers (most compatible)
* MMU: 32KB cache + 32KB IRAM (balanced)
* Non-32-bit access: Use pgm_read macros for IRAM/PROGMEM
* Port: (_lookup the port on which your USB/Serial adapter is. You can also choose the board if it's up, connected to your WiFi, for OTA flashing_)

That's it ! Your IDE should now be configured for your day to day operations.
