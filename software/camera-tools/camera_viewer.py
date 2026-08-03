"""
RMAGC — ESP32-S3 Camera Stream Viewer

Receives JPEG frames over serial from ESP32-S3 (OV2640 camera).
Displays the live stream in an OpenCV window.

Protocol: 4-byte little-endian length header + JPEG data.

Usage:
    1. Update SERIAL_PORT and BAUD_RATE below.
    2. python camera_viewer.py
    3. Press 'q' to quit.
"""

import serial
import cv2
import numpy as np
import struct
import sys
import time

# ==================== Configuration ====================
SERIAL_PORT = 'COM4'       # Windows: COMx, Linux/macOS: /dev/ttyUSB0
BAUD_RATE   = 1500000       # Must match ESP32 Serial.begin() baud rate
# =======================================================


def main():
    # Open serial port
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=2)
    except serial.SerialException as e:
        print(f"Error opening {SERIAL_PORT}: {e}")
        print("Check port name and ensure no other program is using it.")
        sys.exit(1)

    print(f"Connected to {SERIAL_PORT}, waiting for frames...")
    print("Press 'q' to quit.")

    ser.reset_input_buffer()
    frame_count = 0

    try:
        while True:
            # 1. Read 4-byte length header (little-endian)
            header = ser.read(4)
            if len(header) < 4:
                continue

            img_len = struct.unpack('<I', header)[0]

            # Sanity check: JPEG frames are typically 1–150 KB
            if img_len == 0 or img_len > 1024 * 1024:
                print(f"Warning: abnormal length {img_len}, resetting buffer")
                ser.reset_input_buffer()
                continue

            # 2. Read image data
            img_data = b''
            while len(img_data) < img_len:
                remaining = img_len - len(img_data)
                chunk = ser.read(remaining)
                if not chunk:
                    print("Timeout reading frame, discarding")
                    break
                img_data += chunk

            if len(img_data) != img_len:
                continue

            # 3. Decode JPEG and display
            nparr = np.frombuffer(img_data, np.uint8)
            img = cv2.imdecode(nparr, cv2.IMREAD_COLOR)

            if img is not None:
                frame_count += 1
                cv2.imshow('RMAGC — Camera Stream (Q to quit)', img)

                if frame_count % 30 == 0:
                    print(f"Received {frame_count} frames, "
                          f"size: {img.shape[1]}x{img.shape[0]}")
            else:
                print("Frame decode failed — data may be corrupted")

            if cv2.waitKey(1) & 0xFF == ord('q'):
                break

    except KeyboardInterrupt:
        print("\nInterrupted by user")
    except Exception as e:
        print(f"Error: {e}")
    finally:
        ser.close()
        cv2.destroyAllWindows()
        print("Done.")


if __name__ == "__main__":
    main()
