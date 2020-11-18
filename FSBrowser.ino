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

//swd over esp8266
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
  DBG_OUTPUT_PORT.println("handleFileCreate: " + path);
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
  Serial.print("\n");
  delay(1);
  while(Serial.available())
    Serial.read(); //flush all previous output
  Serial.print(cmd);
  Serial.print("\n");
  Serial.readStringUntil('\n'); //consume echo  
}

static void handleCommand() {
  const int cmdBufSize = 128;
  if(!server.hasArg("cmd")) {server.send(500, "text/plain", "BAD ARGS"); return;}

  String cmd = server.arg("cmd").substring(0, cmdBufSize);
  int repeat = 0;
  char buffer[255];
  size_t len = 0;
  String output;

  if (server.hasArg("repeat"))
    repeat = server.arg("repeat").toInt();

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

  sendCommand(cmd);
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
  server.sendHeader("Access-Control-Allow-Origin","*");
  server.send(200, "text/json", output);
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
    sendCommand("reset");

    if (fastUart)
    {
      Serial.begin(115200);
      fastUart = false;
      fastUartAvailable = true; //retry after reboot
    }
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

  //WIFI INIT
  WiFi.mode(WIFI_AP_STA);
  WiFi.setPhyMode(WIFI_PHY_MODE_11B);
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
      server.send(200, "text/json", String(output));

    } else {
      server.send(200, "text/json", "{\"connected\": false}");
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
    snprintf(output, sizeof output, "{\"uid\": \"0x%02x-0x%02x-0x%04x-0x%04x\" }", off0, off2, off4, off8);
    server.send(200, "text/json", String(output));
  });
  server.on("/swd/zero", []() {

    if (swd.begin()) {

      server.setContentLength(CONTENT_LENGTH_UNKNOWN);
      server.send(200, "text/plain", "");

      //Testing
      addr = 0x08000000;
      addrEnd = 0x08000010;

      // Before programming internal SRAM, the ARM Cortex-M3 should first be reset and halted.
      swd.debugHalt();

      swd.debugHaltOnReset(1);
      
      swd.debugReset();

      /*
         https://www.silabs.com/community/mcu/32-bit/knowledge-base.entry.html/2014/10/21/how_to_program_inter-esAv
         The debug and system registers and one Silicon Labs-specific AP register CHIPAP_CTRL1 are used for this purpose.
         CHIPAP_CTRL1 address = 0x1, APSEL = 0x0A. Bit 3 core_reset_ap should be written to 1 to hold the Cortex-M3 core in reset.

         The process is as follows:

          Write 0x08 to CHIPAP_CTRL1.
          Write 0xA05F0001 to DHCSR, which enables debug halt.
          Write 0x01 to DEMCR. This enables Reset Vector Catch.
          Write 0x05FA0004 to AIRCR. This resets the core.
          Write 0x00 to CHIPAP_CTRL1.
      */

      uint32_t addrNext = addr;
      do {

        //Serial.printf("------ %08x ------\n", addrNext);

        swd.memStoreByte(addr, 0xff);

        char output[128];
        snprintf(output, sizeof output, "%08x\n", addrNext);
        server.sendContent(output);

        yield(); //Prevent Reset by Watch-Dog

        addrNext++;
      } while (addrNext <= addrEnd);

      server.sendContent(""); //end stream

    } else {
      server.send(200, "text/plain", "SWD Error");
    }
  });
  server.on("/swd/hex", []() {
    uint32_t idcode;
    if (swd.begin() && swd.getIDCODE(idcode)) {

      if (server.hasArg("bootloader")) {
        addr = 0x08000000;
        addrEnd = 0x08000fff;
      } else if (server.hasArg("flash")) {
        addr = 0x08001000;
        addrEnd = 0x0801ffff;
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

        yield(); //Prevent Reset by Watch-Dog

        addrNext += (addrCount * 4); //step = count * 4 bytes in int32 word
      } while (addrNext <= addrEnd);

      server.sendContent(""); //end stream

    } else {
      server.send(200, "text/plain", "SWD Error");
    }
  });
  server.on("/swd/bin", []() {

    uint32_t idcode;
    if (swd.begin() && swd.getIDCODE(idcode)) {

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
      server.setContentLength(CONTENT_LENGTH_UNKNOWN);
      server.send(200, "application/octet-stream", "");

      uint32_t addrNext = addr;
      do {

        //Serial.printf("------ %08x ------\n", addrNext);

        uint8_t* buff;
        swd.binDump(addrNext, buff);

        char output[sizeof(buff) / 4];
        strcpy(output, (const char*)buff);
        server.sendContent(output, sizeof(output));

        yield(); //Prevent Reset by Watch-Dog

        addrNext++;
      } while (addrNext <= addrEnd);

      server.sendContent(""); //end stream

    } else {
      server.send(200, "text/plain", "SWD Error");
    }
  });
  server.on("/swd/mem/flash", []() {

    uint32_t idcode;
    if (swd.begin() && swd.getIDCODE(idcode)) {

      if (server.hasArg("file")) {

        if (server.hasArg("bootloader")) {
          addr = 0x08000000;
          addrEnd = 0x08000fff;
        } else if (server.hasArg("flash")) {
          addr = 0x08001000;
          addrEnd = 0x0801ffff;
        }
        
        swd.debugHalt();
        
        server.setContentLength(CONTENT_LENGTH_UNKNOWN);
        server.send(200, "text/plain", "");

        File file = SPIFFS.open(server.arg("file"), "r");

        uint32_t addrNext = addr;
        do {

          //Serial.printf("------ %08x ------\n", addrNext);

          swd.memStoreByte(addr, file.read());

          char output[128];
          snprintf(output, sizeof output, "%08x", addrNext);
          server.sendContent(output);

          yield(); //Prevent Reset by Watch-Dog

          addrNext++;
        } while (addrNext <= addrEnd);

        file.close();

        server.sendContent(""); //end stream

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
      server.send(404, "text/plain", "FileNotFound");
  });

  server.begin();
  server.client().setNoDelay(1);
}
 
void loop(void){
  server.handleClient();
  ArduinoOTA.handle();
}
