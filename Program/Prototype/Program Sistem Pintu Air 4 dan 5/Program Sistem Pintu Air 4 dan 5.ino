#include <SPI.h>
#include <LoRa.h>

// Pin LoRa
#define SS 10
#define RST 9
#define DIO0 2

// Pin sensor
#define SOIL_MOISTURE_PIN A0
#define TRIG_PIN 6
#define ECHO_PIN 7

// Pin relay untuk pintu air 4 dan 5
#define RELAY4_OPEN 3
#define RELAY4_CLOSE 4
#define RELAY5_OPEN 5
#define RELAY5_CLOSE 8

// Pin tombol
#define BUTTON_PINTU4 A1
#define BUTTON_PINTU5 A2
#define BUTTON_MODE A3 // Tombol untuk mode otomatis/manual

// Pin LED indikator
#define LED_AUTO A4  // LED indikator mode otomatis
#define LED_MANUAL A5 // LED indikator mode manual

#define TINGGI_MAX 10 // Ubah sesuai dengan tinggi maksimum dari dasar ke sensor (cm)

// Batasan otomatisasi pintu air 4
const int OPEN_THRESHOLD = 9;
const int CLOSE_THRESHOLD = 7;
const int HYSTERESIS = 0;

// Status pintu air
bool gate4Open = false;
bool gate5Open = false;
bool autoMode = true; // Default: otomatis aktif untuk pintu air 4

const char* deviceId = "4"; // Identitas Nano 4

void initLoRa() {
  while (!LoRa.begin(433E6)) {
    Serial.println("Inisialisasi LoRa gagal! Mencoba kembali...");
    delay(5000);
  }
  Serial.println("LoRa Berhasil Diinisialisasi");
}

void setup() {
  Serial.begin(9600);
  
  LoRa.setPins(SS, RST, DIO0);
  initLoRa();

  // Konfigurasi pin
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(RELAY4_OPEN, OUTPUT);
  pinMode(RELAY4_CLOSE, OUTPUT);
  pinMode(RELAY5_OPEN, OUTPUT);
  pinMode(RELAY5_CLOSE, OUTPUT);
  pinMode(BUTTON_PINTU4, INPUT_PULLUP);
  pinMode(BUTTON_PINTU5, INPUT_PULLUP);
  pinMode(BUTTON_MODE, INPUT_PULLUP);
  pinMode(LED_AUTO, OUTPUT);
  pinMode(LED_MANUAL, OUTPUT);

  // Pastikan relay mati saat awal
  stopAll();
  updateLED();
}

void loop() {
  checkButtonMode();
  checkButtonPintu4();
  checkButtonPintu5();

   // Baca sensor
  long distance = getWaterHeight();

  Serial.print("jarak : ");
  Serial.print(distance);
  Serial.println("cm");

  // Hitung ketinggian air
  int ketinggian_air = TINGGI_MAX - distance;

  // Pastikan nilai tidak negatif
  if (ketinggian_air < 0) ketinggian_air = 0;

  //int soilMoistureValue = analogRead(SOIL_MOISTURE_PIN);
  int sensorValue = analogRead(SOIL_MOISTURE_PIN);
  int soilMoistureValue  = map(sensorValue, 897, 215, 1, 10);
  soilMoistureValue = constrain(soilMoistureValue, 1, 10); // Pastikan tetap dalam rentang 0-100%

  if (autoMode) {
    if (distance > OPEN_THRESHOLD + HYSTERESIS && !gate4Open) {
      bukaPintu4();
    } else if (distance < CLOSE_THRESHOLD - HYSTERESIS && gate4Open) {
      tutupPintu4();
    }
  }

  //mengirim data melalui LoRa ke ESP32
  String payload = createPayload(soilMoistureValue, ketinggian_air, gate4Open);
  LoRa.beginPacket();
  LoRa.print(payload);
  LoRa.endPacket();
  Serial.println("Sent: " + payload);

  // **Tambahkan pembacaan RSSI dan SNR setelah pengiriman**
  int rssi = LoRa.packetRssi();
  float snr = LoRa.packetSnr();
  
  Serial.print("RSSI: ");
  Serial.print(rssi);
  Serial.print(" dBm | SNR: ");
  Serial.print(snr);
  Serial.println(" dB");
  
  delay(5000);
}

void checkButtonMode() {
  static bool lastState = HIGH;
  bool currentState = digitalRead(BUTTON_MODE);

  if (lastState == HIGH && currentState == LOW) {
    autoMode = !autoMode;
    updateLED();
    if (!autoMode) {
      Serial.println("Mode otomatis: OFF ");
    } else {
      Serial.println("Mode otomatis: ON ");
    }
  }
  lastState = currentState;
}

void updateLED() {
  digitalWrite(LED_AUTO, autoMode);
  digitalWrite(LED_MANUAL, !autoMode);
}

void checkButtonPintu4() {
  static bool lastState = HIGH;
  bool currentState = digitalRead(BUTTON_PINTU4);

  if (lastState == HIGH && currentState == LOW) {
    if (!autoMode) {
      if (gate4Open) {
        tutupPintu4();
      } else {
        bukaPintu4();
      }
    }
  }
  lastState = currentState;
}

void checkButtonPintu5() {
  static bool lastState = HIGH;
  bool currentState = digitalRead(BUTTON_PINTU5);

  if (lastState == HIGH && currentState == LOW) {
    if (gate5Open) {
      tutupPintu5();
    } else {
      bukaPintu5();
    }
  }
  lastState = currentState;
}

long getWaterHeight() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) {
    Serial.println("Sensor error: Tidak ada respons.");
    return -1;
  }
  return duration * 0.034 / 2;
}

String createPayload(int soilMoisture, long waterHeight, bool gateStatus) {
  int rssi = LoRa.packetRssi();  // Tambahkan RSSI ke payload
  float snr = LoRa.packetSnr();  // Tambahkan SNR ke payload
  String status = gateStatus ? "Open" : "Closed";

  return String("{\"device\":\"") + deviceId +
         "\",\"soilMoisture\":" + soilMoisture +
         ",\"waterHeight\":" + waterHeight +
         ",\"gateStatus\":\"" + status + "\"" +
         ",\"RSSI\":" + rssi +
         ",\"SNR\":" + String(snr, 2) + "}";
}

void bukaPintu4() {
  Serial.println("Membuka pintu air...");
  digitalWrite(RELAY4_OPEN, HIGH);
  digitalWrite(RELAY4_CLOSE, LOW);
  delay(9500);
  stopAll();
  gate4Open = true;
  Serial.println("Pintu air 4 dibuka.");
}

void tutupPintu4() {
  Serial.println("Menutup pintu air 4...");
  digitalWrite(RELAY4_OPEN, LOW);
  digitalWrite(RELAY4_CLOSE, HIGH);
  delay(9500);
  stopAll();
  gate4Open = false;
  Serial.println("Pintu air 4 ditutup.");
}

void bukaPintu5() {
  Serial.println("Membuka pintu air 5...");
  digitalWrite(RELAY5_OPEN, HIGH);
  digitalWrite(RELAY5_CLOSE, LOW);
  delay(9500);
  stopAll();
  gate5Open = true;
  Serial.println("Pintu air 5 dibuka.");
}

void tutupPintu5() {
  Serial.println("Menutup pintu air 5...");
  digitalWrite(RELAY5_OPEN, LOW);
  digitalWrite(RELAY5_CLOSE, HIGH);
  delay(9500);
  stopAll();
  gate5Open = false;
  Serial.println("Pintu air 5 ditutup.");
}

void stopAll() {
  Serial.println("Menghentikan motor...");
  digitalWrite(RELAY4_OPEN, LOW);
  digitalWrite(RELAY4_CLOSE, LOW);
  digitalWrite(RELAY5_OPEN, LOW);
  digitalWrite(RELAY5_CLOSE, LOW);
}
