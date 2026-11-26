/**
 * RMAGC · 通道识别固件（identify）
 *
 * 解决什么问题：
 *   全拆重装之后，7 个舵机插回 PCA9685 时「不确定哪个插在哪个通道」。
 *   主固件的 setup() 里有 resetAll()，一上电就把所有已插舵机拽向 HOME 角 ——
 *   插错通道的舵机会朝别人的角度冲，可能顶死、憋坏。
 *
 * 本固件的做法：
 *   1. 上电**不做任何归位**，所有通道默认无脉冲 → 舵机是松的（limp），不会自己动；
 *   2. 用串口选中某个通道，再显式激活它，这个通道才开始输出；
 *   3. 因此可以「插一个、试一个、记一个」，全程没有意外动作。
 *
 * 烧录：pio run -d code_v0.2/identify -t upload
 * 串口：115200，命令见下发 help 文本（上电时会打印）
 *
 * 与主固件的对应关系（code_v0.1/firmware/src/config.h）：
 *   CH0 CH_BASE / CH1 CH_SHOULDER / CH2 CH_ELBOW / CH3 CH_WRIST
 *   CH4 CH_GRIPPER / CH5 CH_WRIST_ROT / CH6 SLIDE_CH（连续旋转，用 ±脉宽驱动）
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// ==================== 与 config.h 保持一致 ====================
#define I2C_SDA       2
#define I2C_SCL       1
#define PCA9685_ADDR  0x40
#define SERVO_FREQ    50
#define SERVOMIN      102     // ~0.5 ms
#define SERVOMAX      594     // ~2.9 ms
#define CH_COUNT      7
#define SLIDE_CH      6
#define SLIDE_STOP    310
#define SLIDE_FORWARD 520
#define SLIDE_REVERSE 190
#define STEP_DEG      5

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(PCA9685_ADDR);

// 中位 90°，但**只有被激活过的通道才真正输出**
int  angle[CH_COUNT]   = {90, 90, 90, 90, 90, 90, 90};
bool touched[CH_COUNT] = {false, false, false, false, false, false, false};

int sel = 0;              // 当前选中的通道
bool pca_ok = false;

// ==================== 基础 ====================
int angleToPWM(int a) {
  return constrain(map(a, 0, 240, SERVOMIN, SERVOMAX), SERVOMIN, SERVOMAX);
}

void releaseChannel(int ch) {
  pwm.setPWM(ch, 0, 4096);   // full-off：不输出脉冲，舵机松
  touched[ch] = false;
}

void applyChannel(int ch) {
  if (ch == SLIDE_CH) return;   // 滑轨用 t/g/b，不按角度走
  pwm.setPWM(ch, 0, angleToPWM(angle[ch]));
  touched[ch] = true;
}

void setSlide(int ticks) {
  pwm.setPWM(SLIDE_CH, 0, ticks);
  touched[SLIDE_CH] = true;
}

void printStatus() {
  Serial.println();
  Serial.print("PCA9685 @0x40 : "); Serial.println(pca_ok ? "OK" : "NOT FOUND");
  for (int i = 0; i < CH_COUNT; i++) {
    Serial.print("  CH"); Serial.print(i);
    Serial.print(i == sel ? " *" : "  ");
    if (i == SLIDE_CH) {
      Serial.println(touched[i] ? "  SLIDE (有输出)" : "  SLIDE (未激活/松)");
    } else {
      Serial.print("  ");
      Serial.print(angle[i]); Serial.print("°  ");
      Serial.println(touched[i] ? "有输出" : "未激活/松");
    }
  }
  Serial.print("当前选中：CH"); Serial.println(sel);
}

void printHelp() {
  Serial.println();
  Serial.println("========== RMAGC identify v1 ==========");
  Serial.println("上电不归位，舵机默认是松的。插一个舵机，选中它所在的通道，再激活。");
  Serial.println("  0-6  选中通道（0..5 关节，6 滑轨）");
  Serial.println("  + / - 选中通道 ±5°（首次按下时会先输出，舵机此时才动）");
  Serial.println("  m     选中通道回中位 90°");
  Serial.println("  x     释放选中通道（停脉冲，舵机变松，可安全拔插）");
  Serial.println("  r     释放全部通道");
  Serial.println("  p     打印状态");
  Serial.println("  t / g / b   滑轨 前进 / 后退 / 停（只对 CH6）");
  Serial.println("  ?     再打印本帮助");
  Serial.println("=======================================");
}

// ==================== I2C 自检 ====================
bool scanPCA() {
  Wire.beginTransmission(PCA9685_ADDR);
  return Wire.endTransmission() == 0;
}

// ==================== 命令处理 ====================
void handleCmd(char c) {
  if (c >= '0' && c <= '6') {
    sel = c - '0';
    Serial.print(">> 选中 CH"); Serial.println(sel);
    if (sel == SLIDE_CH) Serial.println("   (滑轨：用 t 前进 / g 后退 / b 停)");
    return;
  }

  if (c == '+') {
    if (sel == SLIDE_CH) { Serial.println(">> CH6 是连续旋转，用 t/g/b"); return; }
    angle[sel] = constrain(angle[sel] + STEP_DEG, 0, 240);
    applyChannel(sel);
    Serial.print(">> CH"); Serial.print(sel); Serial.print(" → ");
    Serial.print(angle[sel]); Serial.println("°（看哪个关节在动，记下来）");
    return;
  }

  if (c == '-') {
    if (sel == SLIDE_CH) { Serial.println(">> CH6 是连续旋转，用 t/g/b"); return; }
    angle[sel] = constrain(angle[sel] - STEP_DEG, 0, 240);
    applyChannel(sel);
    Serial.print(">> CH"); Serial.print(sel); Serial.print(" → ");
    Serial.print(angle[sel]); Serial.println("°");
    return;
  }

  switch (c) {
    case 'm':
      if (sel == SLIDE_CH) { Serial.println(">> CH6 无中位概念"); break; }
      angle[sel] = 90;
      applyChannel(sel);
      Serial.print(">> CH"); Serial.print(sel); Serial.println(" 回中位 90°");
      break;

    case 'x':
      releaseChannel(sel);
      Serial.print(">> 已释放 CH"); Serial.print(sel);
      Serial.println("（现在可以安全拔插这个舵机）");
      break;

    case 'r':
      for (int i = 0; i < CH_COUNT; i++) releaseChannel(i);
      Serial.println(">> 全部通道已释放（所有舵机变松）");
      break;

    case 't': setSlide(SLIDE_FORWARD); Serial.println(">> 滑轨 前进"); break;
    case 'g': setSlide(SLIDE_REVERSE); Serial.println(">> 滑轨 后退"); break;
    case 'b': setSlide(SLIDE_STOP);    Serial.println(">> 滑轨 停");   break;

    case 'p': printStatus(); break;
    case '?': printHelp();   break;

    default: break;   // 换行/空格等忽略
  }
}

// ==================== setup / loop ====================
void setup() {
  Serial.begin(115200);
  delay(300);

  printHelp();

  Wire.begin(I2C_SDA, I2C_SCL);
  pca_ok = scanPCA();
  Serial.print("I2C 扫描 0x40 : ");
  Serial.println(pca_ok ? "找到" : "没找到（查 SDA/SCL 是否对调、VCC 是否有 3.3V、共地）");

  pwm.begin();
  pwm.setPWMFreq(SERVO_FREQ);
  delay(10);

  // ★ 关键：这里**没有** resetAll()。全部通道置为 full-off，舵机是松的。
  for (int i = 0; i < CH_COUNT; i++) releaseChannel(i);

  Serial.println("就绪：舵机全部松开，等你选中通道。");
  Serial.println("步骤：插 1 个舵机到 CH0 → 按 0 → 按 m → 看哪个关节动 → 填表 → 按 x 释放 → 换下一个");
  printStatus();
}

void loop() {
  if (Serial.available()) {
    char c = Serial.read();
    if (c != '\n' && c != '\r' && c != ' ') handleCmd(c);
  }
}