#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <HTTPClient.h>

// Wi-Fi credentials
const char* ssid = "Schifer";
const char* password = "neoace30+";

// ThingSpeak API
const char* server = "http://api.thingspeak.com/update";
String apiKey = "WDJA5YHAY2RBR1UO";

// Pin definitions
#define BUZZER_PIN 12
#define SOLAR_ADC_PIN 34
#define LDR_ADC_PIN 35

// LCD config
LiquidCrystal_I2C lcd(0x27, 20, 4);  // LCD I2C address

// Constants
const float EMVADO = 0.000007875;  // Area in m²
const float R = 10000.0;           // Resistor in ohms
const float EFFICIENCY = 17.0;     // Solar panel efficiency in %
const int AVG_WINDOW = 10;
const int REFRESH_INTERVAL = 20000; // ThingSpeak limit: 15 sec minimum

float lastIrradiance = 0;
unsigned long lastTime = 0;
bool buzzerEnabled = true;

// Variables shared between readings and ThingSpeak/LCD
float irradiance, voltage, derivative;
int ldrValue;

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  Wire.begin();
  lcd.begin(20, 4);
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print(" Solar Monitor  ");
  delay(1500);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Connected!");
}

float readSmoothedADC(int pin) {
  int total = 0;
  for (int i = 0; i < AVG_WINDOW; i++) {
    total += analogRead(pin);
    delay(5);
  }
  return total / float(AVG_WINDOW);
}

float calculateVoltage(float adcVal) {
  return (3.3 * adcVal) / 4095.0;
}

float calculatePower(float voltage) {
  return (voltage * voltage) / R;
}

float calculateIrradiance(float power) {
  float solarPower = (power * 100.0) / EFFICIENCY;
  return solarPower / EMVADO;
}

float calculateDerivative(float current, float previous, float dt) {
  return (dt > 0) ? (current - previous) / dt : 0;
}

void buzzerFeedback(float irradiance) {
  if (!buzzerEnabled) {
    digitalWrite(BUZZER_PIN, LOW);
    return;
  }

  if (irradiance < 200) {
    digitalWrite(BUZZER_PIN, LOW);
  } else if (irradiance < 600) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
  } else {
    for (int i = 0; i < 2; i++) {
      digitalWrite(BUZZER_PIN, HIGH);
      delay(100);
      digitalWrite(BUZZER_PIN, LOW);
      delay(100);
    }
  }
}

void updateReadings() {
  unsigned long now = millis();
  float dt = (now - lastTime) / 1000.0;

  float solarADC = readSmoothedADC(SOLAR_ADC_PIN);
  voltage = calculateVoltage(solarADC);
  float power = calculatePower(voltage);
  irradiance = calculateIrradiance(power);
  derivative = calculateDerivative(irradiance, lastIrradiance, dt);
  ldrValue = readSmoothedADC(LDR_ADC_PIN);

  lastIrradiance = irradiance;
  lastTime = now;

  Serial.println("=== Measurement ===");
  Serial.print("Voltage (V): "); Serial.println(voltage, 3);
  Serial.print("Power (mW): "); Serial.println(power * 1000, 2);
  Serial.print("Irradiance (W/m^2): "); Serial.println(irradiance, 1);
  Serial.print("dI/dt: "); Serial.println(derivative, 2);
  Serial.print("LDR Value: "); Serial.println(ldrValue);
}

void sendToThingSpeak() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(server) + "?api_key=" + apiKey +
                 "&field1=" + String(irradiance) +
                 "&field2=" + String(voltage, 2) +
                 "&field3=" + String(ldrValue) +
                 "&field4=" + String(derivative, 2) +
                 "&field5=" + String(buzzerEnabled ? 1 : 0);
                 
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode > 0) {
      Serial.println("Data sent to ThingSpeak!");
    } else {
      Serial.print("Error sending data. Code: ");
      Serial.println(httpCode);
    }
    http.end();
  } else {
    Serial.println("WiFi not connected.");
  }
}

void updateLCD() {
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Irradiance:");
  lcd.setCursor(12, 0); lcd.print(irradiance, 1); lcd.print("W");

  lcd.setCursor(0, 1); lcd.print("Voltage:");
  lcd.setCursor(9, 1); lcd.print(voltage, 2); lcd.print("V");

  lcd.setCursor(0, 2); lcd.print("LDR:");
  lcd.setCursor(5, 2); lcd.print(ldrValue);

  lcd.setCursor(0, 3); lcd.print("dI/dt:");
  lcd.setCursor(6, 3); lcd.print(derivative, 1);
}

void loop() {
  updateReadings();
  updateLCD();
  buzzerFeedback(irradiance);
  sendToThingSpeak();
  delay(REFRESH_INTERVAL);  // Wait 20 sec between updates (ThingSpeak limit)
}
