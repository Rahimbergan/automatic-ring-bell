#include <ESP8266WiFi.h>
#include <time.h>

// WiFi sozlamalari
#define NTP_SERVER     "pool.ntp.org"
#define UTC_OFFSET     18000 // +5 soat (O‘zbekiston vaqti)
#define UTC_OFFSET_DST 0
#define RELAY_PIN      2 // D4 pin (GPIO 2) ESP8266 D1 Mini uchun

const char* SSID = "Rustamov"; // WiFi SSID
const char* Pass = "12122002"; // WiFi parol

// Global o‘zgaruvchilar
int Hour = 0, Minute = 0, Seconds = 0, Weekday = 0;
char RealTime[9]; // "HH:MM:SS" + null terminator

// Qo‘ng‘iroq vaqtlari ro‘yxati (17 ta, yangi qo‘shilgan: "21:30:00")
const char* BellTime[17] = {
  "09:00:00", "10:20:00", 
  "10:30:00", "11:50:00",
  "12:30:00", "13:50:00", 
  "14:00:00", "15:20:00",
  "15:30:00", "16:50:00", 
  "17:16:00", "18:20:00",
  "18:16:00", "19:50:00", 
  "20:00:00", "21:20:00",
  "21:30:00" // Qo‘shimcha qo‘ng‘iroq, yakshanba ishlamaydi
};

// Qo‘ng‘iroq trigger flaglari (har bir vaqt uchun)
bool bellTriggered[17] = {false};

// Non-blocking relay boshqaruvi uchun o‘zgaruvchilar
unsigned long lastBellTime = 0;
int bellState = 0; // 0: off, 1: on1 (2s), 2: off (1s), 3: on2 (2s), 4: done
int activeBellIndex = -1; // Joriy faol qo‘ng‘iroq indeksi

// Vaqtni char formatida olish
void getFormattedTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    strcpy(RealTime, "Conn Err");
    return;
  }

  // Soat, daqiqa, sekundni formatlash
  snprintf(RealTime, sizeof(RealTime), "%02d:%02d:%02d", 
           timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  
  // Global o‘zgaruvchilarni yangilash
  Hour = timeinfo.tm_hour;
  Minute = timeinfo.tm_min;
  Seconds = timeinfo.tm_sec;
  Weekday = timeinfo.tm_wday; // 0 = Yakshanba, 1 = Dushanba, ...
}

// WiFi ulanishini tekshirish va qayta ulanish
void checkWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi uzildi, qayta ulanmoqda...");
    WiFi.reconnect();
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("WiFi qayta ulandi");
  }
}

// Qo‘ng‘iroqni boshqarish funksiyasi (non-blocking)
void RingBell() {
  // Joriy vaqtni tekshirish va trigger qilish
  for (int i = 0; i < 17; i++) {
    bool isMatch = (strcmp(RealTime, BellTime[i]) == 0);
    if (isMatch && !bellTriggered[i]) {
      // Qo‘shimcha qo‘ng‘iroq (oxirgi indeksi 16) uchun yakshanba cheklovi
      bool allowRing = true;
      if (i == 16 && Weekday == 0) {
        allowRing = false; // Yakshanba ishlamaydi
      }
      
      if (allowRing) {
        Serial.print("Qo‘ng‘iroq vaqti: ");
        Serial.println(BellTime[i]);
        bellTriggered[i] = true; // Trigger qilindi
        activeBellIndex = i;
        bellState = 1; // Birinchi yoqish bosqichi
        digitalWrite(RELAY_PIN, LOW);
        lastBellTime = millis();
      }
    } else if (!isMatch) {
      bellTriggered[i] = false; // Keyingi marta uchun flagni tozalash
    }
  }

  // Non-blocking state machine
  if (bellState > 0) {
    unsigned long currentTime = millis();
    switch (bellState) {
      case 1: // Birinchi yoqish (2 soniya)
        if (currentTime - lastBellTime >= 2000) {
          digitalWrite(RELAY_PIN, HIGH);
          lastBellTime = currentTime;
          bellState = 2;
        }
        break;
      case 2: // Pauza (1 soniya)
        if (currentTime - lastBellTime >= 1000) {
          digitalWrite(RELAY_PIN, LOW);
          lastBellTime = currentTime;
          bellState = 3;
        }
        break;
      case 3: // Ikkinchi yoqish (2 soniya)
        if (currentTime - lastBellTime >= 2000) {
          digitalWrite(RELAY_PIN, HIGH);
          lastBellTime = currentTime;
          bellState = 4;
        }
        break;
      case 4: // Tugadi
        bellState = 0;
        activeBellIndex = -1;
        break;
    }
  }
}

// Restartni tekshirish
void checkRestart() {
  if (strcmp(RealTime, "12:00:00") == 0) {
    Serial.println("Kunlik restart: ESP qayta yuklanmoqda...");
    ESP.restart();
  }
}

void setup() {
  Serial.begin(115200);

  // Relay pinni sozlash
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // Dastlab o‘chirilgan holat

  // WiFi ga ulanish
  Serial.print("WiFi ga ulanmoqda...");
  WiFi.begin(SSID, Pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi ulandi");
  Serial.print("IP manzil: ");
  Serial.println(WiFi.localIP());

  // Vaqtni sozlash
  configTime(UTC_OFFSET, UTC_OFFSET_DST, NTP_SERVER);

  // Vaqt sinxronlanishini kutish
  Serial.print("Vaqt sinxronlanmoqda...");
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Vaqt sinxronlandi");
}

void loop() {
  getFormattedTime(); // Joriy vaqtni olish
  Serial.print("Joriy vaqt: ");
  Serial.println(RealTime);
  
  checkWiFi(); // WiFi holatini tekshirish
  RingBell(); // Qo‘ng‘iroq vaqtlarini tekshirish va boshqarish
  checkRestart(); // Restartni tekshirish
  
  delay(1000); // Har sekundda yangilash (non-critical qismda delay ishlatilgan)
}
