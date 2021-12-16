/* 
  FSWebServer - Example WebServer with SPIFFS backend for esp8266
  Copyright (c) 2015 Hristo Gochkov. All rights reserved.
  This file is part of the ESP8266WebServer library for Arduino environment.
 
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
  
  upload the contents of the data folder with MkSPIFFS Tool ("ESP8266 Sketch Data Upload" in Tools menu in Arduino IDE)
  or you can upload the contents of a folder if you CD in that folder and run the following command:
  for file in `ls -A1`; do curl -F "file=@$PWD/$file" esp8266fs.local/edit; done
  
  access the sample web page at http://esp8266fs.local
  edit the page by going to http://esp8266fs.local/edit
*/
/*
 * This file is part of the esp8266 web interface
 *
 * Copyright (C) 2018 Johannes Huebner <dev@johanneshuebner.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <FS.h>
#include <Ticker.h>

#define DBG_OUTPUT_PORT Serial

const char* host = "inverter";
bool fastUart = false;
bool fastUartAvailable = true;

ESP8266WebServer server(80);
ESP8266HTTPUpdateServer updater;
//holds the current upload
File fsUploadFile;
Ticker sta_tick;

//SWD over ESP8266
/*
  https://github.com/scanlime/esp8266-arm-swd
*/
#include "src/arm_debug.h"
#include <StreamString.h>
uint32_t addr = 0x08000000;
uint32_t addrEnd = 0x0801ffff;
const uint8_t swd_clock_pin = 4; //GPIO4 (D2)
const uint8_t swd_data_pin = 5; //GPIO5 (D1)
ARMDebug swd(swd_clock_pin, swd_data_pin, ARMDebug::LOG_NONE);

//format bytes
String formatBytes(size_t bytes){
  if (bytes < 1024){
    return String(bytes)+"B";
  } else if(bytes < (1024 * 1024)){
    return String(bytes/1024.0)+"KB";
  } else if(bytes < (1024 * 1024 * 1024)){
    return String(bytes/1024.0/1024.0)+"MB";
  } else {
    return String(bytes/1024.0/1024.0/1024.0)+"GB";
  }
}

String getContentType(String filename){
  if(server.hasArg("download")) return "application/octet-stream";
  else if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".xml")) return "text/xml";
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path){
  //DBG_OUTPUT_PORT.println("handleFileRead: " + path);
  if(path.endsWith("/")) path += "index.html";
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if(SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)){
    if(SPIFFS.exists(pathWithGz))
      path += ".gz";
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

void handleFileUpload(){
  if(server.uri() != "/edit") return;
  HTTPUpload& upload = server.upload();
  if(upload.status == UPLOAD_FILE_START){
    String filename = upload.filename;
    if(!filename.startsWith("/")) filename = "/"+filename;
    //DBG_OUTPUT_PORT.print("handleFileUpload Name: "); DBG_OUTPUT_PORT.println(filename);
    fsUploadFile = SPIFFS.open(filename, "w");
    filename = String();
  } else if(upload.status == UPLOAD_FILE_WRITE){
    //DBG_OUTPUT_PORT.print("handleFileUpload Data: "); DBG_OUTPUT_PORT.println(upload.currentSize);
    if(fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize);
  } else if(upload.status == UPLOAD_FILE_END){
    if(fsUploadFile)
      fsUploadFile.close();
    //DBG_OUTPUT_PORT.print("handleFileUpload Size: "); DBG_OUTPUT_PORT.println(upload.totalSize);
  }
}

void handleFileDelete(){
  if(server.args() == 0) return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  //DBG_OUTPUT_PORT.println("handleFileDelete: " + path);
  if(path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if(!SPIFFS.exists(path))
    return server.send(404, "text/plain", "FileNotFound");
  SPIFFS.remove(path);
  server.send(200, "text/plain", "");
  path = String();
}

void handleFileCreate(){
  if(server.args() == 0)
    return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  //DBG_OUTPUT_PORT.println("handleFileCreate: " + path);
  if(path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if(SPIFFS.exists(path))
    return server.send(500, "text/plain", "FILE EXISTS");
  File file = SPIFFS.open(path, "w");
  if(file)
    file.close();
  else
    return server.send(500, "text/plain", "CREATE FAILED");
  server.send(200, "text/plain", "");
  path = String();
}

void handleFileList() {
  String path = "/";
  if(server.hasArg("dir")) 
    String path = server.arg("dir");
  //DBG_OUTPUT_PORT.println("handleFileList: " + path);
  Dir dir = SPIFFS.openDir(path);
  path = String();

  String output = "[";
  while(dir.next()){
    File entry = dir.openFile("r");
    if (output != "[") output += ',';
    bool isDir = false;
    output += "{\"type\":\"";
    output += (isDir)?"dir":"file";
    output += "\",\"name\":\"";
    output += String(entry.name()).substring(1);
    output += "\"}";
    entry.close();
  }
  
  output += "]";
  server.send(200, "text/json", output);
}

static void sendCommand(String cmd)
{
  Serial.print(cmd);
  Serial.print("\n");

  char c;
  uint32_t timeout = millis();
  do {
    if(Serial.available() > 0)
      c = Serial.read();
  } while (millis() - timeout < 256 && c != '\n');
}

static void handleCommand() {
  if(!server.hasArg("cmd")) {server.send(500, "text/plain", "BAD ARGS"); return;}

  //const int cmdBufSize = 128;
  //String cmd = server.arg("cmd").substring(0, cmdBufSize);
  
  sendCommand(server.arg("cmd"));
  
  int repeat = 0;
  char buffer[255];
  size_t len = 0;
  String output;

  if (server.hasArg("repeat"))
    repeat = server.arg("repeat").toInt();
    
  do {
    memset(buffer,0,sizeof(buffer));
    len = Serial.readBytes(buffer, sizeof(buffer) - 1);
    output += buffer;

    if (repeat)
    {
      repeat--;
      Serial.print("!");
      Serial.readBytes(buffer, 1); //consume "!"
    }
  } while (len > 0);
  //server.sendHeader("Access-Control-Allow-Origin","*");
  server.send(200, "text/plain", output);
}

static uint32_t crc32_word(uint32_t Crc, uint32_t Data)
{
  int i;

  Crc = Crc ^ Data;

  for(i=0; i<32; i++)
    if (Crc & 0x80000000)
      Crc = (Crc << 1) ^ 0x04C11DB7; // Polynomial used in STM32
    else
      Crc = (Crc << 1);

  return(Crc);
}

static uint32_t crc32(uint32_t* data, uint32_t len, uint32_t crc)
{
   for (uint32_t i = 0; i < len; i++)
      crc = crc32_word(crc, data[i]);
   return crc;
}


static void handleUpdate()
{
  if(!server.hasArg("step") || !server.hasArg("file")) {server.send(500, "text/plain", "BAD ARGS"); return;}
  size_t PAGE_SIZE_BYTES = 1024;
  int step = server.arg("step").toInt();
  File file = SPIFFS.open(server.arg("file"), "r");
  int pages = (file.size() + PAGE_SIZE_BYTES - 1) / PAGE_SIZE_BYTES;
  String message;

  if (server.hasArg("pagesize"))
  {
    PAGE_SIZE_BYTES = server.arg("pagesize").toInt();
  }

  if (step == -1)
  {
    int c;
    char b[128];

    Serial.print("reset\n");
    Serial.readBytesUntil('t', b, sizeof(b) - 1); //echo -> reset

    if (fastUart)
    {
      Serial.begin(115200);
      fastUart = false;
      fastUartAvailable = true; //retry after reboot
    }
    Serial.setTimeout(1000); //default
    
    do {
      c = Serial.read();
    } while (c != 'S' && c != '2');

    if (c == '2') //version 2 bootloader
    {
      Serial.write(0xAA); //Send magic
      while (Serial.read() != 'S');
    }
    
    Serial.write(pages);
    while (Serial.read() != 'P');
    message = "reset";
  }
  else
  {
    bool repeat = true;
    file.seek(step * PAGE_SIZE_BYTES);
    char buffer[PAGE_SIZE_BYTES];
    size_t bytesRead = file.readBytes(buffer, sizeof(buffer));

    while (bytesRead < PAGE_SIZE_BYTES)
      buffer[bytesRead++] = 0xff;
    
    uint32_t crc = crc32((uint32_t*)buffer, PAGE_SIZE_BYTES / 4, 0xffffffff);

    while (repeat)
    {
      Serial.write(buffer, sizeof(buffer));
      while (!Serial.available());
      char res = Serial.read();

      if ('C' == res) {
        Serial.write((char*)&crc, sizeof(uint32_t));
        while (!Serial.available());
        res = Serial.read();
      }

      switch (res) {
        case 'D':
          message = "Update Done";
          repeat = false;
          fastUartAvailable = true;
          break;
        case 'E':
          while (Serial.read() != 'T');
          break;
        case 'P':
          message = "Page write success";
          repeat = false;
          break;
        default:
        case 'T':
          break;
      }
    }
  }
  server.send(200, "text/json", "{ \"message\": \"" + message + "\", \"pages\": " + pages + " }");
  file.close();
}

static void handleWifi()
{
  bool updated = true;
  if(server.hasArg("apSSID") && server.hasArg("apPW")) 
  {
    WiFi.softAP(server.arg("apSSID").c_str(), server.arg("apPW").c_str());
  }
  else if(server.hasArg("staSSID") && server.hasArg("staPW")) 
  {
    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(server.arg("staSSID").c_str(), server.arg("staPW").c_str());
  }
  else
  {
    File file = SPIFFS.open("/wifi.html", "r");
    String html = file.readString();
    file.close();
    html.replace("%staSSID%", WiFi.SSID());
    html.replace("%apSSID%", WiFi.softAPSSID());
    html.replace("%staIP%", WiFi.localIP().toString());
    server.send(200, "text/html", html);
    updated = false;
  }

  if (updated)
  {
    File file = SPIFFS.open("/wifi-updated.html", "r");
    size_t sent = server.streamFile(file, getContentType("wifi-updated.html"));
    file.close();    
  }
}

static void handleBaud()
{
  if (server.hasArg("timeout")) {
    Serial.setTimeout(server.arg("timeout").toInt());
  }

  if (fastUart)
    server.send(200, "text/html", "fastUart on");
  else
    server.send(200, "text/html", "fastUart off");
}

void staCheck(){
  sta_tick.detach();
  if(!(uint32_t)WiFi.localIP()){
    WiFi.mode(WIFI_AP); //disable station mode
  }
}

void setup(void){
  Serial.begin(115200);
  Serial.setTimeout(100);
  SPIFFS.begin();

  Serial.print("\n");
  while(Serial.available())
    Serial.read(); //flush all previous output
  
  if (!fastUart && fastUartAvailable)
  {
    sendCommand("fastuart");
    if (Serial.readString().startsWith("OK"))
    {
      Serial.begin(921600);
      fastUart = true;
    }
    else
    {
      fastUartAvailable = false;
    }
  }

  //WIFI INIT
  #ifdef WIFI_IS_OFF_AT_BOOT
    enableWiFiAtBootTime();
  #endif
  WiFi.mode(WIFI_AP_STA);
  WiFi.setPhyMode(WIFI_PHY_MODE_11B);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.setOutputPower(25); //dbm
  WiFi.begin();
  sta_tick.attach(10, staCheck);
  
  MDNS.begin(host);

  updater.setup(&server);
  
  //SERVER INIT
  ArduinoOTA.begin();
  //list directory
  server.on("/list", HTTP_GET, handleFileList);
  //load editor
  server.on("/edit", HTTP_GET, [](){
    if(!handleFileRead("/edit.htm")) server.send(404, "text/plain", "FileNotFound");
  });
  //create file
  server.on("/edit", HTTP_PUT, handleFileCreate);
  //delete file
  server.on("/edit", HTTP_DELETE, handleFileDelete);
  //first callback is called after the request has ended with all parsed arguments
  //second callback handles file uploads at that location
  server.on("/edit", HTTP_POST, [](){ server.send(200, "text/plain", ""); }, handleFileUpload);

  server.on("/wifi", handleWifi);
  server.on("/cmd", handleCommand);
  server.on("/fwupdate", handleUpdate);
  server.on("/baud", handleBaud);
  server.on("/version", [](){ server.send(200, "text/plain", "1.1.R"); });
  server.on("/swd/begin", []() {
    // See if we can communicate. If so, return information about the target.
    // This shouldn't reset the target, but it does need to communicate,
    // and the debug port itself will be reset.
    //
    // If all is well, this returns some identifying info about the target.

    uint32_t idcode;

    if (swd.begin() && swd.getIDCODE(idcode)) {

      char output[128];
      snprintf(output, sizeof output, "{\"connected\": true, \"idcode\": \"0x%02x\" }", idcode);
      server.send(200, "application/json", String(output));

    } else {
      server.send(200, "application/json", "{\"connected\": false}");
    }
  });
  server.on("/swd/uid", []() {
    // STM32F103 Reference Manual, Chapter 30.2 Unique device ID register (96 bits)
    // http://www.st.com/st-web-ui/static/active/en/resource/technical/document/reference_manual/CD00171190.pdf

    uint32_t REG_U_ID = 0x1FFFF7E8; //96 bits long, read using 3 read operations

    uint16_t off0;
    uint16_t off2;
    uint32_t off4;
    uint32_t off8;

    swd.memLoadHalf(REG_U_ID + 0x0, off0);
    swd.memLoadHalf(REG_U_ID + 0x2, off2);
    swd.memLoad(REG_U_ID + 0x4, off4);
    swd.memLoad(REG_U_ID + 0x8, off8);

    char output[128];
    snprintf(output, sizeof output, "{\"uid\": \"0x%04x-0x%04x-0x%08x-0x%08x\" }", off0, off2, off4, off8);
    server.send(200, "application/json", String(output));
  });
  server.on("/swd/halt", []() {
    if (swd.begin()) {
      char output[128];
      snprintf(output, sizeof output, "{\"halt\": \"%s\"}", swd.debugHalt() ? "true" : "false");
      server.send(200, "application/json", String(output));
    } else {
      server.send(200, "text/plain", "SWD Error");
    }
  });
  server.on("/swd/run", []() {
    if (swd.begin()) {
      char output[128];
      snprintf(output, sizeof output, "{\"run\": \"%s\"}", swd.debugRun() ? "true" : "false");
      server.send(200, "application/json", String(output));
    } else {
      server.send(200, "text/plain", "SWD Error");
    }
  });
  server.on("/swd/reset", []() {
    if (swd.begin()) {
      bool debugHalt = swd.debugHalt();
      bool debugReset = false;
      if (server.hasArg("hard")) {
        swd.reset();
        debugReset = true;
      } else {
        debugReset = swd.debugReset();
      }
      char output[128];
      snprintf(output, sizeof output, "{\"halt\": \"%s\", \"reset\": \"%s\"}", debugHalt ? "true" : "false", debugReset ? "true" : "false");
      server.send(200, "application/json", String(output));
    } else {
      server.send(200, "text/plain", "SWD Error");
    }
  });
  server.on("/swd/zero", []() {

    char output[128];

    if (swd.begin()) {

      uint32_t addrTotal = addrEnd - addr;
      server.setContentLength(CONTENT_LENGTH_UNKNOWN);
      server.send(200, "text/plain", "");

      swd.debugHalt();
      swd.debugHaltOnReset(1);
      swd.reset();
      swd.unlockFlash();

      //METHOD #1
      swd.flashEraseAll();

      //METHOD #2
      // Before programming internal SRAM, the ARM Cortex-M3 should first be reset and halted.
      /*
        1. Write 0xA05F0003 to DHCSR. This will halt the core.
        2. Write 1 to bit VC_CORERESET in DEMCR. This will enable halt-on-reset
        3. Write 0xFA050004 to AIRCR. This will reset the core.
      */
      //swd.flashloaderSRAM();

      uint32_t addrNext = addr;
      uint32_t addrIndex = 0;
      uint32_t addrBuffer = 0x00000000; //Used by METHOD #2
      do {
        //Serial.printf("------ %08x -> %08x ------\n", addrNext, addrBuffer);

        snprintf(output, sizeof output, "%08x:", addrNext);
        server.sendContent(output);

        uint32_t eraseBuffer[4];
        memset(eraseBuffer, 0xff, sizeof(eraseBuffer));

        for (int i = 0; i < 4; i++)
        {
          //METHOD #2
          //swd.writeBufferSRAM(addrBuffer, eraseBuffer, 1);

          //METHOD #3
          //swd.flashWrite(addrNext, eraseBuffer[i]);

          snprintf(output, sizeof output, " | %02x %02x %02x %02x", (uint8_t)(eraseBuffer[i] >> 0), (uint8_t)(eraseBuffer[i] >> 8), (uint8_t)(eraseBuffer[i] >> 16), (uint8_t)(eraseBuffer[i] >> 24));
          server.sendContent(output);

          addrNext += 4;
          addrBuffer += 4;
        }

        server.sendContent("\n");

        addrIndex++;
      } while (addrNext <= addrEnd);

      //METHOD #2
      //swd.flashloaderRUN(addr, addrBuffer);

      swd.debugHaltOnReset(0);
      swd.debugReset();

    } else {
      server.send(200, "text/plain", "SWD Error");
    }
  });
  server.on("/swd/hex", []() {

    if (swd.begin()) {

      if (server.hasArg("bootloader")) {
        addr = 0x08000000;
        addrEnd = 0x08000fff;
      } else if (server.hasArg("flash")) {
        addr = 0x08001000;
        addrEnd = 0x0801ffff;
      } else if (server.hasArg("ram")) {
        addr = 0x20000000;
        addrEnd = 0x200003ff; //Note: Read is limited to 0x200003ff but you can write to higher portion of RAM
      }
      server.setContentLength(CONTENT_LENGTH_UNKNOWN);
      server.send(200, "text/plain", "");

      uint32_t addrCount = 256;
      uint32_t addrNext = addr;
      do {

        //Serial.printf("------ %08x ------\n", addrNext);

        StreamString data;
        swd.hexDump(addrNext, addrCount, data);
        server.sendContent(data.readString());

        addrNext += (addrCount * 4); //step = count * 4 bytes in int32 word
      } while (addrNext <= addrEnd);

    } else {
      server.send(200, "text/plain", "SWD Error");
    }
  });
  server.on("/swd/bin", []() {

    if (swd.begin()) {

      String filename = "flash.bin";

      if (server.hasArg("bootloader")) {
        addr = 0x08000000;
        addrEnd = 0x08000fff;
        filename = "bootloader.bin";
      } else if (server.hasArg("flash")) {
        addr = 0x08001000;
        addrEnd = 0x0801ffff;
      }
      
      server.sendHeader("Content-Disposition", "attachment; filename = \"" + filename + "\"");
      server.setContentLength(addrEnd - addr + 1); //CONTENT_LENGTH_UNKNOWN
      server.send(200, "application/octet-stream", "");

      uint32_t addrNext = addr;
      do {

        //Serial.printf("------ %08x ------\n", addrNext);

        //uint8_t* buff;
        //swd.binDump(addrNext, buff);
        //server.sendContent(String((char *)buff));
        
        uint8_t byte;
        swd.memLoadByte(addrNext, byte);
        server.sendContent(String(byte));

        addrNext++;
      } while (addrNext <= addrEnd);

    } else {
      server.send(200, "text/plain", "SWD Error");
    }
  });
  server.on("/swd/mem/flash", []() {

    char output[128];

    if (swd.begin()) {

      if (server.hasArg("file")) {

        if (server.hasArg("bootloader")) {
          addr = 0x08000000;
          addrEnd = 0x08000fff;
        } else if (server.hasArg("flash")) {
          addr = 0x08001000;
          addrEnd = 0x0801ffff;
        }

        String filename = server.arg("file");
        File fs = SPIFFS.open("/" + filename, "r");
        if (fs)
        {
          server.setContentLength(CONTENT_LENGTH_UNKNOWN);
          server.send(200, "text/plain", "");

          swd.debugHalt();
          swd.debugHaltOnReset(1); //reset lock into halt
          swd.reset();
          swd.unlockFlash();

          pinMode(LED_BUILTIN, OUTPUT);

          uint32_t addrNext = addr;
          uint32_t addrIndex = addr;
          uint32_t addrBuffer = 0x00000000;

          while (addrNext < addrEnd && fs.available())
          {
            swd.debugHalt();
            if (addrBuffer == 0x00000000)
            {
              swd.flashloaderSRAM(); //load flashloader to SRAM @ 0x20000000
            }

            digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
            
            uint8_t PAGE_SIZE = 6; //webserver max chunks
            for (uint8_t p = 0; p < PAGE_SIZE; p++)
            {
              //Serial.printf("------ %08x ------\n", addrIndex);
              if (fs.available() == 0)
                break;

              snprintf(output, sizeof output, "%08x:", addrIndex);
              server.sendContent(output);

              for (int i = 0; i < 4; i++)
              {
                if (fs.available() == 0)
                  break;

                char sramBuffer[4];
                fs.readBytes(sramBuffer, 4);
                swd.writeBufferSRAM(addrBuffer, (uint8_t*)sramBuffer, sizeof(sramBuffer)); //append to SRAM after flashloader

                snprintf(output, sizeof output, " | %02x %02x %02x %02x", sramBuffer[0], sramBuffer[1], sramBuffer[2], sramBuffer[3]);
                server.sendContent(output);

                addrIndex += 4;
                addrBuffer += 4;
              }
              server.sendContent("\n");
            }
            swd.flashloaderRUN(addrNext, addrBuffer);
            delay(400); //Must wait for flashloader to finish
 
            addrBuffer = 0x00000000;
            addrNext = addrIndex;
          }

          swd.debugHaltOnReset(0); //no reset halt lock
          swd.reset(); //hard-reset

          fs.close();
          SPIFFS.remove("/" + filename);

          digitalWrite(LED_BUILTIN, HIGH); //OFF
        } else {
          server.send(200, "text/plain", "File Error");
        }
      } else {
        server.send(200, "text/plain", ".bin File Required");
      }
    } else {
      server.send(200, "text/plain", "SWD Error");
    }
  });
  //called when the url is not defined here
  //use it to load content from SPIFFS
  server.onNotFound([](){
    if(!handleFileRead(server.uri()))
    {
      server.sendHeader("Refresh", "6; url=/update");
      server.send(404, "text/plain", "FileNotFound");
    }
  });

  server.begin();
  server.client().setNoDelay(1);
}
 
void loop(void){
  server.handleClient();
  ArduinoOTA.handle();
}
