#include <WiFi.h>
#include <time.h>

// WiFi sozlamalari
#define NTP_SERVER     "pool.ntp.org"
#define UTC_OFFSET     18000 // +5 soat (O‘zbekiston vaqti)
#define UTC_OFFSET_DST 0
#define relay_pin  D4
const char* SSID = "Wokwi-GUEST"; // WiFi SSID
const char* Pass = ""; // WiFi parol

// Global o‘zgaruvchilar
int Hour = 0, Minute = 0, Seconds = 0, Weekday = 0;
String RealTime;

// Qo‘ng‘iroq vaqtlari ro‘yxati
String BellTime[16] = {
  "09:00:00", "10:20:00", 
  "10:30:00", "11:50:00",
  "12:30:00", "13:50:00", 
  "14:00:00", "15:20:00",
  "15:30:00", "16:50:00", 
  "17:16:00", "18:20:00",
  "18:30:00", "19:50:00", 
  "20:00:00", "21:20:00"
};

// Vaqtni String formatida olish
String getFormattedTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "Connection Err";
  }

  // Soat, daqiqa, sekundni formatlash
  String h = timeinfo.tm_hour < 10 ? "0" + String(timeinfo.tm_hour) : String(timeinfo.tm_hour);
  String m = timeinfo.tm_min < 10 ? "0" + String(timeinfo.tm_min) : String(timeinfo.tm_min);
  String s = timeinfo.tm_sec < 10 ? "0" + String(timeinfo.tm_sec) : String(timeinfo.tm_sec);
  
  // Global o‘zgaruvchilarni yangilash
  Hour = timeinfo.tm_hour;
  Minute = timeinfo.tm_min;
  Seconds = timeinfo.tm_sec;
  Weekday = timeinfo.tm_wday; // Hafta kuni (0 = Yakshanba, 1 = Dushanba, ...)

  // Vaqtni HH:MM:SS formatida qaytarish
  return h + ":" + m + ":" + s;
}

// Qo‘ng‘iroqni boshqarish funksiyasi
void RingBell() {
  for (int i = 0; i < 16; i++) {
    if (RealTime == BellTime[i]) {
      Serial.println("Qo‘ng‘iroq vaqti: " + RealTime);
      digitalWrite(relay_pin, HIGH);
      delay(2000);
      digitalWrite(relay_pin, LOW);
      delay(1000);
      digitalWrite(relay_pin, HIGH);
      delay(2000);
      digitalWrite(relay_pin, LOW);
    }
  }
}

void setup() {
  Serial.begin(115200);

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
  struct tm timeinfo; // Persistent struct tm object
  while (!getLocalTime(&timeinfo)) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Vaqt sinxronlandi");
}

void loop() {
  RealTime = getFormattedTime(); // Joriy vaqtni olish
  Serial.println("Joriy vaqt: " + RealTime);
  
  RingBell(); // Qo‘ng‘iroq vaqtlarini tekshirish
  
  delay(1000); // Har sekundda yangilash
}
