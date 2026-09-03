/*
  FSWebServer - Example WebServer with FS backend for esp8266/esp32
  Copyright (c) 2015 Hristo Gochkov. All rights reserved.
  This file is part of the WebServer library for Arduino environment.

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

  upload the contents of the data folder with MkSPIFFS Tool ("ESP32 Sketch Data Upload" in Tools menu in Arduino IDE)
  or you can upload the contents of a folder if you CD in that folder and run the following command:
  for file in `ls -A1`; do curl -F "file=@$PWD/$file" esp32fs.local/edit; done

  access the sample web page at http://esp32fs.local
  edit the page by going to http://esp32fs.local/edit
*/
#include <WiFi.h>
#include <NetworkClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu;
bool mpuConnected = false;   // tracks whether the sensor actually responded

int lastPrint = 0;

const int SAMPLE_COUNT = 200;
float rmsFiltered = 0;

// High-pass filter variables (to remove gravity)
float ax_offset = 0, ay_offset = 0, az_offset = 0;
float alpha = 0.98;  // smoothing factor

// RMS function
float calculateRMS(float* arr, int len) {
  float sumSq = 0;
  for (int i = 0; i < len; i++) {
    sumSq += arr[i] * arr[i];
  }
  return sqrt(sumSq / len);
}

#define FILESYSTEM SPIFFS
// You only need to format the filesystem once
#define FORMAT_FILESYSTEM false
#define DBG_OUTPUT_PORT   Serial

#if FILESYSTEM == FFat
#include <FFat.h>
#endif
#if FILESYSTEM == SPIFFS
#include <SPIFFS.h>
#endif

const char *ssid = "OPPO F25 Pro 5G";
const char *password = "d7gs7wiy";
const char *host = "industrialdashboard";
WebServer server(80);
//holds the current upload
File fsUploadFile;

//format bytes
String formatBytes(size_t bytes) {
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  } else {
    return String(bytes / 1024.0 / 1024.0 / 1024.0) + "GB";
  }
}

String getContentType(String filename) {
  if (server.hasArg("download")) {
    return "application/octet-stream";
  } else if (filename.endsWith(".htm")) {
    return "text/html";
  } else if (filename.endsWith(".html")) {
    return "text/html";
  } else if (filename.endsWith(".css")) {
    return "text/css";
  } else if (filename.endsWith(".js")) {
    return "application/javascript";
  } else if (filename.endsWith(".png")) {
    return "image/png";
  } else if (filename.endsWith(".gif")) {
    return "image/gif";
  } else if (filename.endsWith(".jpg")) {
    return "image/jpeg";
  } else if (filename.endsWith(".ico")) {
    return "image/x-icon";
  } else if (filename.endsWith(".xml")) {
    return "text/xml";
  } else if (filename.endsWith(".pdf")) {
    return "application/x-pdf";
  } else if (filename.endsWith(".zip")) {
    return "application/x-zip";
  } else if (filename.endsWith(".gz")) {
    return "application/x-gzip";
  }
  return "text/plain";
}

bool exists(String path) {
  bool yes = false;
  File file = FILESYSTEM.open(path, "r");
  if (!file.isDirectory()) {
    yes = true;
  }
  file.close();
  return yes;
}

bool handleFileRead(String path) {
  DBG_OUTPUT_PORT.println("handleFileRead: " + path);
  if (path.endsWith("/")) {
    path += "index.html";
  }
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if (exists(pathWithGz) || exists(path)) {
    if (exists(pathWithGz)) {
      path += ".gz";
    }
    File file = FILESYSTEM.open(path, "r");
    server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

void handleFileUpload() {
  if (server.uri() != "/edit") {
    return;
  }
  HTTPUpload &upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    if (!filename.startsWith("/")) {
      filename = "/" + filename;
    }
    DBG_OUTPUT_PORT.print("handleFileUpload Name: ");
    DBG_OUTPUT_PORT.println(filename);
    fsUploadFile = FILESYSTEM.open(filename, "w");
    filename = String();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    //DBG_OUTPUT_PORT.print("handleFileUpload Data: "); DBG_OUTPUT_PORT.println(upload.currentSize);
    if (fsUploadFile) {
      fsUploadFile.write(upload.buf, upload.currentSize);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile) {
      fsUploadFile.close();
    }
    DBG_OUTPUT_PORT.print("handleFileUpload Size: ");
    DBG_OUTPUT_PORT.println(upload.totalSize);
  }
}

void handleFileDelete() {
  if (server.args() == 0) {
    return server.send(500, "text/plain", "BAD ARGS");
  }
  String path = server.arg(0);
  DBG_OUTPUT_PORT.println("handleFileDelete: " + path);
  if (path == "/") {
    return server.send(500, "text/plain", "BAD PATH");
  }
  if (!exists(path)) {
    return server.send(404, "text/plain", "FileNotFound");
  }
  FILESYSTEM.remove(path);
  server.send(200, "text/plain", "");
  path = String();
}

void handleFileCreate() {
  if (server.args() == 0) {
    return server.send(500, "text/plain", "BAD ARGS");
  }
  String path = server.arg(0);
  DBG_OUTPUT_PORT.println("handleFileCreate: " + path);
  if (path == "/") {
    return server.send(500, "text/plain", "BAD PATH");
  }
  if (exists(path)) {
    return server.send(500, "text/plain", "FILE EXISTS");
  }
  File file = FILESYSTEM.open(path, "w");
  if (file) {
    file.close();
  } else {
    return server.send(500, "text/plain", "CREATE FAILED");
  }
  server.send(200, "text/plain", "");
  path = String();
}

void handleFileList() {
  if (!server.hasArg("dir")) {
    server.send(500, "text/plain", "BAD ARGS");
    return;
  }

  String path = server.arg("dir");
  DBG_OUTPUT_PORT.println("handleFileList: " + path);

  File root = FILESYSTEM.open(path);
  path = String();

  String output = "[";
  if (root.isDirectory()) {
    File file = root.openNextFile();
    while (file) {
      if (output != "[") {
        output += ',';
      }
      output += "{\"type\":\"";
      output += (file.isDirectory()) ? "dir" : "file";
      output += "\",\"name\":\"";
      output += String(file.path()).substring(1);
      output += "\"}";
      file = root.openNextFile();
    }
  }
  output += "]";
  server.send(200, "text/json", output);
}

float vibrationRMS = 0.0000;   // Now holds a SIMULATED value (see loop())
float overloadAmp = 0.0000;      // Replace with ACS712 value

// Works with ACS712 5A, 20A, 30A
// ===============================

const int sensorPin = 34;   // Use ADC1 pins ONLY on ESP32
const float sensitivity = 0.100; // Change this: 5A = 0.185, 20A = 0.100, 30A = 0.066 (V/A)

int offset = 0;  // Zero current offset (calibrated automatically)

const int relayPin = 25; // GPIO pin connected to relay
bool relayState = false;   // ON = true, OFF = false
float transientVoltage = 0.00;  // Replace with your actual sensor reading

// ---------------------------------------
// Calibration: reads sensor with NO load
// ---------------------------------------
void calibrateACS712() {
  long sum = 0;
  const int samples = 1000;

  Serial.println("Calibrating... keep sensor with NO load.");

  for (int i = 0; i < samples; i++) {
    sum += analogRead(sensorPin);
    delay(2);
  }

  offset = sum / samples;
  Serial.print("Calibration complete. Offset = ");
  Serial.println(offset);
}

// ---------------------------------------
// Read RMS current
// ---------------------------------------
float readCurrentACS712() {
  const int samples = 1000;
  long sumSq = 0;

  for (int i = 0; i < samples; i++) {
    int raw = analogRead(sensorPin);
    int value = raw - offset;     // Remove offset
    sumSq += (long)value * value; // Store square of deviation
  }

  float meanSq = (float)sumSq / samples;
  float rms = sqrt(meanSq);

  // Convert ADC value to voltage
  float voltage = (rms * 3.3) / 4095.0;

  // Convert voltage to current using ACS sensitivity
  float current = voltage / sensitivity;

  return current;
}

// ---------------------------------------
// Simulated vibration RMS (MPU6050 workaround)
// Generates a random g-value between 0.6g and 0.8g
// ---------------------------------------
float getSimulatedVibrationRMS() {
  // random(600, 801) gives an integer between 600 and 800 inclusive
  // dividing by 1000.0 converts it to a float between 0.600 and 0.800
  return random(30, 61) / 1000.0;
}

void setup(void) {
  DBG_OUTPUT_PORT.begin(115200);
  DBG_OUTPUT_PORT.print("\n");
  DBG_OUTPUT_PORT.setDebugOutput(true);

  // Setting Relay to default
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH);
  //----------------------------------------------
   
  if (FORMAT_FILESYSTEM) {
    FILESYSTEM.format();
  }
  FILESYSTEM.begin();
  {
    File root = FILESYSTEM.open("/");
    File file = root.openNextFile();
    while (file) {
      String fileName = file.name();
      size_t fileSize = file.size();
      DBG_OUTPUT_PORT.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
      file = root.openNextFile();
    }
    DBG_OUTPUT_PORT.printf("\n");
  }

  Wire.begin(21, 22);  // ESP32 I2C pins

  // ---- MPU6050 init with connection verification ----
  mpu.initialize();
  mpuConnected = mpu.testConnection();

  if (!mpuConnected) {
    Serial.println("MPU6050 not found at 0x68, trying 0x69 (AD0 HIGH)...");
    mpu = MPU6050(0x69);      // retry with alternate address
    mpu.initialize();
    mpuConnected = mpu.testConnection();
  }

  if (mpuConnected) {
    Serial.println("MPU6050 connection successful.");
  } else {
    Serial.println("MPU6050 connection FAILED. Using SIMULATED vibration values (0.6g - 0.8g) instead.");
  }
  // -----------------------------------------------------------

  //WIFI INIT
  DBG_OUTPUT_PORT.printf("Connecting to %s\n", ssid);
  if (String(WiFi.SSID()) != String(ssid)) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
  }

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    DBG_OUTPUT_PORT.print(".");
  }
  DBG_OUTPUT_PORT.println("");
  DBG_OUTPUT_PORT.print("Connected! IP address: ");
  DBG_OUTPUT_PORT.println(WiFi.localIP());

  MDNS.begin(host);
  DBG_OUTPUT_PORT.print("Open http://");
  DBG_OUTPUT_PORT.print(host);
  DBG_OUTPUT_PORT.println(".local to see the file browser");

    // API Endpoint: /data  
  server.on("/data", []() {
    String json = "{";
    json += "\"rms\":" + String(vibrationRMS, 4) + ",";
    json += "\"amp\":" + String(overloadAmp, 2) + ",";
    json += "\"relay\":\"" + String(relayState ? "ON" : "OFF") + "\",";
    json += "\"transient\":" + String(transientVoltage, 2) + ",";
    json += "\"mpu_connected\":" + String(mpuConnected ? "true" : "false"); // lets you see sensor status via API
    json += "}";
    server.send(200, "application/json", json);
  });
  
  server.on("/relay", HTTP_GET, []() {
  if (server.hasArg("state")) {
    String state = server.arg("state");

    if (relayState == false) {
      digitalWrite(relayPin, HIGH);
      relayState = true;
      server.send(200, "text/plain", "Relay ON");
    } 
    else {
      digitalWrite(relayPin, LOW);
      relayState = false;
      server.send(200, "text/plain", "Relay OFF");
    }
  }
});
Serial.println("ACS712 ESP32 Current Sensor");
  calibrateACS712();


  //called when the url is not defined here
  //use it to load content from FILESYSTEM
  server.onNotFound([]() {
    if (!handleFileRead(server.uri())) {
      server.send(404, "text/plain", "FileNotFound");
    }
  });

  //get heap status, analog input value and all GPIO statuses in one json call
  server.on("/all", HTTP_GET, []() {
    String json = "{";
    json += "\"heap\":" + String(ESP.getFreeHeap());
    json += ", \"analog\":" + String(analogRead(A0));
    json += ", \"gpio\":" + String((uint32_t)(0));
    json += "}";
    server.send(200, "text/json", json);
    json = String();
  });
  server.begin();
  DBG_OUTPUT_PORT.println("HTTP server started");
}

void loop(void) {
  server.handleClient();

  if (mpuConnected) {
    // ---- Real MPU6050 reading path (kept for when the sensor is fixed) ----
    int16_t ax_raw, ay_raw, az_raw, gx_raw, gy_raw, gz_raw;

    float samples[SAMPLE_COUNT];

    // Collect samples for RMS
    for (int i = 0; i < SAMPLE_COUNT; i++) {

      // Read raw data
      mpu.getMotion6(&ax_raw, &ay_raw, &az_raw, &gx_raw, &gy_raw, &gz_raw);

      // Convert to g for ±2g (default scale: divide by 16384)
      float ax = ax_raw / 16384.0;
      float ay = ay_raw / 16384.0;
      float az = az_raw / 16384.0;

      // ---- High-pass filter (removes gravity) ----
      ax_offset = alpha * ax_offset + (1 - alpha) * ax;
      ay_offset = alpha * ay_offset + (1 - alpha) * ay;
      az_offset = alpha * az_offset + (1 - alpha) * az;

      float ax_vib = ax - ax_offset;
      float ay_vib = ay - ay_offset;
      float az_vib = az - az_offset;

      // Vector magnitude = vibration intensity
      samples[i] = sqrt(ax_vib * ax_vib + ay_vib * ay_vib + az_vib * az_vib);

      delayMicroseconds(1000); // ~1kHz sampling
    }

    // Compute RMS
    float rms = calculateRMS(samples, SAMPLE_COUNT);

    // Smoothing filter
    rmsFiltered = 0.7 * rmsFiltered + 0.3 * rms;

    vibrationRMS = rmsFiltered;
    Serial.println(vibrationRMS);
  } else {
    // ---- SIMULATED vibration RMS (MPU6050 workaround) ----
    // Random fluctuation between 0.6g and 0.8g
    vibrationRMS = getSimulatedVibrationRMS();
    Serial.print("Simulated RMS: ");
    Serial.println(vibrationRMS, 4);
  }

  delay(50);  // update speed
  delay(2);  //allow the cpu to switch to other tasks
  overloadAmp = readCurrentACS712();
  transientVoltage = random(1, 20) / 1.0;
}
