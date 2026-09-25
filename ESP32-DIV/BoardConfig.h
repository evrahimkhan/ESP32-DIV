#pragma once

// Select the hardware target.
// Leave all lines commented to use the ESP32-DIV V2 wiring.

// #define BOARD_CYD
// #define BOARD_ESP32_DIV_V1
// #define BOARD_ESP32_DIV_V2
#define BOARD_EVRAS3

// BOARD_EVRAS3 = "Evrahim S3" — custom ESP32-S3 N16R8 (16 MB flash / 8 MB octal PSRAM)
// ---------------------------------------------------------------------------
//   ST7789 2.8" 240x320 TFT ........ CS 10, DC 9, RST 14, BL 7 (PWM, HIGH-on)
//   XPT2046 touch (own SPI bus) .... SCK 38, MOSI 39, MISO 40, CS 41, IRQ 42
//   Shared data bus (TFT+SD+RF) .... SCK 12, MOSI 11, MISO 13
//   SD card ........................ CS 5
//   CC1101 sub-GHz ................. CSN 21, GDO0 4, GDO2 6
//   NRF24L01+ (E01-MLO1DP5) ........ CE 47, CSN 48
//   Grove I2C ...................... SDA 15, SCL 16
//   UART0 .......................... TX 43, RX 44  (native USB used for flash/monitor)
//   Boot / deep-sleep wake ......... GPIO 0 (active LOW)
// Not populated on this board: PCF8574 buttons (touch-only UI), IR, buzzer,
// NeoPixel/RGB, GPS, PN532 RFID, battery voltage divider.
// All pin assignments live in shared.h under `#if defined(BOARD_EVRAS3)`.
//
// Display controller note: this board uses an ST7789 panel, not ILI9341.
// TFT_eSPI's controller/pin selection lives in Libraries/User_Setup evras3.h,
// which must be copied over the TFT_eSPI library's User_Setup.h when building:
//   cp "Libraries/User_Setup evras3.h" ~/Arduino/libraries/TFT_eSPI/User_Setup.h

// Set to 0 to hide the on-screen touch nav bar (5 footer buttons).
// Touch button input will still work when this is disabled.
#define TOUCH_BUTTON_CUE_ENABLED 1

// Optional fixed PCF8574 I2C address (0x20-0x27). Leave commented for auto-detect.
//#define pcf_ADDR 0x20

// Optional per-board touch calibration overrides (raw XPT2046 ADC range).
// CYD defaults (portrait): X 200..3700, Y 240..3800 — run Touch Calibrate if needed.
//#define TOUCH_X_MIN 200
//#define TOUCH_X_MAX 3700
//#define TOUCH_Y_MIN 240
//#define TOUCH_Y_MAX 3800

// Display orientation for BOARD_EVRAS3 (0/2 = portrait 240x320, 1/3 = landscape 320x240).
// The stock UI is laid out for portrait; keep 0 unless you have adapted the menus.
//#define TFT_ROTATION 1

#if !defined(BOARD_ESP32_DIV_V2) && !defined(BOARD_CYD) && !defined(BOARD_ESP32_DIV_V1) && !defined(BOARD_EVRAS3)
#define BOARD_ESP32_DIV_V2
#endif
