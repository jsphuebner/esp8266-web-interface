# OpenInverter esp8266 Web Interface (Huebner Inverter)

This repository contains the software which runs on the esp8266 WiFi modules
used as part of the OpenInverter project. Its purpose is to provide a web
interface for the configuration and monitoring of an OpenInverter based system.

There are two parts to the esp8266 WiFi module software - a firmware and a web
application. The firmware implements a HTTP API and a small web server. The HTTP
API sits on top of a serial communication protocol between the esp8266 and the
OpenInverter board. The web server hosts the HTML/css/js files which make up the
end user web application.

## Connecting an esp8266 to your laptop/desktop for programming

You can use an FTDI board to connect your esp8266 module to your laptop/desktop
in order to program it. Only four cables are required between the FTDI and the
esp8266 - 3.3V, GND, RX,and TX.

TX on the esp8266 connects to RX on the FTDI and vice versa.

Connect the USB port of the FTDI to your laptop/desktop.

You can find the pinout of the Olimex esp8266 module here: https://github.com/OLIMEX/ESP8266/

Warning: be sure your FTDI board is set to 3.3v and not 5v mode. If it is in 5v
mode, it may damage your esp8266 module.


## Setting up your laptop/desktop to program an esp8266 board

1. Install the Arduino application
2. Add esp8266 board support in the Arduino application. See 
   https://github.com/esp8266/Arduino for more details on how to do this.



## Installing this software on an esp8266

1. Fetch the code in this repository.

Download the zip file at this link:
  https://github.com/jsphuebner/esp8266-web-interface/archive/refs/heads/master.zip

~~ OR ~~

Clone the code with git.

```
git clone https://github.com/jsphuebner/esp8266-web-interface.git
```

2. Open OpenInverterWeb/OpenInverterWeb.ino in the Arduino application.

3. Ensure you have the correct settings in Arduino for the esp8266.

  - Tools > Board > "Olimex MOD-WIFI-ESP8266(-DEV)""
  - Tools > Upload speed > 115200
  - Tools > Flash Size > 2MB
  - Tools > Debug Port > Disabled
  - Tools > Port > Whatever serial port your esp8266 appears on

4. Install the firmware.

Sketch > upload.

5. Install the HTML/CSS/js files on the esp8266 which make up the web interface.

Tools > ESP8266 Sketch Data Upload


## Makefile-based installation

If you already have a copy of the firmware/web interface installed and just want
to update the web interface to the latest version, you can use the makefile
included here. This can be useful when developing against the esp8266 for
iterating changes quickly.

1. Install the 'make' tool on your PC. E.g. sudo apt-get install build-essential
Debian/Ubuntu. 

2. Set the INVERTER_IP environment variable to the IP of your esp8266. If you
are connecting your PC directly to the esp8266, the IP will be 192.168.4.1
usually.

```
export INVERTER_IP=192.168.4.1
```

3. Upload the web interface files

```
make install
```


## Related links
* https://github.com/esp8266/Arduino
* https://arduino-esp8266.readthedocs.io/en/latest/index.html
