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

You can use an FTDI board to connect your esp8266 module to you laptop/desktop.
Only four cables are required between the FTDI and the esp8266 - 3.3V, GND, RX,
and TX.

TX on the esp8266 connects to RS on the FTDI and vice versa.

Connect the USB port of the FTDI to your laptop/desktop.

You can find the pinout of the Olimex esp8266 module here: https://github.com/OLIMEX/ESP8266/

Warning: be sure your FTDI board is set to 3.3v and not 5v mode. If it is in 5v
mode, it may damage your esp8266 module.

--------------------------------------------------------------------------------

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

4. Install the HTML/CSS/js files on the esp8266 which make up the web interface.

Tools > ESP8266 Sketch Data Upload


## Related links

https://arduino-esp8266.readthedocs.io/en/latest/index.html


inkscape -w 1024 -h 1024 input.svg -o output.png

https://css-tricks.com/snippets/css/a-guide-to-flexbox/

https://github.com/esp8266/Arduino
https://arduino-esp8266.readthedocs.io/en/latest/index.html
https://stackoverflow.com/questions/51161837/cors-issue-in-nodemcu-esp8266
https://developer.mozilla.org/en-US/docs/Web/API/FormData/append
https://stackoverflow.com/questions/13333378/how-can-javascript-upload-a-blob
https://developer.mozilla.org/en-US/docs/Web/API/XMLHttpRequest/Sending_and_Receiving_Binary_Data
https://stackoverflow.com/questions/64247232/post-multipart-form-data-with-file-data-from-url-using-javascript
https://stackoverflow.com/questions/26337344/download-file-url-using-javascript
https://stackoverflow.com/questions/44070437/how-to-get-a-file-or-blob-from-an-url-in-javascript
https://stackoverflow.com/questions/30714871/check-if-an-input-field-has-focus-in-vanilla-javascript

