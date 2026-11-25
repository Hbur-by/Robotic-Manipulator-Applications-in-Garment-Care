/**
 * RMAGC Firmware — Hardware Configuration
 *
 * Pin definitions and tunable parameters for the ESP32-S3 based
 * clothing care robot control system.
 */

#ifndef CONFIG_H
#define CONFIG_H

// ==================== I2C ====================
#define I2C_SDA         2
#define I2C_SCL         1

// ==================== PCA9685 Servo Driver ====================
#define PCA9685_ADDR    0x40
#define SERVO_FREQ      50          // 50 Hz for analog servos
#define SERVOMIN        102         // ~0.5 ms pulse @ 50 Hz → 0°
#define SERVOMAX        594         // ~2.9 ms pulse @ 50 Hz → 240°

#define JOINT_COUNT     6

// Servo channel assignments
#define CH_BASE         0           // Base rotation
#define CH_SHOULDER     1           // Shoulder (small arm)
#define CH_ELBOW        2           // Elbow (large arm)
#define CH_WRIST        3           // Wrist
#define CH_GRIPPER      4           // Gripper open/close
#define CH_WRIST_ROT    5           // Wrist rotation

// Slide rail
#define SLIDE_CH        6
#define SLIDE_STOP      310         // Calibrate per actual hardware
#define SLIDE_FORWARD   520
#define SLIDE_REVERSE   190

// ==================== Camera (OV2640) ====================
#define PWDN_GPIO_NUM   17
#define RESET_GPIO_NUM  16
#define XCLK_GPIO_NUM   38
#define SIOD_GPIO_NUM   5           // SCCB SDA
#define SIOC_GPIO_NUM   4           // SCCB SCL

#define Y9_GPIO_NUM     15
#define Y8_GPIO_NUM     14
#define Y7_GPIO_NUM     13
#define Y6_GPIO_NUM     12
#define Y5_GPIO_NUM     11
#define Y4_GPIO_NUM     10
#define Y3_GPIO_NUM     9
#define Y2_GPIO_NUM     8

#define VSYNC_GPIO_NUM  6
#define HREF_GPIO_NUM   7
#define PCLK_GPIO_NUM   39
#define LED_PIN         21

// ==================== Relay / Fan ====================
#define FAN_PIN         15

// ==================== WiFi AP ====================
#define WIFI_SSID       "HuanYiZhiLian"
#define WIFI_PASSWORD   "12345678"

// ==================== Default Joint Angles (folded pose) ====================
// {base, shoulder, elbow, wrist, gripper, wrist_rot}
const int HOME_ANGLES[JOINT_COUNT] = {53, 230, 190, 240, 85, 135};

// ==================== Kinematics (arm geometry, mm) ====================
#define L0_BASE_HEIGHT  25          // Base rotation center above table
#define L1_UPPER_ARM    100         // Upper arm length (shoulder → elbow)
#define L2_FOREARM      120         // Forearm length (elbow → end effector)

#endif // CONFIG_H
