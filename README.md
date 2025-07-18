# 📸 ESP32\_CAM - Mind Sync Smart Glasses Firmware

This repository contains the firmware developed for the **ESP32-CAM** module in the final year project titled: **Mind Sync Smart Glasses: An AI-Powered Memory Recall System**.

---

## 🧑‍💻 Developer Responsibility

**Role:** Embedded Systems Developer
**Contribution:** I was solely responsible for developing and configuring the ESP32-CAM to:

* Automatically capture images every 15 minutes.
* Connect to Wi-Fi networks.
* Transmit image data securely to the backend server via HTTP POST.
* Ensure low-power consumption and stable image capture performance.

---

## 🎯 Objective

To implement an autonomous visual logging system using ESP32-CAM that can run continuously in a wearable form factor and communicate with a cloud backend for AI-based memory recall.

---

## 🔧 Hardware Requirements

* ESP32-CAM (AI-Thinker Module)
* OV2640 Camera
* 3.7V Rechargeable Li-Po Battery (1000 mAh or above)
* FTDI Programmer or Micro USB-to-Serial Adapter
* Smart Glass Frame (custom 3D printed)

---

## 🛠️ Software Requirements

* Arduino IDE 2.0+
* ESP32 Board Package
* Required Libraries: `WiFi.h`, `HTTPClient.h`, `esp_camera.h`

---

## 🚀 Setup Instructions

### Step 1: Install ESP32 Board Support

In Arduino IDE, go to:
**File → Preferences → Additional Board Manager URLs:**

```txt
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
```

Then open Board Manager and install **esp32 by Espressif Systems**.

### Step 2: Select Correct Board

**Board:** ESP32 Wrover Module
**Partition Scheme:** Huge APP (3MB No OTA/1MB SPIFFS)

### Step 3: Upload the Firmware

Edit these lines in `esp32_cam_main.ino`:

```cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* serverUrl = "https://your-server-url/upload";
```

Connect IO0 to GND to enable flashing mode, then upload. After upload, disconnect IO0 and press RESET.

### Step 4: Deployment

* Use mobile hotspot or router to provide Wi-Fi
* ESP32 connects, captures image, and sends it every 15 mins
* Backend receives and stores it for AI processing

---

## 🔁 Key Features

* 📷 Autonomous image capture
* 📡 Wi-Fi transmission via HTTP
* 🔐 Secure communication
* 🔋 Low power design
* 🕒 Time-controlled interval capture

---

## 📤 Image Upload Format

Images are sent as `multipart/form-data` with `Content-Type: image/jpeg`.

---

## 🧪 Testing & Results

* Average interval stability: ±2 seconds
* Successful upload rate (under good Wi-Fi): 95%
* Image quality optimized for 800x600 resolution (JPEG)

---

## 👨‍🎓 Project Info

* Final Year Project 
* Degree: B.Tech in Computer Science & Engineering
* Title: Mind Sync Smart Glasses
* Role: Embedded Systems Developer

---
