# code_v0.2

能力阶梯改造版本目录。实施顺序见仓库根目录 `PLAN_能力阶梯.md`。

## 规划结构

```
code_v0.2/
├── firmware/
│   ├── platformio.ini            ← 从 v0.1 复制，保留 qio_opi PSRAM 配置
│   └── src/
│       ├── config.h              ← 填入实测 JOINT_MIN / JOINT_MAX
│       ├── main.cpp              ← 状态机 + 网页控制台
│       ├── teach.h / teach.cpp   ← L1 示教录制与回放（NVS 持久化）
│       └── protocol.h / .cpp     ← L4 结构化串口协议
└── software/
    ├── armctl/serial_link.py     ← L4 PC 端控制库
    └── vision/                   ← L5 / L6
        ├── camera.py
        ├── detect.py
        ├── calib.py
        ├── servo_loop.py
        └── app_preview.py
```

## 约束

- `code_v0.1/` 保持原样，作为现场失效时的回退版本，不在此处修改。
- L4 的结构化串口协议是 L6 视觉闭环的硬前置。
- 各级完成判据见 `PLAN_能力阶梯.md` 第 10 节自查表。