/**
 * RMAGC — PCA9685 Communication Test
 *
 * I2C scanner + servo angle control via PCA9685.
 * Validates I2C wiring and servo functionality.
 *
 * Usage:
 *   - Open Serial Monitor @ 115200 baud
 *   - Type an angle (0-180) + Enter to set servo position
 *   - Type "scan" to re-scan I2C bus
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

#define I2C_SDA        2
#define I2C_SCL        1
#define PCA9685_ADDR   0x40

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(PCA9685_ADDR);

#define SERVO_FREQ  50        // 50 Hz for analog servos
#define SERVOMIN    102       // ~0.5 ms → 0°
#define SERVOMAX    512       // ~2.5 ms → 180°
#define SERVO_CH    0         // Servo connected to PWM channel 0

void scanI2C() {
  Serial.println("\n=== I2C Scanner ===");
  byte error, address;
  int nDevices = 0;
  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("I2C device found at 0x");
      Serial.println(address, HEX);
      nDevices++;
    }
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found - check wiring!");
  else
    Serial.println("Scan complete.\n");
}

int angleToPWM(int angle) {
  int pulse = map(angle, 0, 180, SERVOMIN, SERVOMAX);
  return constrain(pulse, SERVOMIN, SERVOMAX);
}

void setServoAngle(int angle) {
  angle = constrain(angle, 0, 180);
  pwm.setPWM(SERVO_CH, 0, angleToPWM(angle));
  Serial.printf("Servo set to %d° (PWM: %d)\n", angle, angleToPWM(angle));
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(I2C_SDA, I2C_SCL);
  Serial.printf("I2C initialized: SDA=%d, SCL=%d\n", I2C_SDA, I2C_SCL);

  scanI2C();

  pwm.begin();
  pwm.setPWMFreq(SERVO_FREQ);
  delay(10);

  setServoAngle(90);  // Center position

  Serial.println("\n=== Ready ===");
  Serial.println("Commands:");
  Serial.println("  <angle>  - set servo to angle (0-180)");
  Serial.println("  scan     - re-scan I2C bus");
  Serial.println("================================\n");
}

void handleSerial() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    if (input.length() == 0) return;

    if (input == "scan") {
      scanI2C();
    } else {
      int angle = input.toInt();
      if (angle >= 0 && angle <= 180) {
        setServoAngle(angle);
      } else {
        Serial.println("Invalid. Enter angle 0-180 or 'scan'.");
      }
    }
  }
}

void loop() {
  handleSerial();
}
