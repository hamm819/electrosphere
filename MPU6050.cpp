#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

// Konfigurasi WiFi
const char* ssid = "hahahaham";
const char* password = "Muhammad1lham";

// Konfigurasi Telegram Bot
#define BOT_TOKEN "8412410134:AAHCgCRxnw11_MPhpY-rhhI35tr8IG-V4wo"
#define CHAT_ID "2095463972" // Chat ID Anda

WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

// Konfigurasi Sensor MPU6050
Adafruit_MPU6050 mpu;

// Threshold untuk deteksi getaran
const float VIBRATION_THRESHOLD = 3.0;    // Getaran abnormal
const float CRITICAL_THRESHOLD = 5.0;     // Getaran berbahaya
const float EMERGENCY_THRESHOLD = 7.0;    // Getaran darurat

// Variabel monitoring
unsigned long lastNotificationTime = 0;
const unsigned long NOTIFICATION_COOLDOWN = 30000; // 30 detik cooldown
int vibrationLevel = 0;
bool systemInitialized = false;

// LED dan Buzzer pins
const int BUZZER_PIN = 25;
const int LED_NORMAL_PIN = 26;
const int LED_WARNING_PIN = 27;
const int LED_CRITICAL_PIN = 14;

void setup() {
  Serial.begin(115200);

  // Setup LED dan Buzzer
  pinMode(LED_NORMAL_PIN, OUTPUT);
  pinMode(LED_WARNING_PIN, OUTPUT);
  pinMode(LED_CRITICAL_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // Matikan semua awal
  digitalWrite(LED_NORMAL_PIN, LOW);
  digitalWrite(LED_WARNING_PIN, LOW);
  digitalWrite(LED_CRITICAL_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  // Inisialisasi I2C
  Wire.begin(21, 22);
  Wire.setClock(100000);

  Serial.println("Sistem Monitoring Getaran Sepeda Gym dengan Telegram");
  Serial.println("====================================================");

  // Koneksi WiFi
  connectToWiFi();

  // Inisialisasi Telegram Bot
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Set root certificate for Telegram
  bot.sendMessage(CHAT_ID, "🤖 Sistem Monitoring Sepeda Gym Started\n"
                           "📍 Status: Online\n"
                           "📡 IP: " + WiFi.localIP().toString(), "");

  // Inisialisasi MPU6050
  initializeMPU6050();

  systemInitialized = true;
  digitalWrite(LED_NORMAL_PIN, HIGH);

  Serial.println("Sistem siap monitoring. Kirim /status ke bot untuk info");
}

void loop() {
  // Handle message Telegram
  if (millis() % 3000 == 0) {
    bot.getUpdates(bot.last_message_received + 1);
  }

  // Monitoring getaran
  monitorVibration();

  delay(100); // Sampling rate 10Hz
}

void connectToWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
    // Blink LED selama connecting
    digitalWrite(LED_NORMAL_PIN, !digitalRead(LED_NORMAL_PIN));
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to WiFi!");
    while(1) {
      digitalWrite(LED_CRITICAL_PIN, HIGH);
      delay(500);
      digitalWrite(LED_CRITICAL_PIN, LOW);
      delay(500);
    }
  }
}

void initializeMPU6050() {
  Serial.println("Menginisialisasi MPU6050...");

  if (!mpu.begin()) {
    Serial.println("Gagal menemukan MPU6050!");
    bot.sendMessage(CHAT_ID, "❌ ERROR: Sensor MPU6050 tidak terdeteksi!\n"
                             "Periksa koneksi sensor.", "");
    while(1) {
      digitalWrite(LED_CRITICAL_PIN, !digitalRead(LED_CRITICAL_PIN));
      delay(500);
    }
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  Serial.println("MPU6050 initialized successfully");
  bot.sendMessage(CHAT_ID, "✅ Sensor MPU6050 terdeteksi dan siap monitoring", "");
}

void monitorVibration() {
  sensors_event_t a, g, temp;

  if (mpu.getEvent(&a, &g, &temp)) {
    float vibrationMagnitude = calculateVibrationMagnitude(a);
    int newVibrationLevel = getVibrationLevel(vibrationMagnitude);

    // Update status LED
    updateLEDStatus(newVibrationLevel);

    // Handle notifikasi jika level berubah
    if (newVibrationLevel != vibrationLevel) {
      handleVibrationLevelChange(newVibrationLevel, vibrationMagnitude);
      vibrationLevel = newVibrationLevel;
    }

    // Display data di serial
    displaySerialData(vibrationMagnitude, newVibrationLevel);

  } else {
    Serial.println("Error reading sensor!");
    if (systemInitialized) {
      bot.sendMessage(CHAT_ID, "⚠️ Sensor Error: Gagal membaca data MPU6050", "");
    }
  }
}

float calculateVibrationMagnitude(sensors_event_t a) {
  // Hitung magnitude getaran (minus gravitasi)
  float magnitude = sqrt(a.acceleration.x * a.acceleration.x +
                        a.acceleration.y * a.acceleration.y +
                        a.acceleration.z * a.acceleration.z);
  return abs(magnitude - 9.8); // Kurangi gravitasi
}

int getVibrationLevel(float magnitude) {
  if (magnitude >= EMERGENCY_THRESHOLD) return 3;    // Darurat
  if (magnitude >= CRITICAL_THRESHOLD) return 2;     // Kritis
  if (magnitude >= VIBRATION_THRESHOLD) return 1;    // Warning
  return 0;                                          // Normal
}

void updateLEDStatus(int level) {
  // Reset semua LED
  digitalWrite(LED_NORMAL_PIN, LOW);
  digitalWrite(LED_WARNING_PIN, LOW);
  digitalWrite(LED_CRITICAL_PIN, LOW);

  switch(level) {
    case 0: // Normal
      digitalWrite(LED_NORMAL_PIN, HIGH);
      digitalWrite(BUZZER_PIN, LOW);
      break;
    case 1: // Warning
      digitalWrite(LED_WARNING_PIN, HIGH);
      triggerBuzzer(200, 1);
      break;
    case 2: // Critical
      digitalWrite(LED_CRITICAL_PIN, HIGH);
      triggerBuzzer(500, 2);
      break;
    case 3: // Emergency
      digitalWrite(LED_CRITICAL_PIN, HIGH);
      digitalWrite(LED_WARNING_PIN, HIGH);
      triggerBuzzer(1000, 0); // Continuous
      break;
  }
}

void handleVibrationLevelChange(int newLevel, float magnitude) {
  unsigned long currentTime = millis();

  // Cooldown untuk notifikasi
  if (currentTime - lastNotificationTime < NOTIFICATION_COOLDOWN && newLevel < 3) {
    return;
  }

  String message = "";
  String emoji = "";

  switch(newLevel) {
    case 1: // Warning
      emoji = "⚠️";
      message = emoji + " PERINGATAN: Getaran Abnormal Terdeteksi\n";
      message += "📊 Level: " + String(magnitude, 1) + " m/s²\n";
      message += "💡 Info: Getaran di atas normal, perhatikan kondisi sepeda";
      break;

    case 2: // Critical
      emoji = "🚨";
      message = emoji + " KRITIS: Getaran Berlebihan!\n";
      message += "📊 Level: " + String(magnitude, 1) + " m/s²\n";
      message += "🔧 Tindakan: Periksa komponen sepeda ASAP";
      break;

    case 3: // Emergency
      emoji = "🚨🚨";
      message = emoji + " DARURAT: Getaran Sangat Berbahaya!\n";
      message += "📊 Level: " + String(magnitude, 1) + " m/s²\n";
      message += "🛑 Tindakan: HENTIKAN PENGGUNAAN SEGERA!\n";
      message += "🔧 Perlu inspeksi teknikal mendalam";
      break;

    case 0: // Normal
      emoji = "✅";
      message = emoji + " Kondisi Normal: Getaran kembali normal\n";
      message += "📊 Level: " + String(magnitude, 1) + " m/s²";
      break;
  }

  // Kirim notifikasi Telegram
  if (systemInitialized && message != "") {
    bot.sendMessage(CHAT_ID, message, "");
    lastNotificationTime = currentTime;
  }
}

void triggerBuzzer(int duration, int count) {
  if (count == 0) {
    digitalWrite(BUZZER_PIN, HIGH);
  } else {
    for (int i = 0; i < count; i++) {
      digitalWrite(BUZZER_PIN, HIGH);
      delay(duration);
      digitalWrite(BUZZER_PIN, LOW);
      if (i < count - 1) delay(200);
    }
  }
}

void displaySerialData(float magnitude, int level) {
  static unsigned long lastDisplayTime = 0;

  if (millis() - lastDisplayTime > 2000) {
    Serial.print("Getaran: ");
    Serial.print(magnitude, 2);
    Serial.print(" m/s² | Level: ");

    switch(level) {
      case 0: Serial.println("Normal"); break;
      case 1: Serial.println("Warning"); break;
      case 2: Serial.println("Critical"); break;
      case 3: Serial.println("EMERGENCY"); break;
    }

    lastDisplayTime = millis();
  }
}

// Handle Telegram messages
void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID) {
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }

    String text = bot.messages[i].text;
    String from_name = bot.messages[i].from_name;

    if (text == "/status") {
      String statusMsg = "🏋️ Status Sepeda Gym\n";
      statusMsg += "📶 WiFi: " + String(WiFi.SSID()) + "\n";
      statusMsg += "📡 IP: " + WiFi.localIP().toString() + "\n";
      statusMsg += "🔧 Sensor: " + String(mpu.begin() ? "OK" : "ERROR") + "\n";
      statusMsg += "📊 Level Getaran: " + String(vibrationLevel) + "\n";
      statusMsg += "⏰ Uptime: " + String(millis() / 60000) + " menit";

      bot.sendMessage(CHAT_ID, statusMsg, "");
    }
    else if (text == "/help") {
      String helpMsg = "🤖 Bot Commands:\n";
      helpMsg += "/status - Status sistem\n";
      helpMsg += "/help - Menampilkan bantuan\n";
      helpMsg += "/threshold - Info threshold getaran";

      bot.sendMessage(CHAT_ID, helpMsg, "");
    }
    else if (text == "/threshold") {
      String thresholdMsg = "📊 Threshold Getaran:\n";
      thresholdMsg += "✅ Normal: < " + String(VIBRATION_THRESHOLD, 1) + " m/s²\n";
      thresholdMsg += "⚠️ Warning: ≥ " + String(VIBRATION_THRESHOLD, 1) + " m/s²\n";
      thresholdMsg += "🚨 Critical: ≥ " + String(CRITICAL_THRESHOLD, 1) + " m/s²\n";
      thresholdMsg += "🚨🚨 Emergency: ≥ " + String(EMERGENCY_THRESHOLD, 1) + " m/s²";

      bot.sendMessage(CHAT_ID, thresholdMsg, "");
    }
  }
}