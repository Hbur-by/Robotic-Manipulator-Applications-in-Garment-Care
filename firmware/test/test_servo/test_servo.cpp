/**
 * RMAGC — Multi-Servo Keyboard Control Test
 *
 * Controls 3 servos (base, shoulder, elbow) via serial keyboard commands.
 * Validates multi-channel PCA9685 operation and joint range of motion.
 *
 * Keys:
 *   z/x : Base   ±10°
 *   a/s : Shoulder ±5°
 *   d/f : Elbow    ±5°
 *   r   : Reset all to 90°
 *   p   : Print current angles
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

#define I2C_SDA       2
#define I2C_SCL       1
#define PCA9685_ADDR  0x40

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(PCA9685_ADDR);

#define SERVO_FREQ    50
#define SERVOMIN      102
#define SERVOMAX      512
#define SERVO_COUNT   3           // Base, Shoulder, Elbow

int angles[SERVO_COUNT] = {90, 90, 90};

int angleToPWM(int angle) {
  int pulse = map(angle, 0, 180, SERVOMIN, SERVOMAX);
  return constrain(pulse, SERVOMIN, SERVOMAX);
}

void setServo(uint8_t ch, int angle) {
  if (ch >= SERVO_COUNT) return;
  angle = constrain(angle, 0, 180);
  pwm.setPWM(ch, 0, angleToPWM(angle));
  angles[ch] = angle;
}

void printStatus() {
  Serial.print("Angles: ");
  Serial.print("Base="); Serial.print(angles[0]);
  Serial.print("°  Shoulder="); Serial.print(angles[1]);
  Serial.print("°  Elbow="); Serial.print(angles[2]);
  Serial.println("°");
}

void resetAll() {
  for (int i = 0; i < SERVO_COUNT; i++) setServo(i, 90);
  Serial.println("All servos reset to 90°");
  printStatus();
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(I2C_SDA, I2C_SCL);
  pwm.begin();
  pwm.setPWMFreq(SERVO_FREQ);
  delay(10);
  resetAll();

  Serial.println("\n=== Keyboard Control ===");
  Serial.println("  z : Base +10°    x : Base -10°");
  Serial.println("  a : Shoulder +5°  s : Shoulder -5°");
  Serial.println("  d : Elbow +5°     f : Elbow -5°");
  Serial.println("  r : Reset all to 90°");
  Serial.println("  p : Print angles");
  Serial.println("=============================\n");
}

void loop() {
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    if (cmd == '\n' || cmd == '\r') return;

    bool updated = false;

    switch (cmd) {
      case 'z': angles[0] += 10; setServo(0, angles[0]); updated = true; break;
      case 'x': angles[0] -= 10; setServo(0, angles[0]); updated = true; break;
      case 'a': angles[1] +=  5; setServo(1, angles[1]); updated = true; break;
      case 's': angles[1] -=  5; setServo(1, angles[1]); updated = true; break;
      case 'd': angles[2] +=  5; setServo(2, angles[2]); updated = true; break;
      case 'f': angles[2] -=  5; setServo(2, angles[2]); updated = true; break;
      case 'r': resetAll(); updated = true; break;
      case 'p': printStatus(); break;
    }

    if (updated && cmd != 'r') printStatus();
  }
  delay(5);
}
