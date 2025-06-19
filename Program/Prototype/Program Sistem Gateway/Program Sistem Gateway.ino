#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <LoRa.h>
#include <ArduinoJson.h>

// Konfigurasi WiFi
const char* ssid = "Adyra";
const char* password = "1234567890";

// Konfigurasi MQTT
const char* mqtt_server = "123.231.250.36";
const int mqtt_port = 21283;
const char* mqtt_user = "tester1";
const char* mqtt_password = "itbstikombali11";
const char* mqtt_topic_publish = "satu";

WiFiClient espClient;
PubSubClient client(espClient);

// Pin LoRa untuk ESP32
#define SCK 18
#define MISO 19
#define MOSI 23
#define SS 5
#define RST 14
#define DIO0 26

// Konfigurasi LCD I2C
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Konfigurasi tombol dan LED
#define BUTTON_PIN 4
#define LED_PIN 2
#define BUTTON_DEBOUNCE_DELAY 200

int currentNanoIndex = 0;
String nanoNames[4] = {"1", "2", "3", "4"};
unsigned long lastButtonPress = 0;

// Menyimpan data terakhir
String lastDevice = "";
int lastSoilMoisture = 0;
long lastWaterHeight = 0;
String lastGateStatus = "";

void setupWiFi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client", mqtt_user, mqtt_password)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  setupWiFi();

  client.setServer(mqtt_server, mqtt_port);

  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa initialization failed!");
    while (1);
  }
  Serial.println("LoRa Receiver Initialized");

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Waiting for Data...");

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
}

void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  if (digitalRead(BUTTON_PIN) == LOW && (millis() - lastButtonPress > BUTTON_DEBOUNCE_DELAY)) {
    lastButtonPress = millis();
    currentNanoIndex = (currentNanoIndex + 1) % 4;
    Serial.printf("Switched to: %s\n", nanoNames[currentNanoIndex].c_str());
    digitalWrite(LED_PIN, HIGH);
  } else if (digitalRead(BUTTON_PIN) == HIGH) {
    digitalWrite(LED_PIN, LOW);
  }

  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String incomingData = "";
    while (LoRa.available()) {
      incomingData += (char)LoRa.read();
    }

    int rssi = LoRa.packetRssi();  // Membaca nilai RSSI
    float snr = LoRa.packetSnr();  // Membaca nilai SNR

    Serial.println("Data Received from LoRa:");
    Serial.println(incomingData);
    Serial.print("RSSI: ");
    Serial.println(rssi);
    Serial.print("SNR: ");
    Serial.println(snr);

    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, incomingData);
    if (error) {
      Serial.print("JSON parsing failed: ");
      Serial.println(error.c_str());
      return;
    }

    const char* device = doc["device"];
    int soilMoisture = doc["soilMoisture"];
    long waterHeight = doc["waterHeight"];
    const char* gateStatus = doc["gateStatus"];

    if (strcmp(device, nanoNames[currentNanoIndex].c_str()) == 0) {
      lastDevice = device;
      lastSoilMoisture = soilMoisture;
      lastWaterHeight = waterHeight;
      lastGateStatus = gateStatus;
    }

    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("Petak: " + String(lastDevice));
    lcd.setCursor(2, 1);
    lcd.print("Moisture: " + String(lastSoilMoisture));
    lcd.setCursor(2, 2);
    lcd.print("Water H: " + String(lastWaterHeight) + "cm");
    lcd.setCursor(2, 3);
    lcd.print("Gate: " + String(lastGateStatus));

    doc["RSSI"] = rssi;
    doc["SNR"] = snr;

    String jsonMessage;
    serializeJson(doc, jsonMessage);
    if (client.publish(mqtt_topic_publish, jsonMessage.c_str())) {
      Serial.println("Data published to MQTT successfully");
    } else {
      Serial.println("Failed to publish data to MQTT");
    }
  }
}
