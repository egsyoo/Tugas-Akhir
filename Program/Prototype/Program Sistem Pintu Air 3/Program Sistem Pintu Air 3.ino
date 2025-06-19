#include <SPI.h>
#include <LoRa.h>

// Pin untuk LoRa
#define SS 10
#define RST 9
#define DIO0 2

// Pin untuk sensor
#define SOIL_MOISTURE_PIN A0
#define TRIG_PIN 6
#define ECHO_PIN 7

// Pin untuk relay
#define RELAY1_PIN 3
#define RELAY2_PIN 4

// Pin untuk tombol
#define BUTTON_AUTO 5
#define BUTTON_MANUAL 8

// Pin untuk LED indikator mode
#define LED_AUTO A1
#define LED_MANUAL A2

#define TINGGI_MAX 7 // Ubah sesuai dengan tinggi maksimum dari dasar ke sensor (cm)

// Batas jarak dan histeresis
const int OPEN_THRESHOLD = 6;
const int CLOSE_THRESHOLD = 5;
const int HYSTERESIS = 0;

bool gateOpen = false;    
bool autoMode = true;   
bool manualToggle = false; // Untuk menyimpan status pintu saat mode manual aktif

const char* deviceId = "3";

void setup() {
  Serial.begin(9600);

  // Inisialisasi LoRa dengan retry jika gagal
  while (!initLoRa()) {
    Serial.println("Inisialisasi LoRa gagal! Mencoba kembali...");
    delay(5000);
  }

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(BUTTON_AUTO, INPUT_PULLUP);
  pinMode(BUTTON_MANUAL, INPUT_PULLUP);
  pinMode(LED_AUTO, OUTPUT);
  pinMode(LED_MANUAL, OUTPUT);

  // Pastikan relay mati saat awal
  stop();

  // Set indikator LED awal
  digitalWrite(LED_AUTO, HIGH);
  digitalWrite(LED_MANUAL, LOW);

  Serial.println("LoRa Berhasil Diinisialisasi");
}

void loop() {
  handleButtonPress();

  // Baca sensor
  long distance = getWaterHeight();

  // Hitung ketinggian air
  int ketinggian_air = TINGGI_MAX - distance;

  // Pastikan nilai tidak negatif
  if (ketinggian_air < 0) ketinggian_air = 0;
  
  int sensorValue = analogRead(SOIL_MOISTURE_PIN);
  int soilMoistureValue  = map(sensorValue, 897, 215, 1, 10);
  soilMoistureValue = constrain(soilMoistureValue, 1, 10); // Pastikan tetap dalam rentang 0-10

  // Mode otomatis
  if (autoMode) {
    if (distance > OPEN_THRESHOLD + HYSTERESIS && !gateOpen) {
      buka();
    } else if (distance < CLOSE_THRESHOLD - HYSTERESIS && gateOpen) {
      tutup();
    }
  }

  // Kirim data ke ESP
  String payload = createPayload(soilMoistureValue, ketinggian_air, gateOpen);
  LoRa.beginPacket();
  LoRa.print(payload);
  LoRa.endPacket();
  Serial.println("Sent: " + payload);

  // Tunggu sebentar agar data dikirim
  delay(100);

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

bool initLoRa() {
  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    return false;
  }
  Serial.println("LoRa Initialized Successfully!");
  return true;
}

void handleButtonPress() {
  static bool lastAutoButtonState = HIGH;
  static bool lastManualButtonState = HIGH;

  bool autoButtonState = digitalRead(BUTTON_AUTO);
  bool manualButtonState = digitalRead(BUTTON_MANUAL);

  // Tombol Mode Otomatis
  if (autoButtonState == LOW && lastAutoButtonState == HIGH) {
    delay(50); // Debounce
    if (digitalRead(BUTTON_AUTO) == LOW) {
      autoMode = !autoMode;
      
      // Sinkronisasi manualToggle dengan gateOpen saat beralih ke mode manual
      if (!autoMode) {
        manualToggle = gateOpen;
      }
      
      digitalWrite(LED_AUTO, autoMode ? HIGH : LOW);
      digitalWrite(LED_MANUAL, autoMode ? LOW : HIGH);
      Serial.print("Mode otomatis: ");
      Serial.println(autoMode ? "ON" : "OFF");
    }
  }
  lastAutoButtonState = autoButtonState;

  // Tombol Mode Manual (Hanya jika mode otomatis mati)
  if (!autoMode && manualButtonState == LOW && lastManualButtonState == HIGH) {
    delay(50);
    if (digitalRead(BUTTON_MANUAL) == LOW) {
      manualToggle = !manualToggle;
      if (manualToggle) {
        buka();
      } else {
        tutup();
      }
    }
  }
  lastManualButtonState = manualButtonState;
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
  long distance = duration * 0.034 / 2;

  Serial.print("Jarak: ");
  Serial.print(distance);
  Serial.println(" cm");

  return distance;
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

void buka() {
  Serial.println("Membuka pintu air...");
  digitalWrite(RELAY1_PIN, HIGH);
  digitalWrite(RELAY2_PIN, LOW);
  delay(9500);
  stop();
  gateOpen = true;
  Serial.println("Pintu air dibuka.");
}

void tutup() {
  Serial.println("Menutup pintu air...");
  digitalWrite(RELAY1_PIN, LOW);
  digitalWrite(RELAY2_PIN, HIGH);
  delay(9500);
  stop();
  gateOpen = false;
  Serial.println("Pintu air ditutup.");
}

void stop() {
  Serial.println("Menghentikan motor...");
  digitalWrite(RELAY1_PIN, LOW);
  digitalWrite(RELAY2_PIN, LOW);
}
