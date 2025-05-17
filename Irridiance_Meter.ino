#define BLYNK_TEMPLATE_ID "TMPL6q8TuZTSK"
#define BLYNK_TEMPLATE_NAME "Irradiance Monitoring"
#define BLYNK_AUTH_TOKEN "yhHXb3msqXmOYfqpJJEuPjIgjUqQuIc6"

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

// Replace these with your actual credentials
char auth[] = "get blnk authtoken";   //Sorry but this token is for us developers only
char ssid[] = "pakibago dipende sa wifi ssid"; //WIFI SSID
char pass[] = "wifi pw"; //WIFI PASSWORD

// Pin configuration
#define BUZZER_PIN 12
#define SOLAR_ADC_PIN 34
#define LDR_ADC_PIN 35

LiquidCrystal_I2C lcd(0x27, 20, 4); // LCD config

// Constants
const float EMVADO = 0.000007875;  // Area in m²
const float R = 10000.0;           // Resistor in ohms
const float EFFICIENCY = 17.0;     // Conversion efficiency in %
const int AVG_WINDOW = 10;
const int REFRESH_INTERVAL = 2000; // ms

float lastIrradiance = 0;
unsigned long lastTime = 0;

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);

  WiFi.begin(ssid, pass);
  Blynk.begin(auth, ssid, pass);

  Wire.begin();
  lcd.begin();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print(" Solar Monitor  ");
  delay(1000);
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

void loop() {
  Blynk.run();  // Always call this!

  unsigned long now = millis();
  float dt = (now - lastTime) / 1000.0;

  float solarADC = readSmoothedADC(SOLAR_ADC_PIN);
  float voltage = calculateVoltage(solarADC);
  float power = calculatePower(voltage);
  float irradiance = calculateIrradiance(power);
  float derivative = calculateDerivative(irradiance, lastIrradiance, dt);
  int ldrValue = analogRead(LDR_ADC_PIN);

  // Serial Debug
  Serial.println("=== Measurement ===");
  Serial.print("Voltage (V): "); Serial.println(voltage, 3);
  Serial.print("Power (mW): "); Serial.println(power * 1000, 2);
  Serial.print("Irradiance (W/m^2): "); Serial.println(irradiance, 1);
  Serial.print("dI/dt: "); Serial.println(derivative, 2);
  Serial.print("LDR Value: "); Serial.println(ldrValue);

  // LCD Display
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Irradiance:");
  lcd.setCursor(12, 0);
  lcd.print(irradiance, 1);
  lcd.print("W");

  lcd.setCursor(0, 1);
  lcd.print("Voltage:");
  lcd.setCursor(9, 1);
  lcd.print(voltage, 2);
  lcd.print("V");

  lcd.setCursor(0, 2);
  lcd.print("LDR:");
  lcd.setCursor(5, 2);
  lcd.print(ldrValue);

  lcd.setCursor(0, 3);
  lcd.print("dI/dt:");
  lcd.setCursor(6, 3);
  lcd.print(derivative, 1);

  // Feedback
  buzzerFeedback(irradiance);

  // Send to Blynk
  Blynk.virtualWrite(V0, irradiance);
  Blynk.virtualWrite(V1, voltage);
  Blynk.virtualWrite(V2, ldrValue);
  Blynk.virtualWrite(V3, derivative);

  lastIrradiance = irradiance;
  lastTime = now;
  delay(REFRESH_INTERVAL);
}
