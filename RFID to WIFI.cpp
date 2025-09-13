#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>

// --- CONFIG: ganti sesuai ---
const char* WIFI_SSID     = "Bismillah";
const char* WIFI_PASSWORD = "1234567890";
const char* SCRIPT_URL    = "https://script.google.com/macros/s/AKfycbzakN5A325_3T4RV3LMFsjHXwUIHtII4xyYK_J5sG6HjNSHbLX85eJojM4SZSc--mjf/exec";
const char* TOKEN         = "12345";
// --------------------------------

// Pin (contoh untuk ESP32 - sesuaikan jika memakai board lain)
#define RST_PIN    0
#define SS_PIN     5

MFRC522 mfrc522(SS_PIN, RST_PIN);

String lastUid = "";
unsigned long lastTapMillis = 0;
const unsigned long debounceMs = 1500; // abaikan taps yang terlalu dekat (ms)

void setup() {
  Serial.begin(115200);
  SPI.begin();                 // SCK, MOSI, MISO default pins ESP32
  mfrc522.PCD_Init();
  Serial.println("RFID reader ready.");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected.");
  } else {
    Serial.println("\nWiFi NOT connected. You can still read UID locally.");
  }
}

String uidToHex() {
  String s = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) s += "0";
    s += String(mfrc522.uid.uidByte[i], HEX);
  }
  s.toUpperCase();
  return s;
}

void sendUidToServer(const String &uid) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, cannot send.");
    return;
  }
  HTTPClient http;
  http.begin(SCRIPT_URL);
  http.addHeader("Content-Type", "application/json");
  String payload = "{\"uid\":\"" + uid + "\",\"token\":\"" + String(TOKEN) + "\"}";
  int httpCode = http.POST(payload);
  if (httpCode > 0) {
    String response = http.getString();
    Serial.printf("HTTP %d: %s\n", httpCode, response.c_str());
  } else {
    Serial.printf("HTTP POST failed, error: %d\n", httpCode);
  }
  http.end();
}

void loop() {
  // cek apakah ada kartu hadir
  if (!mfrc522.PICC_IsNewCardPresent()) {
    delay(120);
    return;
  }
  if (!mfrc522.PICC_ReadCardSerial()) {
    delay(120);
    return;
  }
  String uid = uidToHex();
  unsigned long now = millis();
  if (uid == lastUid && (now - lastTapMillis) < debounceMs) {
    // abaikan bounce / double-read singkat
    mfrc522.PICC_HaltA();
    return;
  }
  lastUid = uid;
  lastTapMillis = now;

  Serial.println("Tag read: " + uid);
  sendUidToServer(uid);

  mfrc522.PICC_HaltA();
  delay(250);
}