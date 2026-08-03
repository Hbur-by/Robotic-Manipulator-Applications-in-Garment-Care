# RMAGC — Robotic Manipulator Applications in Garment Care

[![Version](https://img.shields.io/badge/version-v2.0.0-blue)](https://github.com/your-org/rmagc)
[![Platform](https://img.shields.io/badge/platform-ESP32--S3-green)](https://github.com/your-org/rmagc)
[![License](https://img.shields.io/badge/license-MIT-orange)](LICENSE)

> **焕衣智联** — An integrated intelligent clothing care robot with multi-DOF manipulator, quick-change end effectors, and vision-based closed-loop control.

An integrated intelligent clothing care robot system featuring a movable platform with a 6-axis robotic arm. Designed for precision garment care at the *part level* (collars, underarms, cuffs), the system combines visual recognition with closed-loop control to deliver targeted dehumidification, UV sterilization, aroma misting, and ironing.

---

## Table of Contents

- [Project Status](#project-status)
- [System Overview](#system-overview)
- [Key Features](#key-features)
- [Hardware Architecture](#hardware-architecture)
- [Software Architecture](#software-architecture)
- [Performance](#performance)
- [Repository Structure](#repository-structure)
- [Getting Started](#getting-started)
- [Documentation](#documentation)
- [Competition Milestones](#competition-milestones)
- [Contributors](#contributors)
- [License](#license)

---

## Project Status

| Stage | Status | Description |
|-------|--------|-------------|
| School-level | ✅ Completed | 3-DOF acrylic arm, HSV color detection, feasibility proven |
| Provincial | ✅ Completed | 6-axis metal arm + linear rail, preset trajectory, WiFi control |
| **National** | 🚧 In Progress | USB HD camera, YOLOv8 object detection, full visual closed-loop |

**Current version**: v2.0.0 (first National-round refinement)

---

## System Overview

```
┌─────────────────────────────────────────────────────┐
│                   PERCEPTION LAYER                    │
│   USB Camera / OV2640 → Image Acquisition            │
├─────────────────────────────────────────────────────┤
│                    DECISION LAYER                     │
│   Laptop: OpenCV + YOLOv8 → Detection → Kinematics   │
│   Serial (UART / JSON) → ESP32-S3                    │
├─────────────────────────────────────────────────────┤
│                   EXECUTION LAYER                     │
│   ESP32-S3 → PCA9685 PWM → 6× Servos + Slide Motor   │
│            → Relay → Fan / Dehumidifier               │
├─────────────────────────────────────────────────────┤
│                  INTERACTION LAYER                    │
│   Phone / PC → WiFi → ESP32-S3 built-in Web Server   │
│   (LLM outfit recommendation — interface reserved)    │
└─────────────────────────────────────────────────────┘
```

---

## Key Features

- 🎯 **Part-level precision care** — Targets collars, underarms, and cuffs instead of whole-garment treatment
- 🔄 **Quick-change end effectors** — Standardized mechanical interface for dehumidifier, UV sterilizer, aroma mister, ironing head, and gripper
- 👁️ **Visual closed-loop control** — HSV color detection (provincial) → YOLOv8 object detection (national)
- 📱 **WiFi web control panel** — Real-time monitoring and command dispatch from any device
- 🧠 **LLM-ready** — Reserved API interface for GPT-4o / DeepSeek outfit recommendations
- 💰 **Low cost** — Target BOM ≤ ¥500 (vs. ¥5,000–10,000 for LG Styler)

---

## Hardware Architecture

| Subsystem | Component | Spec |
|-----------|-----------|------|
| **Main Controller** | ESP32-S3 (N16R8) | 16 MB Flash, 8 MB PSRAM (OPI), 240 MHz |
| **Camera** | OV2640 / USB HD | QVGA (provincial) → 720p (national) |
| **Servo Driver** | PCA9685 | 16-channel, 12-bit PWM, 50 Hz |
| **Robot Arm** | 6-axis metal arm | MG996R servos (≥9 kg·cm), 3-DOF + gripper |
| **Mobile Platform** | Single linear rail (prov.) / Gantry dual-axis (planned) | Timing belt or ball screw, ±0.02 mm repeatability |
| **End Effectors** | Dehumidifier (TEC1-12706), Ultrasonic atomizer (1.7 MHz), UV LED, Gripper | Quick-connect via spring-ball-lock |
| **Power** | 6V / 5V DC | Independent servo & logic power rails |

---

## Software Architecture

| Module | Technology | Runtime |
|--------|-----------|---------|
| Visual Recognition | OpenCV, YOLOv8 | Laptop (Python) |
| Color Detection (prov.) | HSV thresholding, custom RGB565→HSV | ESP32-S3 (C++) |
| Kinematics | D-H parameters, numerical IK (SciPy) / lookup table | Laptop / ESP32-S3 |
| Motion Control | PCA9685 PWM, linear interpolation, state machine | ESP32-S3 |
| Communication | UART (JSON), WiFi (HTTP + MJPEG stream) | ESP32-S3 |
| Web UI | Single-page HTML/CSS/JS (ESP32 built-in) | Browser |

---

## Performance

| Metric | Value |
|--------|-------|
| Color recognition accuracy | 95% (19/20) |
| Full closed-loop success rate | **90%** (18/20) |
| Average end-to-end time | **8.8 s** |
| Positioning error | < 5 mm (in normal workspace) |
| Quick-change time | < 3 s (target) |

---

## Repository Structure

```
rmagc/
├── README.md                       # You are here
├── README_CN.md                    # Full technical documentation (Chinese)
├── docs/
│   ├── images/                     # Photos, screenshots, diagrams
│   ├── mechanical-design/          # CAD renders, exploded views
│   └── ui-mockups/                 # App UI design mockups
├── hardware/
│   ├── mechanical/                 # 3D models, CAD files
│   │   ├── robot-arm/
│   │   ├── slide-rail/
│   │   ├── end-effectors/
│   │   └── quick-connect/
│   ├── electrical/                 # Schematics, wiring diagrams
│   └── BOM.md                      # Bill of materials
├── firmware/                       # ESP32-S3 firmware (PlatformIO)
│   ├── platformio.ini
│   ├── src/                        # Main application code
│   ├── lib/                        # Custom libraries
│   └── test/                       # Standalone module tests
├── software/                       # Host-side tools
│   ├── kinematics-sim/             # Python kinematics simulation
│   ├── camera-tools/               # Python camera viewer & color test
│   └── vision/                     # YOLOv8 training & inference
├── app/
│   └── ui-prototype/               # HTML/CSS/JS UI prototype
├── .gitignore
└── LICENSE
```

---

## Getting Started

### Prerequisites

- [PlatformIO](https://platformio.org/) (VS Code extension)
- Python 3.9+ with `numpy`, `matplotlib`, `scipy`, `opencv-python`, `pyserial`
- ESP32-S3 development board (N16R8 variant recommended)

### Firmware

```bash
cd firmware/
# Edit platformio.ini to match your board variant
# Build & upload
pio run --target upload
```

### Kinematics Simulation

```bash
cd software/kinematics-sim/
pip install -r requirements.txt
python kinematics.py
```

### Camera Viewer

```bash
cd software/camera-tools/
python camera_viewer.py   # Requires COM port configuration
```

---

## Documentation

- 🇨🇳 [完整中文技术文档](README_CN.md) — Full technical specification in Chinese
- 📋 [元器件清单](hardware/BOM.md) — Bill of Materials
- 🔧 [PlatformIO 配置](firmware/platformio.ini) — Build configuration

---

## Competition Milestones

### Stage 1 — School-level (Technical Feasibility)
- 3-DOF acrylic robot arm
- OV2640 color recognition (red/blue ≥ 90%)
- Incremental visual servoing: 8.8 s average, 90% success

### Stage 2 — Provincial (Prototype)
- Upgraded to 6-axis metal arm + single-motor timing-belt slide rail
- D-H kinematics modeling, forward/inverse solutions
- WiFi web control panel with live MJPEG stream
- Pre-programmed action sequence demo

### Stage 3 — National (Intelligent Closed-Loop) 🚧
- Camera upgrade: OV2640 → USB 720p
- Vision upgrade: HSV thresholding → YOLOv8 object detection
- Control upgrade: Open-loop preset → visual positioning + LUT + real-time execution
- Full "perception → decision → execution" integration

---

## Contributors

- **Team 焕衣智联 (HuanYiZhiLian)**

---

## License

This project is licensed under the MIT License — see [LICENSE](LICENSE) for details.

---

*Built with ❤️ for the National Robotics Innovation Contest*
