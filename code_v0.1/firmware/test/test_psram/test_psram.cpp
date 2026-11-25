/**
 * RMAGC — ESP32-S3 PSRAM Diagnostic Test
 *
 * Checks PSRAM availability, runs read/write verification, and tests
 * large memory allocations (simulating camera frame buffer requirements).
 *
 * This is critical for ESP32-S3-N16R8 boards which require
 * `board_build.arduino.memory_type = qio_opi` in platformio.ini.
 */

#include <Arduino.h>
#include <esp_psram.h>
#include <esp_heap_caps.h>

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n==========================================");
  Serial.println("   RMAGC — ESP32-S3 PSRAM Diagnostic");
  Serial.println("==========================================");

  // --- 1. Chip Info ---
  Serial.printf("\n[1] Chip Model:    %s\n", ESP.getChipModel());
  Serial.printf("[1] Chip Revision: v%d\n", ESP.getChipRevision());
  Serial.printf("[1] CPU Freq:      %d MHz\n", ESP.getCpuFreqMHz());
  Serial.printf("[1] Flash Size:    %d MB\n",
                ESP.getFlashChipSize() / (1024 * 1024));

  // --- 2. PSRAM Status ---
  Serial.println("\n[2] PSRAM Status:");

  size_t psram_size = esp_psram_get_size();
  Serial.printf("    esp_psram_get_size() = %d bytes", psram_size);

  if (psram_size == 0) {
    Serial.println(" FAIL — PSRAM not detected or not enabled");
    Serial.println("\n[Analysis] Possible causes:");
    Serial.println("  1. Dev board has no PSRAM chip soldered");
    Serial.println("  2. PSRAM not enabled in PlatformIO/Arduino config");
    Serial.println("  3. Wrong PSRAM mode (OPI vs QSPI)");
    Serial.println("  4. Hardware fault or soldering issue");
    Serial.println("\n[Fix] Set in platformio.ini:");
    Serial.println("  board_build.arduino.memory_type = qio_opi");
    Serial.println("  board_build.psram_type = opi");
    Serial.println("  build_flags = -DBOARD_HAS_PSRAM");
  } else {
    float size_mb = psram_size / (1024.0 * 1024.0);
    Serial.printf(" OK (%.1f MB)\n", size_mb);
  }

  // --- 3. Heap Info ---
  Serial.println("\n[3] Heap Memory:");
  Serial.printf("    Internal DRAM free: %d bytes\n",
                heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
  Serial.printf("    PSRAM free:         %d bytes\n",
                heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
  Serial.printf("    Total DRAM:         %d bytes\n",
                heap_caps_get_total_size(MALLOC_CAP_INTERNAL));
  Serial.printf("    Total PSRAM:        %d bytes\n",
                heap_caps_get_total_size(MALLOC_CAP_SPIRAM));

  // --- 4. PSRAM Read/Write Test ---
  if (psram_size > 0) {
    Serial.println("\n[4] PSRAM Read/Write Test:");

    const size_t test_size = 1024;
    uint8_t* psram_buffer =
        (uint8_t*)heap_caps_malloc(test_size, MALLOC_CAP_SPIRAM);

    if (psram_buffer == NULL) {
      Serial.println("    FAIL — Could not allocate in PSRAM!");
    } else {
      Serial.printf("    OK — Allocated %d bytes at %p\n",
                    test_size, psram_buffer);

      // Write
      for (size_t i = 0; i < test_size; i++) {
        psram_buffer[i] = (uint8_t)(i & 0xFF);
      }
      Serial.println("    OK — Data written");

      // Verify
      bool verify_ok = true;
      for (size_t i = 0; i < test_size; i++) {
        if (psram_buffer[i] != (uint8_t)(i & 0xFF)) {
          verify_ok = false;
          Serial.printf("    FAIL at %d: expected 0x%02X got 0x%02X\n",
                        i, (uint8_t)(i & 0xFF), psram_buffer[i]);
          break;
        }
      }
      if (verify_ok) Serial.println("    OK — Data verified");

      heap_caps_free(psram_buffer);
      Serial.println("    OK — Memory freed");
    }
  }

  // --- 5. Frame Buffer Allocation Test ---
  Serial.println("\n[5] Frame Buffer Simulation:");
  // QVGA RGB565: 320 * 240 * 2 = 153,600 bytes (~150 KB)
  size_t frame_buffer_size = 320 * 240 * 2;
  Serial.printf("    Allocating %d bytes (~%.1f KB) ...\n",
                frame_buffer_size, frame_buffer_size / 1024.0);

  void* test_fb = heap_caps_malloc(frame_buffer_size, MALLOC_CAP_SPIRAM);
  if (test_fb == NULL) {
    Serial.println("    FAIL — Camera init would also fail!");
    Serial.println("    Cause: PSRAM insufficient or not available");
  } else {
    Serial.printf("    OK — Allocated at %p\n", test_fb);
    heap_caps_free(test_fb);
  }

  // --- 6. Summary ---
  if (psram_size == 0) {
    Serial.println("\n==========================================");
    Serial.println("  ACTION REQUIRED: Enable PSRAM");
    Serial.println("  1. Open platformio.ini");
    Serial.println("  2. Add: board_build.arduino.memory_type = qio_opi");
    Serial.println("  3. Add: board_build.psram_type = opi");
    Serial.println("  4. Add: build_flags = -DBOARD_HAS_PSRAM");
    Serial.println("  5. Rebuild and upload");
    Serial.println("==========================================");
  } else {
    Serial.println("\n==========================================");
    Serial.println("  PSRAM OK — Ready for camera init.");
    Serial.println("==========================================");
  }

  Serial.println("\n[Test Complete] Idle loop with periodic heap report.\n");
}

void loop() {
  delay(5000);
  Serial.printf("[Runtime] DRAM free: %d, PSRAM free: %d\n",
                heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}
