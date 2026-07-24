#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// =========================
// OLED DISPLAY
// =========================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// =========================
// MOTOR DRIVER PINS
// =========================
#define PWMA 25
#define AIN1 26
#define AIN2 27

#define PWMB 14
#define BIN1 12
#define BIN2 13

#define STBY 33

// =========================
// EMERGENCY STOP
// =========================
#define ESTOP_BTN 15
#define ESTOP_LED 2

// =========================
// ENCODERS
// =========================
#define ENC1_CLK 34
#define ENC1_DT  35

#define ENC2_CLK 16
#define ENC2_DT  17

// =========================
// IMU (MPU6050)
// =========================
Adafruit_MPU6050 mpu;
float lastGyroZ = 0;

// =========================
// BATTERY MONITOR (ADC)
// =========================
#define BATTERY_PIN 32
float batteryVoltage = 0;
float batteryPercent = 0;

// =========================
// STATE VARIABLES
// =========================
bool estop_active = false;
bool estop_released_wait = false;
unsigned long estop_release_time = 0;

// =========================
// ENCODER COUNTS
// =========================
volatile long encoderA_count = 0;
volatile long encoderB_count = 0;

// =========================
// ENCODER INTERRUPTS
// =========================
void IRAM_ATTR encoderA_ISR() {
  int clk = digitalRead(ENC1_CLK);
  int dt  = digitalRead(ENC1_DT);
  if (clk == dt) encoderA_count++;
  else encoderA_count--;
}

void IRAM_ATTR encoderB_ISR() {
  int clk = digitalRead(ENC2_CLK);
  int dt  = digitalRead(ENC2_DT);
  if (clk == dt) encoderB_count++;
  else encoderB_count--;
}

// =========================
// DISTANCE CALCULATION
// =========================
float cm_per_step = 1.02;

float getDistanceA() { return encoderA_count * cm_per_step; }
float getDistanceB() { return encoderB_count * cm_per_step; }

// =========================
// SPEED CALCULATION
// =========================
unsigned long lastSpeedTime = 0;
long lastA = 0;
long lastB = 0;

float speedA = 0;
float speedB = 0;

void updateSpeed() {
  unsigned long now = millis();
  if (now - lastSpeedTime >= 1000) {
    speedA = (encoderA_count - lastA) * cm_per_step;
    speedB = (encoderB_count - lastB) * cm_per_step;
    lastA = encoderA_count;
    lastB = encoderB_count;
    lastSpeedTime = now;
  }
}

// =========================
// IMU READ FUNCTION
// =========================
void readIMU() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  lastGyroZ = g.gyro.z;
}

// =========================
// BATTERY MONITOR FUNCTION
// =========================
void updateBattery() {
  int raw = analogRead(BATTERY_PIN);      // 0–4095
  float volts = (raw / 4095.0) * 3.3;     // Convert to 0–3.3V
  batteryVoltage = volts;

  batteryPercent = (batteryVoltage / 3.3) * 100.0;
  if (batteryPercent > 100) batteryPercent = 100;
  if (batteryPercent < 0) batteryPercent = 0;
}

// =========================
// SERIAL TELEMETRY (prints once per second)
// =========================
unsigned long lastPrint = 0;
unsigned long lastBatteryPrint = 0;

void printTelemetry() {
  Serial.print("Dist L: ");
  Serial.print(getDistanceA());
  Serial.print("  Dist R: ");
  Serial.println(getDistanceB());

  Serial.print("Speed L: ");
  Serial.print(speedA);
  Serial.print("  Speed R: ");
  Serial.println(speedB);

  Serial.print("Gyro Z: ");
  Serial.println(lastGyroZ);

  Serial.println("----------------------");
}

void printBatteryEvery5Sec() {
  if (millis() - lastBatteryPrint >= 5000) {
    Serial.print("Battery Voltage: ");
    Serial.print(batteryVoltage, 2);
    Serial.print(" V  (");
    Serial.print(batteryPercent, 0);
    Serial.println("%)");
    lastBatteryPrint = millis();
  }
}

// =========================
// OLED DASHBOARD
// =========================
void updateOLED() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.print("Dist L: ");
  display.print(getDistanceA());

  display.setCursor(0, 10);
  display.print("Dist R: ");
  display.print(getDistanceB());

  display.setCursor(0, 20);
  display.print("Speed L: ");
  display.print(speedA);

  display.setCursor(0, 30);
  display.print("Speed R: ");
  display.print(speedB);

  display.setCursor(0, 40);
  display.print("Gyro Z: ");
  display.print(lastGyroZ);

  display.setCursor(70, 0);
  display.print("Bat:");
  display.print(batteryVoltage, 2);

  display.setCursor(70, 10);
  display.print("Pct:");
  display.print(batteryPercent, 0);
  display.print("%");

  display.setCursor(0, 50);
  if (estop_active) display.print("ESTOP: ACTIVE");
  else display.print("ESTOP: OK");

  display.display();
}

// =========================
// MOTOR FUNCTIONS
// =========================
void motorA_forward(int speed) {
  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, LOW);
  analogWrite(PWMA, speed);
}

void motorA_reverse(int speed) {
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);
  analogWrite(PWMA, speed);
}

void motorA_stop() {
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, LOW);
  analogWrite(PWMA, 0);
}

void motorB_forward(int speed) {
  digitalWrite(BIN1, HIGH);
  digitalWrite(BIN2, LOW);
  analogWrite(PWMB, speed);
}

void motorB_reverse(int speed) {
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, HIGH);
  analogWrite(PWMB, speed);
}

void motorB_stop() {
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, LOW);
  analogWrite(PWMB, 0);
}

void stop_all() {
  motorA_stop();
  motorB_stop();
}

// =========================
// EMERGENCY STOP
// =========================
void checkEmergencyStop() {
  bool pressed = (digitalRead(ESTOP_BTN) == LOW);

  if (pressed && !estop_active) {
    estop_active = true;
    estop_released_wait = false;
    digitalWrite(ESTOP_LED, HIGH);
    stop_all();
    Serial.println("ESTOP ACTIVATED");
  }

  if (!pressed && estop_active) {
    estop_active = false;
    estop_released_wait = true;
    estop_release_time = millis();
    digitalWrite(ESTOP_LED, LOW);
    Serial.println("ESTOP RELEASED - WAITING 5 SEC");
    stop_all();
  }
}

// =========================
// STRAIGHT-LINE CORRECTION
// =========================
void correctStraight(int baseSpeed) {
  int diff = encoderA_count - encoderB_count;
  int correction = diff * 2;
  motorA_forward(baseSpeed - correction);
  motorB_forward(baseSpeed + correction);
}

// =========================
// SETUP
// =========================
void setup() {
  Serial.begin(115200);

  pinMode(PWMA, OUTPUT);
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);

  pinMode(PWMB, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);

  pinMode(STBY, OUTPUT);
  digitalWrite(STBY, HIGH);

  pinMode(ESTOP_BTN, INPUT_PULLUP);
  pinMode(ESTOP_LED, OUTPUT);

  pinMode(ENC1_CLK, INPUT);
  pinMode(ENC1_DT, INPUT);
  pinMode(ENC2_CLK, INPUT);
  pinMode(ENC2_DT, INPUT);

  attachInterrupt(ENC1_CLK, encoderA_ISR, CHANGE);
  attachInterrupt(ENC2_CLK, encoderB_ISR, CHANGE);

  Wire.begin(21, 22);

  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050!");
    while (1) delay(10);
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  pinMode(BATTERY_PIN, INPUT);
  analogReadResolution(12);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED not found!");
    while (true);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Controller Ready");
  display.display();

  Serial.println("Motor Controller + Encoders + IMU + OLED + Battery Ready");
}

// =========================
// LOOP
// =========================
void loop() {

  checkEmergencyStop();
  if (estop_active) { stop_all(); return; }

  if (estop_released_wait) {
    stop_all();
    if (millis() - estop_release_time >= 5000) {
      estop_released_wait = false;
    } else return;
  }

  // FORWARD
  Serial.println("FORWARD");
  correctStraight(180);
  delay(2000);

  updateSpeed();
  readIMU();
  updateBattery();
  printTelemetry();
  printBatteryEvery5Sec();
  updateOLED();

  checkEmergencyStop();
  if (estop_active) return;

  // REVERSE
  Serial.println("REVERSE");
  motorA_reverse(180);
  motorB_reverse(180);
  delay(2000);

  updateSpeed();
  readIMU();
  updateBattery();
  printTelemetry();
  printBatteryEvery5Sec();
  updateOLED();

  checkEmergencyStop();
  if (estop_active) return;

  // STOP
  Serial.println("STOP");
  stop_all();
  delay(1500);

  updateBattery();
  printBatteryEvery5Sec();
  updateOLED();
}
