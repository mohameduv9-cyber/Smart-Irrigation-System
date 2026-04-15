#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "DHT.h"

// --- الإعدادات ---
#define WIFI_SSID       "MOHAMED9727"
#define WIFI_PASSWORD   "MOHAMED22"
#define DATABASE_URL    "https://smart-irrigation-system-23802-default-rtdb.europe-west1.firebasedatabase.app"
#define DATABASE_SECRET "fkFfGOM7xF98uFLX8WztBOSm5pX5DTvr9N98zEuN"

#define DHTPIN   4
#define DHTTYPE  DHT11
#define LDR_PIN  34
#define SOIL_PIN 35
#define PUMP_PIN 26

// حماية المضخة: 30 ثانية عمل كحد أقصى و 60 ثانية تبريد
#define PUMP_MAX_ON_MS   30000UL
#define PUMP_COOLDOWN_MS 60000UL

DHT dht(DHTPIN, DHTTYPE);
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

bool          lastAutoState = false;
unsigned long lastLoop = 0, pumpOnSince = 0, pumpOffSince = 0;
bool          pumpSafetyTrip = false;

// --- دالة التحكم في المضخة مع الحماية ---
bool setPump(bool req) {
  unsigned long now = millis();

  // 1. فحص تخطي الوقت المسموح (30 ثانية)
  if (pumpOnSince && (now - pumpOnSince >= PUMP_MAX_ON_MS)) {
    Serial.println("[PUMP] [SAFETY TRIP] Max runtime exceeded!");
    pumpSafetyTrip = true; 
    pumpOnSince = 0; 
    pumpOffSince = now;
    digitalWrite(PUMP_PIN, LOW);
    Firebase.RTDB.setBool(&fbdo, "/controls/pump", false); // تحديث فيربيز للإغلاق
    return false;
  }

  // 2. فحص فترة التبريد (60 ثانية)
  if (pumpSafetyTrip) {
    unsigned long elapsed = now - pumpOffSince;
    if (elapsed < PUMP_COOLDOWN_MS) {
      Serial.printf("[PUMP] [COOLDOWN] Locked. Wait %lus\n", (PUMP_COOLDOWN_MS - elapsed) / 1000);
      digitalWrite(PUMP_PIN, LOW);
      return false;
    }
    pumpSafetyTrip = false;
    Serial.println("[PUMP] Cooldown complete. Ready.");
  }

  // 3. تنفيذ أمر التشغيل أو الإطفاؤ
  if (req) {
    if (!pumpOnSince) {
      pumpOnSince = now;
      Serial.println("[PUMP] State -> ON");
    }
    digitalWrite(PUMP_PIN, HIGH);
    return true;
  } else {
    if (pumpOnSince) Serial.println("[PUMP] State -> OFF");
    pumpOnSince = 0;
    digitalWrite(PUMP_PIN, LOW);
    return false;
  }
}

void connectWiFi() {
  Serial.print("[WIFI] Connecting");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long t = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - t > 15000) ESP.restart();
    delay(500); Serial.print(".");
  }
  Serial.println("\n[WIFI] Connected.");
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, LOW);

  connectWiFi();

  config.database_url = DATABASE_URL;
  config.signer.tokens.legacy_token = DATABASE_SECRET;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Serial.println("[INIT] Firebase Ready.");
}

void loop() {
  // تحديث البيانات كل 5 ثوانٍ
  if (millis() - lastLoop < 5000) return;
  lastLoop = millis();

  if (WiFi.status() != WL_CONNECTED) { connectWiFi(); return; }

  // قراءة الحساسات
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  int light = map(analogRead(LDR_PIN), 0, 4095, 0, 100);
  int soil = constrain(map(analogRead(SOIL_PIN), 3931, 1800, 0, 100), 0, 100);

  if (Firebase.ready()) {
    bool autoMode = false, pumpReq = false;
    int threshold = 30;

    // جلب الإعدادات من فيربيز
    Firebase.RTDB.getBool(&fbdo, "/controls/auto_irrigation") && (autoMode = fbdo.boolData());
    Firebase.RTDB.getInt(&fbdo, "/settings/moisture_threshold") && (threshold = fbdo.intData());
    Firebase.RTDB.getBool(&fbdo, "/controls/pump") && (pumpReq = fbdo.boolData());

    // منطق التبديل: إذا تم إغلاق الـ Auto فجأة، نطفئ المضخة فوراً
    if (lastAutoState && !autoMode) {
        Serial.println("[CTRL] Auto turned OFF. Resetting pump.");
        pumpReq = false;
        Firebase.RTDB.setBool(&fbdo, "/controls/pump", false);
    }
    lastAutoState = autoMode;

    bool finalPumpState = false;
    if (autoMode) {
      // الوضع الآلي
      finalPumpState = setPump(soil < threshold);
      Firebase.RTDB.setBool(&fbdo, "/controls/pump", finalPumpState);
    } else {
      // الوضع اليدوي (يخضع أيضاً لحماية الـ 30 ثانية)
      finalPumpState = setPump(pumpReq);
    }

    // إرسال البيانات النهائية
    FirebaseJson j;
    j.set("temp", temp); 
    j.set("humidity", hum);
    j.set("light", light); 
    j.set("moisture", soil);
    j.set("pump_state", finalPumpState);
    Firebase.RTDB.setJSON(&fbdo, "/sensor_data", &j);
    
    Serial.printf("[SYS] Soil: %d%% | Pump: %s | Mode: %s\n", 
                  soil, finalPumpState ? "ON" : "OFF", autoMode ? "AUTO" : "MANUAL");
  }
}