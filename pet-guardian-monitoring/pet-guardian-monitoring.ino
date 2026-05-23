#include <HTTPClient.h>
#include <WiFi.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHTesp.h"

LiquidCrystal_I2C LCD = LiquidCrystal_I2C(0x27, 20, 4);
DHTesp dht;

#define DHT_PIN 15
#define POT_ACTIVITY 34
#define POT_HEART 32
#define BUZZER 14
const char* API_URL = "http://enderecoDaApiDeC#/api/sensor-readings";

unsigned long lastSensorRead = 0;
unsigned long lastLcdUpdate = 0;
unsigned long lastSerialPrint = 0;
unsigned long lastBuzzerToggle = 0;
unsigned long lastAlertGenerated = 0;

const unsigned long SENSOR_INTERVAL = 3000;
const unsigned long LCD_INTERVAL = 2500;
const unsigned long SERIAL_INTERVAL = 3000;
const unsigned long BUZZER_INTERVAL = 500;
const unsigned long ALERT_COOLDOWN = 30000;

int lcdScreen = 0;
bool buzzerState = false;

float temperature = 0;
int activity = 0;
int heartRate = 0;
int healthScore = 100;

String status = "NORMAL";
String alertMessage = "NO_ALERT";
String confirmedAlert = "NO_ALERT";

int riskCounter = 0;
int attentionCounter = 0;
int normalCounter = 0;
int alertId = 0;

String traduzirAlerta(String a) {
  if (a == "NO_ALERT")          return "Sem alerta";
  if (a == "HIGH_TEMP")         return "Temp. alta";
  if (a == "HIGH_BPM")          return "Bpm elevado";
  if (a == "LOW_ACTIVITY")      return "Ativ. baixa";
  if (a == "TEMP_BPM_HIGH")     return "Temp e bpm altos";
  if (a == "BPM_LOW_ACTIVITY")  return "Bpm e ativ. baixa";
  if (a == "TEMP_LOW_ACTIVITY") return "Temp. e ativ. baixa";
  if (a == "GENERAL_RISK")      return "Risco geral";
  return a;
}

void spinner() {
  static int8_t counter = 0;
  const char* glyphs = "|/-\\";

  LCD.setCursor(15, 3);
  LCD.print(glyphs[counter++]);

  if (counter == strlen(glyphs)) {
    counter = 0;
  }
}

void connectWiFi() {
  LCD.setCursor(0, 0);
  LCD.print("Pet Guardian");

  LCD.setCursor(0, 1);
  LCD.print("Monitoramento pet");

  LCD.setCursor(0, 3);
  LCD.print("Conectando...");

  WiFi.begin("Wokwi-GUEST", "", 6);

  unsigned long lastSpinner = 0;

  while (WiFi.status() != WL_CONNECTED) {
    unsigned long currentMillis = millis();

    if (currentMillis - lastSpinner >= 250) {
      lastSpinner = currentMillis;
      spinner();
    }
  }

  Serial.println();
  Serial.println("WiFi conectado");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  LCD.clear();
  LCD.setCursor(0, 1);
  LCD.print("Sistema online");

  LCD.setCursor(0, 2);
  LCD.print("Monitorando...");
}

void readSensors() {
  TempAndHumidity data = dht.getTempAndHumidity();

  float newTemperature = data.temperature;

  int activityRaw = analogRead(POT_ACTIVITY);
  int heartRaw = analogRead(POT_HEART);

  int newActivity = map(activityRaw, 0, 4095, 0, 100);
  int newHeartRate = map(heartRaw, 0, 4095, 60, 180);

  temperature = (temperature * 0.7) + (newTemperature * 0.3);
  activity = (activity * 0.7) + (newActivity * 0.3);
  heartRate = (heartRate * 0.7) + (newHeartRate * 0.3);
}

void calculateHealth() {
  healthScore = 100;
  alertMessage = "NO_ALERT";

  bool highTemperature = temperature > 39.5;
  bool highHeartRate = heartRate > 150;
  bool lowActivity = activity < 25;

  if (highTemperature) {
    healthScore -= 30;
    alertMessage = "HIGH_TEMP";
  }

  if (highHeartRate) {
    healthScore -= 25;
    alertMessage = "HIGH_BPM";
  }

  if (lowActivity) {
    healthScore -= 10;
    alertMessage = "LOW_ACTIVITY";
  }

  if (highTemperature && highHeartRate) {
    healthScore -= 10;
    alertMessage = "TEMP_BPM_HIGH";
  }

  if (highHeartRate && lowActivity) {
    healthScore -= 15;
    alertMessage = "BPM_LOW_ACTIVITY";
  }

  if (highTemperature && lowActivity) {
    healthScore -= 15;
    alertMessage = "TEMP_LOW_ACTIVITY";
  }

  if (highTemperature && highHeartRate && lowActivity) {
    healthScore -= 20;
    alertMessage = "GENERAL_RISK";
  }

  if (healthScore < 0) {
    healthScore = 0;
  }

  status = "NORMAL";

  if (healthScore < 70 && healthScore >= 40) {
    status = "ATTENTION";
  }

  if (healthScore < 40) {
    status = "RISK";
  }
}

void stabilizeAlert() {
  unsigned long currentMillis = millis();

  if (status == "RISK") {
    riskCounter++;
    attentionCounter = 0;
    normalCounter = 0;
  }
  else if (status == "ATTENTION") {
    attentionCounter++;
    riskCounter = 0;
    normalCounter = 0;
  }
  else {
    normalCounter++;
    riskCounter = 0;
    attentionCounter = 0;
  }

  if (riskCounter >= 3 && currentMillis - lastAlertGenerated >= ALERT_COOLDOWN) {
    alertId++;
    confirmedAlert = alertMessage;
    lastAlertGenerated = currentMillis;
  }

  if (attentionCounter >= 3 && confirmedAlert == "NO_ALERT") {
    alertId++;
    confirmedAlert = alertMessage;
  }

  if (normalCounter >= 3) {
    confirmedAlert = "NO_ALERT";
  }
}

void updateBuzzer() {
  unsigned long currentMillis = millis();

  if (status == "RISK") {
    if (currentMillis - lastBuzzerToggle >= BUZZER_INTERVAL) {
      lastBuzzerToggle = currentMillis;
      buzzerState = !buzzerState;

      if (buzzerState) {
        tone(BUZZER, 1000);
      } else {
        noTone(BUZZER);
      }
    }
  } else {
    noTone(BUZZER);
    buzzerState = false;
  }
}

void updateLCD() {
  LCD.clear();

  if (lcdScreen == 0) {
    LCD.setCursor(0, 1);
    LCD.print("Saude: ");
    LCD.print(healthScore);

    LCD.setCursor(0, 2);
    LCD.print("Status: ");
      if      (status == "ATTENTION") LCD.print("ATENCAO");
      else if (status == "RISK") LCD.print("RISCO");
      else LCD.print("NORMAL");
  }
  
  if (lcdScreen == 1) {
    LCD.setCursor(0, 0);
    LCD.print("Temperatura:");

    LCD.setCursor(0, 1);
    LCD.print(temperature, 1);
    LCD.print(" C");

    LCD.setCursor(0, 2);
    LCD.print("Frequencia Cardiaca:");

    LCD.setCursor(0, 3);
    LCD.print(heartRate);
    LCD.print(" bpm");
}

  if (lcdScreen == 2) {
    LCD.setCursor(0, 1);
    LCD.print("Alerta:");

    LCD.setCursor(0, 2);
    LCD.print(traduzirAlerta(confirmedAlert));
  }

  lcdScreen++;

  if (lcdScreen > 2) {
    lcdScreen = 0;
  }
}

void printSerial() {
  Serial.println("===== Monitoramento - PetGuardian =====");

  Serial.print("Temperatura: ");
  Serial.println(temperature);

  Serial.print("Atividade: ");
  Serial.println(activity);

  Serial.print("Freq. Cardiaca: ");
  Serial.println(heartRate);

  Serial.print("Pontuacao de Saude: ");
  Serial.println(healthScore);

  Serial.print("Status: ");
  Serial.println(status);

  Serial.print("Alerta atual: ");
  Serial.println(alertMessage);

  Serial.print("Alerta confirmado: ");
  Serial.println(confirmedAlert);

  Serial.print("ID do alerta: ");
  Serial.println(alertId);

  Serial.print("Contador de risco: ");
  Serial.println(riskCounter);

  Serial.print("Contador de atencao: ");
  Serial.println(attentionCounter);

  Serial.print("Contador normal: ");
  Serial.println(normalCounter);

  Serial.println("=======================");
  Serial.println();
}

void sendDataToApi() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    http.begin(API_URL);
    http.addHeader("Content-Type", "application/json");

    String json = "{";
    json += "\"deviceCode\":\"PG-ESP32-001\",";
    json += "\"temperature\":" + String(temperature, 1) + ",";
    json += "\"heartRate\":" + String(heartRate) + ",";
    json += "\"activityLevel\":" + String(activity) + ",";
    json += "\"healthScore\":" + String(healthScore) + ",";
    json += "\"status\":\"" + status + "\",";
    json += "\"alertMessage\":\"" + confirmedAlert + "\"";
    json += "}";

    int responseCode = http.POST(json);

    Serial.print("Resposta HTTP: ");
    Serial.println(responseCode);

    Serial.println("JSON enviado:");
    Serial.println(json);

    http.end();
  } else {
    Serial.println("WiFi desconectado. Dados nao enviados.");
  }
}

void setup() {
  Serial.begin(115200);

  LCD.init();
  LCD.backlight();

  dht.setup(DHT_PIN, DHTesp::DHT22);
  pinMode(BUZZER, OUTPUT);

  connectWiFi();
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - lastSensorRead >= SENSOR_INTERVAL) {
    lastSensorRead = currentMillis;

    readSensors();
    calculateHealth();
    stabilizeAlert();
    sendDataToApi();
  }

  if (currentMillis - lastLcdUpdate >= LCD_INTERVAL) {
    lastLcdUpdate = currentMillis;
    updateLCD();
  }

  if (currentMillis - lastSerialPrint >= SERIAL_INTERVAL) {
    lastSerialPrint = currentMillis;
    printSerial();
  }

  updateBuzzer();
}