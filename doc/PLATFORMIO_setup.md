# About PlatformIO

PlatformIO is an open-source development environment with support for multiple platforms.

Learn more : https://docs.platformio.org/

# Installing PlatformIO Core (Command line)

Follow these instructions for the initial install:
* [PlatformIO Core](http://docs.platformio.org/page/core.html)

Ensure that the tools are properly installed, and that you can test the following command:
```
$ pio --version
PlatformIO Core, version 6.1.0
```

# Customization of the project configuration file

In some cases you will want to customize some aspects of the project configuration file `platformio.ini`.

If these aspects are useful to others, then you should consider doing a PR to share those change.

However in some cases there are some changes that are specific to your working environment, for example the
name of the serial port on which you communicate with the ESP8266 board, etc...

To that end, you can create a local file named `platformio-local-override.ini` ; which is explicitely ignored
by the git version control (cf `.gitignore`).
In this file you'll be able to override the settings of the main `platformio.ini` file without having pending
file modifications.

As a starting point you can create it from the existing `platformio-local-override.ini.example`:
```
cp platformio-local-override.ini.example platformio-local-override.ini
```
and modify the file according to your own tastes / needs.
