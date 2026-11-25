"""
RMAGC — Camera Viewer with Color Detection Overlay

Receives JPEG frames from ESP32-S3 via serial and performs HSV
color detection on the PC side. Compares with ESP32 onboard detection.

Usage:
    python camera_viewer_color.py
"""

import serial
import cv2
import numpy as np
import struct
import time

# ==================== Configuration ====================
COM = "COM9"
BAUD = 1500000
# =======================================================

# HSV thresholds for primary colors
RED_LOW   = np.array([0, 100, 60])
RED_HIGH  = np.array([15, 255, 255])
GREEN_LOW  = np.array([35, 60, 60])
GREEN_HIGH = np.array([80, 255, 255])
BLUE_LOW   = np.array([90, 70, 60])
BLUE_HIGH  = np.array([125, 255, 255])


def judge_color(frame):
    """Detect dominant color in the center ROI of the frame."""
    h, w = frame.shape[:2]
    roi = frame[h // 4 : h * 3 // 4, w // 4 : w * 3 // 4]
    hsv = cv2.cvtColor(roi, cv2.COLOR_BGR2HSV)

    red_area   = cv2.countNonZero(cv2.inRange(hsv, RED_LOW, RED_HIGH))
    green_area = cv2.countNonZero(cv2.inRange(hsv, GREEN_LOW, GREEN_HIGH))
    blue_area  = cv2.countNonZero(cv2.inRange(hsv, BLUE_LOW, BLUE_HIGH))

    max_area = max(red_area, green_area, blue_area)
    if max_area < 500:
        return "Color: None"
    if max_area == red_area:
        return "Color: RED"
    elif max_area == green_area:
        return "Color: GREEN"
    else:
        return "Color: BLUE"


def main():
    ser = serial.Serial(COM, BAUD, timeout=3)
    cv2.namedWindow("RMAGC — Color Detection", cv2.WINDOW_AUTOSIZE)

    while True:
        # Overflow protection
        if ser.in_waiting > 80000:
            ser.reset_input_buffer()
            continue

        head = ser.read(4)
        if len(head) != 4:
            time.sleep(0.01)
            continue

        frame_len = struct.unpack("<I", head)[0]
        if frame_len < 1000 or frame_len > 150000:
            ser.reset_input_buffer()
            continue

        data = b""
        while len(data) < frame_len:
            chunk = ser.read(frame_len - len(data))
            if not chunk:
                break
            data += chunk
        if len(data) != frame_len:
            continue

        img = cv2.imdecode(np.frombuffer(data, np.uint8), cv2.IMREAD_COLOR)
        if img is None:
            continue

        # Draw center ROI rectangle
        h, w = img.shape[:2]
        cv2.rectangle(img, (w // 4, h // 4), (w * 3 // 4, h * 3 // 4),
                      (0, 255, 0), 2)

        # Color detection result
        res_text = judge_color(img)
        cv2.putText(img, res_text, (20, 45),
                    cv2.FONT_HERSHEY_SIMPLEX, 1.2, (0, 0, 255), 2)

        cv2.imshow("RMAGC — Color Detection", img)
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    ser.close()
    cv2.destroyAllWindows()


if __name__ == "__main__":
    main()
