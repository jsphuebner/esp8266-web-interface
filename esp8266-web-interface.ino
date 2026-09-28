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
bool fastUartAvailable = false;

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
  if (!server.hasArg("cmd")) {
    server.send(500, "text/plain", "BAD ARGS");
    return;
  }

  String cmd = server.arg("cmd").substring(0, cmdBufSize);
  int repeat = 0;
  if (server.hasArg("repeat"))
    repeat = server.arg("repeat").toInt();

  // Stream directly to TCP client using HTTP "Connection: close" (Streaming to Close)
  // Bypasses RAM buffering completely, allowing arbitrary-sized payloads (50KB+) without OOM
  WiFiClient client = server.client();

  client.print(
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: application/json\r\n"
    "Access-Control-Allow-Origin: *\r\n"
    "Connection: close\r\n"
    "\r\n"
  );

  sendCommand(cmd);

  // Buffer incoming serial bytes into full TCP packets (MSS size: 1460 bytes)
  // to avoid exhausting LwIP TCP PCB segments and prevent network choking
  const size_t TX_BUF_SIZE = 1460;
  char txBuffer[TX_BUF_SIZE];
  size_t txLen = 0;

  bool isJson = cmd.startsWith("json");
  bool jsonStarted = false;
  int jsonDepth = 0;
  bool inQuotes = false;
  bool escapeNext = false;
  bool complete = false;
  bool done = false;

  unsigned long startWait = millis();
  while (!Serial.available() && (millis() - startWait < 1500))
  {
    delay(1);
  }

  unsigned long lastDataTime = millis();

  while (!done)
  {
    int avail = Serial.available();
    if (avail > 0)
    {
      size_t space = TX_BUF_SIZE - txLen;
      int toRead = avail < (int)space ? avail : (int)space;
      int bytesRead = Serial.readBytes(&txBuffer[txLen], toRead);
      if (bytesRead > 0)
      {
        lastDataTime = millis();

        if (isJson && !complete)
        {
          for (int i = 0; i < bytesRead; i++)
          {
            char c = txBuffer[txLen + i];
            if (escapeNext)
            {
              escapeNext = false;
            }
            else if (c == '\\')
            {
              if (inQuotes) escapeNext = true;
            }
            else if (c == '"')
            {
              inQuotes = !inQuotes;
            }
            else if (!inQuotes)
            {
              if (c == '{')
              {
                jsonDepth++;
                jsonStarted = true;
              }
              else if (c == '}')
              {
                jsonDepth--;
                if (jsonStarted && jsonDepth == 0)
                {
                  complete = true;
                  break;
                }
              }
            }
          }
        }

        txLen += bytesRead;

        if (txLen >= TX_BUF_SIZE)
        {
          client.write((const uint8_t*)txBuffer, txLen);
          txLen = 0;
          yield();
        }
      }
    }
    else
    {
      // Idle serial: flush partial buffer if pending for >= 10ms
      if (txLen > 0 && (millis() - lastDataTime >= 10))
      {
        client.write((const uint8_t*)txBuffer, txLen);
        txLen = 0;
        yield();
      }

      if (repeat > 0)
      {
        if (millis() - lastDataTime >= 20)
        {
          repeat--;
          Serial.print("!");
          char discard[1];
          Serial.readBytes(discard, 1); // consume "!"
          lastDataTime = millis();
        }
      }
      else if (complete)
      {
        // JSON root closed: brief 20ms window to drain any trailing \r\n, then done
        if (millis() - lastDataTime >= 20)
        {
          done = true;
          break;
        }
      }
      else
      {
        // Timeout: wait longer for JSON (800ms) to ensure slow parameter iterations are not cut off
        unsigned long timeout = isJson ? 800 : 250;
        if (millis() - lastDataTime >= timeout)
        {
          done = true;
          break;
        }
      }

      delay(1);
    }
  }

  // Flush any final remaining bytes
  if (txLen > 0)
  {
    client.write((const uint8_t*)txBuffer, txLen);
    txLen = 0;
  }

  client.flush();
  client.stop();
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


static int waitForBootloaderChar(const char* targetChars, unsigned long timeoutMs)
{
  unsigned long start = millis();
  while (millis() - start < timeoutMs)
  {
    if (Serial.available() > 0)
    {
      int c = Serial.read();
      if (c >= 0)
      {
        for (const char* p = targetChars; *p; p++)
        {
          if ((char)c == *p)
            return c;
        }
      }
    }
    delay(1);
  }
  return -1;
}

static void handleUpdate()
{
  if (!server.hasArg("step") || !server.hasArg("file"))
  {
    server.send(500, "application/json", "{\"error\": \"BAD ARGS\"}");
    return;
  }

  size_t PAGE_SIZE_BYTES = 1024;
  int step = server.arg("step").toInt();
  String filepath = server.arg("file");
  File file = SPIFFS.open(filepath, "r");
  if (!file)
  {
    server.send(404, "application/json", "{\"error\": \"File not found on device: " + filepath + "\", \"pages\": 0}");
    return;
  }

  int pages = (file.size() + PAGE_SIZE_BYTES - 1) / PAGE_SIZE_BYTES;
  String message;

  if (server.hasArg("pagesize"))
  {
    PAGE_SIZE_BYTES = server.arg("pagesize").toInt();
  }

  if (step == -1)
  {
    // The STM32 bootloader USART is configured for 115200 8-N-2 (2 stop bits)
    Serial.begin(115200, SERIAL_8N2);
    delay(10);

    // Flush any pending data in serial buffer
    while (Serial.available()) Serial.read();

    // Send reset command directly (do NOT use sendCommand which waits for a \n echo!)
    Serial.print("reset\r\n");

    // Wait for bootloader version: '2' (v2) or 'S' (v1)
    int ver = waitForBootloaderChar("2S", 3000);
    if (ver < 0)
    {
      server.send(500, "application/json", "{\"error\": \"Bootloader did not respond after reset (no '2' or 'S' received)\", \"pages\": 0}");
      file.close();
      return;
    }

    if (ver == '2') // Version 2 bootloader
    {
      Serial.write(0xAA); // Send magic byte
      if (waitForBootloaderChar("S", 1500) < 0)
      {
        server.send(500, "application/json", "{\"error\": \"Timeout waiting for 'S' after sending magic byte\", \"pages\": 0}");
        file.close();
        return;
      }
    }

    // Send number of pages
    Serial.write((uint8_t)pages);

    // Bootloader acknowledges and requests page 0 with 'P'
    if (waitForBootloaderChar("P", 2000) < 0)
    {
      server.send(500, "application/json", "{\"error\": \"Timeout waiting for 'P' after sending page count\", \"pages\": 0}");
      file.close();
      return;
    }

    message = "reset";
  }
  else
  {
    file.seek(step * PAGE_SIZE_BYTES);
    char buffer[PAGE_SIZE_BYTES];
    size_t bytesRead = file.readBytes(buffer, sizeof(buffer));

    while (bytesRead < PAGE_SIZE_BYTES)
      buffer[bytesRead++] = 0xff;

    uint32_t crc = crc32((uint32_t*)buffer, PAGE_SIZE_BYTES / 4, 0xffffffff);

    // Flush any stale bytes
    while (Serial.available()) Serial.read();

    // Send page buffer (1024 bytes)
    Serial.write((const uint8_t*)buffer, sizeof(buffer));

    // Wait for bootloader to request CRC ('C') or error ('T'/'E')
    int res = waitForBootloaderChar("CTE", 2500);

    if (res == 'C')
    {
      // Send 4-byte CRC32 (little endian)
      Serial.write((const uint8_t*)&crc, sizeof(uint32_t));

      // Wait for page result: 'P' (success, next page), 'D' (update done), 'E' (crc error), 'T' (timeout)
      res = waitForBootloaderChar("PDET", 3000);
    }

    if (res == 'P')
    {
      message = "Page write success";
    }
    else if (res == 'D')
    {
      message = "Update Done";
      // Restore normal 8N1 serial for regular operation
      Serial.begin(115200, SERIAL_8N1);
    }
    else if (res == 'E')
    {
      server.send(500, "application/json", "{\"error\": \"CRC verification error on page " + String(step) + "\", \"pages\": " + String(pages) + "}");
      file.close();
      return;
    }
    else
    {
      server.send(500, "application/json", "{\"error\": \"Timeout or sync error on page " + String(step) + " (code: " + String((char)(res > 0 ? res : '?')) + ")\", \"pages\": " + String(pages) + "}");
      file.close();
      return;
    }
  }

  server.send(200, "application/json", "{\"message\": \"" + message + "\", \"pages\": " + pages + ", \"step\": " + step + "}");
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
  Serial.setRxBufferSize(8192);
  Serial.begin(115200);
  Serial.setTimeout(100);
  SPIFFS.begin();

  //WIFI INIT
  #ifdef WIFI_IS_OFF_AT_BOOT
    enableWiFiAtBootTime();
  #endif
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin();
  sta_tick.attach(10, staCheck);
  
  MDNS.begin(host);

  updater.setup(&server);
  
  //SERVER INIT
  ArduinoOTA.setHostname(host);
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

      server.sendContent(""); //end stream

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
        //addrEnd = 0x080011ff; //Quick Debug
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

      server.sendContent(""); //end stream

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

      server.sendContent(""); //end stream

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

          server.sendContent(""); //end stream
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

  MDNS.addService("http", "tcp", 80);
}
 
void loop(void){
  server.handleClient();
  ArduinoOTA.handle();
  // note: ArduinoOTA.handle() calls MDNS.update();
}
