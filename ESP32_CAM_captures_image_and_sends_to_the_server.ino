#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <Arduino.h>
#include <WiFiClientSecure.h>
#include "esp_camera.h"

#define EEPROM_SIZE 128  
#define SSID_ADDR 0
#define PASSWORD_ADDR 32
#define BACKUP_SSID_ADDR 64
#define BACKUP_PASSWORD_ADDR 96

#define LED_PIN 2
#define FLASH_LED_PIN 4    // Change to match your ESP32 LED pin

String defaultBackupSSID = "<Your wifi Name>";     
String defaultBackupPassword = "<wifi Password>";  

// Server details
const char* serverName = "emb-service.onrender.com";  
String serverPath = "/api/v1/feedback/upload";
const int serverPort = 443;  // HTTPS uses port 443

WiFiClientSecure client;

// CAMERA_MODEL_AI_THINKER PINS
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22


volatile unsigned long timerInterval = 120000;  // Default 2 minutes
volatile unsigned long previousMillis = 0;


void fetchCaptureInterval() {
    if (WiFi.status() != WL_CONNECTED) return;

    HTTPClient http;
    http.begin("https://emb-service.onrender.com/api/v1/stats/get-capture-interval");
    http.setTimeout(5000);

    int httpResponseCode = http.GET();
    if (httpResponseCode == 200) {
        String response = http.getString();
        Serial.println("📡 Interval API Response: " + response);

        // ✅ Increase buffer size if needed
        DynamicJsonDocument doc(512);
        DeserializationError error = deserializeJson(doc, response);
        
        if (error) {
            Serial.print("❌ JSON Parsing failed: ");
            Serial.println(error.f_str());  // Print detailed error message
            return;
        }

        if (!doc.containsKey("data") || !doc["data"].containsKey("interval")) {
            Serial.println("❌ Invalid JSON format: Missing 'data' or 'interval' key.");
            return;
        }

        // ✅ Extract and update the interval
        int newInterval = doc["data"]["interval"];
        Serial.print("✅ Updated capture interval: ");
        Serial.println(newInterval);

        // Update timerInterval dynamically
        timerInterval = newInterval;
    } else {
        Serial.print("❌ HTTP Error: ");
        Serial.println(httpResponseCode);
    }

    http.end();
}


void connectToWiFi(const char* ssid, const char* password);
bool fetchWiFiFromServer();
void resetEEPROM();
String readEEPROM(int start, int length);
void writeEEPROM(int start, const String &data, int length);
void initializeEEPROM();
void blinkLED(int times, int delayTime);

void setup() {
    Serial.begin(115200);
        pinMode(FLASH_LED_PIN, OUTPUT);  // ✅ Set flash LED pin as output
    digitalWrite(FLASH_LED_PIN, LOW);  // Ensure it's off initially
    pinMode(LED_PIN, OUTPUT);
    EEPROM.begin(EEPROM_SIZE);

    initializeEEPROM();

    String storedSSID = readEEPROM(SSID_ADDR, 32);
    String storedPassword = readEEPROM(PASSWORD_ADDR, 32);
    String backupSSID = readEEPROM(BACKUP_SSID_ADDR, 32);
    String backupPassword = readEEPROM(BACKUP_PASSWORD_ADDR, 32);


    // Try connecting to stored WiFi first
    if (storedSSID.length() > 0 && storedPassword.length() > 0) {
        Serial.println("📶 Connecting to stored WiFi: " + storedSSID);
        connectToWiFi(storedSSID.c_str(), storedPassword.c_str());
    }

    // If stored WiFi fails, use backup WiFi
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("⚠️ Stored WiFi failed! Switching to backup WiFi...");
        connectToWiFi(backupSSID.c_str(), backupPassword.c_str());
    }

    // If still not connected, restart ESP
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("❌ No WiFi connection available! Retrying in 30 seconds...");
        delay(30000);
        ESP.restart();
    }

    // Fetch new WiFi credentials from server
    if (fetchWiFiFromServer()) {
        Serial.println("✅ New WiFi credentials saved. Restarting ESP...");
        delay(2000);
        ESP.restart();
    } else {
        Serial.println("🔄 No new WiFi credentials. Continuing...");
    }

        if (WiFi.status() == WL_CONNECTED) {
        fetchCaptureInterval();  // ✅ Fetch the interval at startup
    }

      client.setInsecure();  

  // Initialize Camera
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 10;  
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_CIF;
    config.jpeg_quality = 12;  
    config.fb_count = 1;
  }

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera init failed!");
    delay(1000);
    ESP.restart();
  }

  sendPhoto(); 
}

void loop() {
    // Your main code logic here
      unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= timerInterval) {
    sendPhoto();
    previousMillis = currentMillis;

     // Optionally fetch the interval again every 10 cycles
     static int cycleCount = 0;
        if (++cycleCount % 10 == 0) {  
            fetchCaptureInterval();
        }
  }
}

// Function to blink LED while connecting
void blinkLED(int times, int delayTime) {
    for (int i = 0; i < times; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(delayTime);
        digitalWrite(LED_PIN, LOW);
        delay(delayTime);
    }
}

// Function to connect to WiFi
void connectToWiFi(const char* ssid, const char* password) {
    Serial.print("🔗 Connecting to WiFi: ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);
    int attempts = 0;

    while (WiFi.status() != WL_CONNECTED && attempts < 15) {
        blinkLED(1, 500); // Blink LED while connecting
        Serial.print(".");
        delay(1000);
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✅ Connected to WiFi!");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\n❌ Failed to connect to WiFi!");
    }
}


// Function to fetch WiFi credentials from server
bool fetchWiFiFromServer() {
    if (WiFi.status() != WL_CONNECTED) return false;

    HTTPClient http;
    http.begin("https://emb-service.onrender.com/api/v1/stats/get-wifi");  
    http.setTimeout(5000);
    int httpResponseCode = http.GET();

    if (httpResponseCode == 200) {
        String response = http.getString();
        Serial.println("📡 Server Response: " + response);

        DynamicJsonDocument doc(256);
        DeserializationError error = deserializeJson(doc, response);
        if (error) {
            Serial.println("❌ JSON Parsing failed!");
            return false;
        }

        if (!doc.containsKey("data") || !doc["data"].containsKey("ssid") || !doc["data"].containsKey("password")) {
            Serial.println("❌ Invalid JSON format.");
            return false;
        }

        String newSSID = doc["data"]["ssid"].as<String>();
        String newPassword = doc["data"]["password"].as<String>();
        String currentSSID = readEEPROM(SSID_ADDR, 32);
        String currentPassword = readEEPROM(PASSWORD_ADDR, 32);

        if (newSSID.length() > 0 && newPassword.length() > 0 && 
            (newSSID != currentSSID || newPassword != currentPassword)) { 

            Serial.println("📶 New WiFi SSID: " + newSSID);
            Serial.println("🔍 Testing new WiFi credentials...");
            connectToWiFi(newSSID.c_str(), newPassword.c_str());

            if (WiFi.status() == WL_CONNECTED) {
                Serial.println("✅ New WiFi works! Updating EEPROM...");

                // Move current WiFi to backup before saving new WiFi
                writeEEPROM(BACKUP_SSID_ADDR, currentSSID, 32);
                writeEEPROM(BACKUP_PASSWORD_ADDR, currentPassword, 32);

                // Reset EEPROM and save new WiFi
                resetEEPROM();
                writeEEPROM(SSID_ADDR, newSSID, 32);
                writeEEPROM(PASSWORD_ADDR, newPassword, 32);
                EEPROM.commit();

                return true;
            } else {
                Serial.println("❌ New WiFi failed! Keeping current WiFi.");
            }
        } else {
            Serial.println("❌ No changes in WiFi credentials.");
        }
    } else {
        Serial.print("❌ HTTP Error: ");
        Serial.println(httpResponseCode);
    }

    http.end();
    return false;
}

// Function to initialize EEPROM if empty
void initializeEEPROM() {
    if (readEEPROM(SSID_ADDR, 32).length() == 0) {
        Serial.println("🛠 First boot detected. Storing default backup WiFi...");
        writeEEPROM(BACKUP_SSID_ADDR, defaultBackupSSID, 32);
        writeEEPROM(BACKUP_PASSWORD_ADDR, defaultBackupPassword, 32);
        EEPROM.commit();
        Serial.println("✅ Default backup WiFi saved.");
    }
}

// Function to reset EEPROM
void resetEEPROM() {
    for (int i = 0; i < EEPROM_SIZE; i++) {
        EEPROM.write(i, 0);
    }
    EEPROM.commit();
    Serial.println("🧹 EEPROM Reset Completed.");
}

// Function to read data from EEPROM
String readEEPROM(int start, int length) {
    String value = "";
    for (int i = start; i < start + length; i++) {
        char c = EEPROM.read(i);
        if (c == 0xFF || c == 0) break;
        value += c;
    }
    value.trim();
    return value.length() > 0 ? value : "";
}

// Function to write data to EEPROM
void writeEEPROM(int start, const String &data, int length) {
    for (int i = 0; i < length; i++) {
        EEPROM.write(start + i, (i < data.length()) ? data[i] : 0);
    }
}
String sendPhoto() {
   Serial.println("Turning on flash...");
  digitalWrite(FLASH_LED_PIN, HIGH);  // Turn on flash before capturing
  Serial.println("Capturing image...");
  camera_fb_t* fb = esp_camera_fb_get();

  if (!fb) {
    Serial.println("Camera capture failed");
    digitalWrite(FLASH_LED_PIN, LOW);  // Turn off flash if capture fails
    delay(1000);
    ESP.restart();
    return "";
  }
    Serial.println("Turning off flash...");
  digitalWrite(FLASH_LED_PIN, LOW);  // Turn off flash after capture

  Serial.println("Connecting to server: " + String(serverName));

  client.setTimeout(20000);  // ✅ Increase timeout to 10 seconds
  if (!client.connect(serverName, serverPort)) {
    Serial.println("Connection failed!");
    return "";
  }

  Serial.println("Connected to server.");

  String boundary = "----ESP32CamBoundary";
  String head = "--" + boundary + "\r\n" +
                "Content-Disposition: form-data; name=\"image\"; filename=\"esp32-cam.jpg\"\r\n" +
                "Content-Type: image/jpeg\r\n\r\n";
  String tail = "\r\n--" + boundary + "--\r\n";

  uint16_t imageLen = fb->len;
  uint16_t totalLen = imageLen + head.length() + tail.length();

  client.println("POST " + serverPath + " HTTP/1.1");
  client.println("Host: " + String(serverName));
  client.println("Content-Length: " + String(totalLen));
  client.println("Content-Type: multipart/form-data; boundary=" + boundary);
  client.println();
  client.print(head);

  uint8_t* fbBuf = fb->buf;
  size_t fbLen = fb->len;
  for (size_t n = 0; n < fbLen; n += 1024) {
    if (n + 1024 < fbLen) {
      client.write(fbBuf, 1024);
      fbBuf += 1024;
    } else {
      size_t remainder = fbLen % 1024;
      client.write(fbBuf, remainder);
    }
  }
  
  client.print(tail);
  esp_camera_fb_return(fb);
  
  Serial.println("Image sent, waiting for response...");
  
  String response = "";
  unsigned long startTime = millis();
  
  while ((millis() - startTime) < 10000) {  // ✅ Wait up to 10 sec for response
    while (client.available()) {
      response += client.readStringUntil('\n');
    }
  }

  Serial.println("Server response: " + response);
  client.stop();
  
  return response;
}

