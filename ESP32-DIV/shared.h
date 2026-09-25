#ifndef SHARED_H
#define SHARED_H
#include <stdint.h>
#include "BoardConfig.h"

/*──────────────────── Colors ────────────────────*/
const uint16_t GRAY = 0x8410, BLUE = 0x001F, RED = 0xF800,
               GREEN  = 0xB721, BLACK = 0x0000, WHITE = 0xFFFF,
               LIGHT_GRAY = 0xC618, DARK_GRAY = 0x4208;

uint16_t uiUniversalColor();
#define ORANGE uiUniversalColor()
               
#define TFT_DARKBLUE   0x3166
#define TFT_LIGHTBLUE  0x051F
#define TFTWHITE       0xFFFF
#define TFT_GRAY       0x8410

#ifdef TFT_GREEN
#undef TFT_GREEN
#endif
#define TFT_GREEN GREEN
#ifdef TFT_GREENYELLOW
#undef TFT_GREENYELLOW
#endif
#define TFT_GREENYELLOW GREEN

#define BG_Dark        0x20e4
#define BG_Light       0xf7de
#define FG_Dark        0x3166
#define FG_Light       0xe73c
#define LINE_Dark      0x8410  
#define LINE_Light     0x8410  
#define ICON_Dark      0xFBE4 
#define ICON_Light     0xFBE4
#define TEXT_Dark      0xFFFF
#define TEXT_Light     0x0000   
#define UI_ACCENT      0x3166

#define L_Dark        0x4208
#define L_Light       0xC618

// Default background for feature screens (separate from main menu background).
// Currently black, but you can change this one define to restyle all features.
#ifndef FEATURE_BG
#define FEATURE_BG BLACK
#endif

#define SELECTED_ICON_COLOR UI_ICON

/*──────────────────── UI Defaults ────────────────────*/
#ifndef UI_BG
#define UI_BG UI.bg
#endif
#ifndef UI_FG
#define UI_FG UI.fg
#endif
#ifndef UI_ICON
#define UI_ICON UI.icon
#endif
#ifndef UI_TEXT
#define UI_TEXT UI.text
#endif
#ifndef UI_LINE
#define UI_LINE UI.line
#endif
#ifndef UI_LABLE
#define UI_LABLE UI.lable
#endif
#ifndef UI_ACCENT
#define UI_ACCENT UI.accent
#endif
#ifndef UI_WARN
#define UI_WARN UI.warn
#endif
#ifndef UI_OK
#define UI_OK GREEN
#endif

/*──────────────────── Project Info ────────────────────*/
#ifndef ESP32DIV_NAME
#define ESP32DIV_NAME "ESP32-DIV"
#endif
#ifndef ESP32DIV_VERSION
#define ESP32DIV_VERSION "v1.7.2"
#endif


static inline constexpr uint8_t k0() { return (uint8_t)('h' - '`'); }

static const uint8_t OBF_PN[]   = {77, 91, 88, 59, 58, 37, 76, 65, 94};                              
static const uint8_t OBF_DN[]   = {75, 97, 110, 109, 122, 92, 109, 107, 96};                           
static const uint8_t OBF_EM[]   = {107, 97, 110, 109, 122, 124, 109, 107, 96, 72, 111, 101, 105, 97, 100, 38, 107, 103, 101};  
static const uint8_t OBF_GH[]   = {111, 97, 124, 96, 125, 106, 38, 107, 103, 101, 39, 107, 97, 110, 109, 122, 124, 109, 107, 96}; 
static const uint8_t OBF_WB[]   = {75, 97, 110, 109, 122, 92, 109, 107, 96, 38, 102, 109, 124};       

/*──────────────────── Board Selection ────────────────────*/
// Default is selected in BoardConfig.h. You can also pass a BOARD_* define
// from your build flags to target a board without editing source.
#if !defined(BOARD_ESP32_DIV_V2) && !defined(BOARD_CYD) && !defined(BOARD_ESP32_DIV_V1) && !defined(BOARD_EVRAS3)
#define BOARD_ESP32_DIV_V2
#endif

#if (defined(BOARD_ESP32_DIV_V2) + defined(BOARD_CYD) + defined(BOARD_ESP32_DIV_V1) + defined(BOARD_EVRAS3)) > 1
#error "Select only one board: BOARD_ESP32_DIV_V2, BOARD_ESP32_DIV_V1, BOARD_CYD, or BOARD_EVRAS3"
#endif

#if defined(BOARD_CYD)
#ifndef ESP32DIV_BOARD_NAME
#define ESP32DIV_BOARD_NAME "CYD ESP32-2432S028R"
#endif
#ifndef HAS_PCF8574_BUTTONS
#define HAS_PCF8574_BUTTONS 0
#endif
#ifndef BOARD_HAS_ESP32S3
#define BOARD_HAS_ESP32S3 0
#endif
#elif defined(BOARD_ESP32_DIV_V1)
#ifndef ESP32DIV_BOARD_NAME
#define ESP32DIV_BOARD_NAME "ESP32-DIV V1 ESP32U"
#endif
#ifndef HAS_PCF8574_BUTTONS
#define HAS_PCF8574_BUTTONS 1
#endif
#ifndef BOARD_HAS_ESP32S3
#define BOARD_HAS_ESP32S3 0
#endif
#elif defined(BOARD_ESP32_DIV_V2)
#ifndef ESP32DIV_BOARD_NAME
#define ESP32DIV_BOARD_NAME "ESP32-DIV V2"
#endif
#ifndef HAS_PCF8574_BUTTONS
#define HAS_PCF8574_BUTTONS 1
#endif
#ifndef BOARD_HAS_ESP32S3
#define BOARD_HAS_ESP32S3 1
#endif
#elif defined(BOARD_EVRAS3)
/* Evrahim S3: ESP32-S3 N16R8 + ST7789 240x320 + XPT2046 (own SPI bus) +
 * CC1101 + a single NRF24L01+. Touch-only UI: no PCF8574 button expander. */
#ifndef ESP32DIV_BOARD_NAME
#define ESP32DIV_BOARD_NAME "Evrahim S3"
#endif
#ifndef HAS_PCF8574_BUTTONS
#define HAS_PCF8574_BUTTONS 0
#endif
#ifndef BOARD_HAS_ESP32S3
#define BOARD_HAS_ESP32S3 1
#endif
#else
#error "Unknown board: define BOARD_ESP32_DIV_V2, BOARD_ESP32_DIV_V1, BOARD_CYD, or BOARD_EVRAS3"
#endif

/* v1 ESP32 has less internal DRAM for static buffers; v2/S3 keeps full sizes. */
#if BOARD_HAS_ESP32S3
#define ESP32DIV_FFT_SAMPLES       256
#define ESP32DIV_FFT_PALETTE_SIZE  128
#define ESP32DIV_PKT_GRAPH_WIDTH   240
#define ESP32DIV_JD_WAVE_WIDTH     220
#define ESP32DIV_JD_RSSI_SAMPLES   128
#define ESP32DIV_BLE_SCANNER_BARS  128
#define ESP32DIV_BLE_SCANNER_CHANS 128
#define ESP32DIV_ESB_RP_CAPTURES   8
#define ESP32DIV_ESB_RP_SEL_LINES  8
#define ESP32DIV_ESB_RP_LOG_LINES  10
#define ESP32DIV_MAX_WIFI_NETWORKS 50
#define ESP32DIV_PCAP_POOL_SIZE    10
#define ESP32DIV_PCAP_SNAP_LEN     2324
#define ESP32DIV_RFID_SRC_PAGES    256
#else
#define ESP32DIV_FFT_SAMPLES       256
#define ESP32DIV_FFT_PALETTE_SIZE  128
#define ESP32DIV_PKT_GRAPH_WIDTH   240
#define ESP32DIV_JD_WAVE_WIDTH     220
#define ESP32DIV_JD_RSSI_SAMPLES   64
#define ESP32DIV_BLE_SCANNER_BARS  64
#define ESP32DIV_BLE_SCANNER_CHANS 64
#define ESP32DIV_ESB_RP_CAPTURES   4
#define ESP32DIV_ESB_RP_SEL_LINES  4
#define ESP32DIV_ESB_RP_LOG_LINES  6
#define ESP32DIV_MAX_WIFI_NETWORKS 28
#define ESP32DIV_PCAP_POOL_SIZE    3
#define ESP32DIV_PCAP_SNAP_LEN     512
#define ESP32DIV_RFID_SRC_PAGES    128
#endif

/*──────────────────── Touch calibration profiles ────────────────────*/
/* Factory defaults per board (raw XPT2046 range). Override in BoardConfig.h.
 * User-saved calibration in settings.json overrides when board id matches. */
#if defined(BOARD_CYD)
#ifndef TOUCH_PROFILE_ID
#define TOUCH_PROFILE_ID "CYD"
#endif
#ifndef TOUCH_X_MIN
#define TOUCH_X_MIN 200
#endif
#ifndef TOUCH_X_MAX
#define TOUCH_X_MAX 3700
#endif
#ifndef TOUCH_Y_MIN
#define TOUCH_Y_MIN 240
#endif
#ifndef TOUCH_Y_MAX
#define TOUCH_Y_MAX 3800
#endif
#elif defined(BOARD_ESP32_DIV_V1)
#ifndef TOUCH_PROFILE_ID
#define TOUCH_PROFILE_ID "ESP32_DIV_V1"
#endif
#ifndef TOUCH_X_MIN
#define TOUCH_X_MIN 300
#endif
#ifndef TOUCH_X_MAX
#define TOUCH_X_MAX 3800
#endif
#ifndef TOUCH_Y_MIN
#define TOUCH_Y_MIN 300
#endif
#ifndef TOUCH_Y_MAX
#define TOUCH_Y_MAX 3800
#endif
#elif defined(BOARD_ESP32_DIV_V2)
#ifndef TOUCH_PROFILE_ID
#define TOUCH_PROFILE_ID "ESP32_DIV_V2"
#endif
#ifndef TOUCH_X_MIN
#define TOUCH_X_MIN 280
#endif
#ifndef TOUCH_X_MAX
#define TOUCH_X_MAX 3850
#endif
#ifndef TOUCH_Y_MIN
#define TOUCH_Y_MIN 320
#endif
#ifndef TOUCH_Y_MAX
#define TOUCH_Y_MAX 3750
#endif
#elif defined(BOARD_EVRAS3)
/* Generic 2.8" XPT2046 panel defaults. Run Tools -> Touch Calibrate once and the
 * result is stored in /config/settings.json for this TOUCH_PROFILE_ID. */
#ifndef TOUCH_PROFILE_ID
#define TOUCH_PROFILE_ID "EVRAS3"
#endif
#ifndef TOUCH_X_MIN
#define TOUCH_X_MIN 300
#endif
#ifndef TOUCH_X_MAX
#define TOUCH_X_MAX 3800
#endif
#ifndef TOUCH_Y_MIN
#define TOUCH_Y_MIN 300
#endif
#ifndef TOUCH_Y_MAX
#define TOUCH_Y_MAX 3800
#endif
#endif

#ifndef TOUCH_PROFILE_ID
#define TOUCH_PROFILE_ID "GENERIC"
#endif
#ifndef TOUCH_X_MIN
#define TOUCH_X_MIN 300
#endif
#ifndef TOUCH_X_MAX
#define TOUCH_X_MAX 3800
#endif
#ifndef TOUCH_Y_MIN
#define TOUCH_Y_MIN 300
#endif
#ifndef TOUCH_Y_MAX
#define TOUCH_Y_MAX 3800
#endif

#ifndef TFT_ROTATION
#if defined(BOARD_CYD) || defined(BOARD_ESP32_DIV_V1) || defined(BOARD_EVRAS3)
/* CYD / DIV V1 / Evrahim S3 panels are portrait-native.
 * EVRAS3: the whole UI is laid out for 240x320 portrait; the main menu needs the
 * full 320 px of height, so 1/3 (landscape) will clip rows until the menus are
 * made size-aware. Override in BoardConfig.h if you adapt the layout. */
#define TFT_ROTATION 0
#else
#define TFT_ROTATION 2
#endif
#endif

#ifndef TOUCH_SHARES_TFT_SPI
/* CYD uses a dedicated VSPI touch bus (T_CLK/T_DIN/T_OUT on 25/32/39); TFT stays on HSPI.
 * EVRAS3 also uses a dedicated touch bus: on ESP32-S3 the XPT2046 runs on the
 * second hardware SPI controller (HSPI/SPI3) while TFT + SD + CC1101 + nRF24
 * share the first one (FSPI/SPI2, GPIO 11/12/13). */
#define TOUCH_SHARES_TFT_SPI 0
#endif

#ifndef TOUCH_INVERT_X
/* 1 when the controller's raw X grows towards the right of the screen. */
#define TOUCH_INVERT_X 0
#endif
#ifndef TOUCH_INVERT_Y
/* 1 when the controller's raw Y grows towards the bottom of the screen.
 *
 * The display rotation and the touch rotation are separate things, and a panel
 * can report either direction. v1/v2 panels report Y growing upwards, which is
 * why the mapping inverted it; EVRAS3 reports the usual downwards direction.
 * A panel that shows touches mirrored vertically (touching the top lands at the
 * bottom) or horizontally needs the matching one flipped, and a fresh
 * Tools -> Touch Calibrate afterwards, because the stored limits are read in
 * this order. */
#if defined(BOARD_CYD) || defined(BOARD_EVRAS3)
#define TOUCH_INVERT_Y 0
#else
#define TOUCH_INVERT_Y 1
#endif
#endif

#if defined(BOARD_CYD)
#ifndef TOUCH_ROTATION
/* Match TFT_ROTATION (RNT CYD test uses the same rotation for tft and touch). */
#define TOUCH_ROTATION TFT_ROTATION
#endif
#endif

/*──────────────────── I/O & Pins ────────────────────*/
#if defined(BOARD_EVRAS3)
/* ═══════════ Evrahim S3 — ESP32-S3 N16R8 pin map ═══════════
 * Everything below feeds the per-peripheral #ifndef guards further down this
 * file, so this block is the single place to edit for this board.
 *
 * SHARED DATA BUS — SCK 12 / MOSI 11 / MISO 13
 *   On ESP32-S3 the Arduino `SPI` object is FSPI (bus 0 / SPI2). TFT_eSPI is
 *   built WITHOUT USE_HSPI_PORT for this board, so it drives that same
 *   controller — one bus, one pin set, four chip selects:
 *     ST7789 TFT ....... CS 10, DC 9, RST 14, BL 7   (see User_Setup evras3.h)
 *     SD card .......... CS 5
 *     CC1101 ........... CSN 21, GDO0 4 (TX), GDO2 6 (RX)
 *     NRF24L01+ ........ CE 47, CSN 48
 *
 * TOUCH BUS — XPT2046 on the second controller (HSPI / SPI3), see
 *   TOUCH_SHARES_TFT_SPI: SCK 38, MOSI 39, MISO 40, CS 41, IRQ 42.
 *
 * RESERVED — N16R8 has OCTAL PSRAM, so GPIO 33-37 belong to the PSRAM and are
 *   not usable. GPIO 45/46 and 3 are strapping pins; 19/20 are native USB;
 *   43/44 are UART0. GPIO 0 is BOOT / deep-sleep wake (active LOW).
 */
#define EVRAS3_BUS_SCK    12
#define EVRAS3_BUS_MOSI   11
#define EVRAS3_BUS_MISO   13

/* Backlight: PWM, active HIGH (ledcAttachPin(BACKLIGHT_PIN, PWM_CHANNEL)). */
#define BACKLIGHT_PIN      7

/* XPT2046 touch — dedicated SPI controller, never the shared bus. */
#define XPT2046_CLK       38
#define XPT2046_MOSI      39
#define XPT2046_MISO      40
#define XPT2046_CS        41
#define XPT2046_IRQ       42

/* SD card, shared bus. No card-detect switch wired, so SD_CD stays undefined. */
#define SD_CS              5
#define SD_CS_PIN          SD_CS
#define SD_SCLK            EVRAS3_BUS_SCK
#define SD_MOSI            EVRAS3_BUS_MOSI
#define SD_MISO            EVRAS3_BUS_MISO

/* CC1101 sub-GHz, shared bus. setGDO(TX, RX) -> GDO0 = 4, GDO2 = 6. */
#define CC1101_CS         21
#define CC1101_SCK         EVRAS3_BUS_SCK
#define CC1101_MOSI        EVRAS3_BUS_MOSI
#define CC1101_MISO        EVRAS3_BUS_MISO
#define SUBGHZ_TX_PIN      4
#define SUBGHZ_RX_PIN      6

/* One NRF24L01+ (E01-MLO1DP5). Radios 2 and 3 are absent: -1 keeps their
 * CSN lines undriven so they cannot disturb the module on CE_PIN_1/CSN_PIN_1. */
#define CE_PIN_1          47
#define CSN_PIN_1         48
#define CE_PIN_2          -1
#define CSN_PIN_2         -1
#define CE_PIN_3          -1
#define CSN_PIN_3         -1
#define NRF24_SCAN_CE      CE_PIN_1
#define NRF24_SCAN_CSN     CSN_PIN_1
#define NRF24_SPI_SCK      EVRAS3_BUS_SCK
#define NRF24_SPI_MOSI     EVRAS3_BUS_MOSI
#define NRF24_SPI_MISO     EVRAS3_BUS_MISO
#define NRF24_SPI_SS       CSN_PIN_1

/* UART0 + free Grove I2C port.
 * RX_PIN/TX_PIN are the legacy external 433 MHz RX/TX lines, not the CC1101's:
 * nothing reads RX_PIN, and subghz.cpp still toggles TX_PIN in the jammer. They
 * stay on an unconnected GPIO (8) — pointing TX_PIN at GPIO 4 would have the
 * ESP32 driving against the CC1101, because GDO0 is the radio's own output. */
#define RX_PIN             8
#define TX_PIN             8
#define I2C_SDA_PIN       15
#define I2C_SCL_PIN       16

/* Absent hardware. -1 is safe for these: the CS helpers bail out on pin < 0 and
 * pinMode()/ADC/UART all reject out-of-range pins. IR keeps two real but unused
 * pins (1/2) because the IR library installs an interrupt on the RX pin and an
 * invalid pin number there is not worth the risk. */
#define BUZZER_PIN        -1
#define BATTERY_ADC_PIN   -1
#define GPS_UART_RX       -1
#define GPS_UART_TX       -1
#define PN532_SCK         -1
#define PN532_MISO        -1
#define PN532_MOSI        -1
#define PN532_SS          -1
#define IR_RX_PIN          1
#define IR_TX_PIN          2
#endif  /* BOARD_EVRAS3 */

// PCF8574 I2C address: auto-detect 0x20-0x27 by default.
// To force a fixed address, add to BoardConfig.h: #define pcf_ADDR 0x21
#ifndef pcf_ADDR
#define PCF8574_AUTO_DETECT 1
#define PCF8574_I2C_ADDR  0x20
#else
#define PCF8574_AUTO_DETECT 0
#define PCF8574_I2C_ADDR  pcf_ADDR
#endif
#define PCF8574_ADDR_MIN 0x20
#define PCF8574_ADDR_MAX 0x27
#if defined(BOARD_ESP32_DIV_V1)
#define BTN_UP       6
#define BTN_DOWN     3
#define BTN_LEFT     4
#define BTN_RIGHT    5
#define BTN_SELECT   7
#else
#define BTN_UP       7
#define BTN_DOWN     5
#define BTN_LEFT     3
#define BTN_RIGHT    4
#define BTN_SELECT   6
#endif

/* Buzzer */
#ifndef BUZZER_PIN
// User hardware: buzzer on IO2
#define BUZZER_PIN -1
#endif

/* Backlight / PWM */
#ifndef BACKLIGHT_PIN
#if defined(BOARD_CYD)
#define BACKLIGHT_PIN   21
#elif defined(BOARD_ESP32_DIV_V1)
#define BACKLIGHT_PIN   32
#else
#define BACKLIGHT_PIN   7
#endif
#endif
#define PWM_CHANNEL     0
#define PWM_FREQ        5000
#define PWM_RESOLUTION  8

/* XPT2046 (Touch) SPI */
#ifndef XPT2046_CS
#if defined(BOARD_CYD)
#define XPT2046_CS   33
#elif defined(BOARD_ESP32_DIV_V1)
#define XPT2046_CS   33
#else
#define XPT2046_CS   18
#endif
#endif
#ifndef XPT2046_MOSI
#if defined(BOARD_CYD)
#define XPT2046_MOSI 32
#elif defined(BOARD_ESP32_DIV_V1)
#define XPT2046_MOSI 32
#else
#define XPT2046_MOSI 35
#endif
#endif
#ifndef XPT2046_MISO
#if defined(BOARD_CYD)
#define XPT2046_MISO 39
#elif defined(BOARD_ESP32_DIV_V1)
#define XPT2046_MISO 35
#else
#define XPT2046_MISO 37
#endif
#endif
#ifndef XPT2046_CLK
#if defined(BOARD_CYD)
#define XPT2046_CLK  25
#elif defined(BOARD_ESP32_DIV_V1)
#define XPT2046_CLK  25
#else
#define XPT2046_CLK  36
#endif
#endif
#ifndef XPT2046_IRQ
#if defined(BOARD_CYD)
#define XPT2046_IRQ  36
#elif defined(BOARD_ESP32_DIV_V1)
#define XPT2046_IRQ  34
#else
#define XPT2046_IRQ  255
#endif
#endif

/* SD Card */
#ifndef SD_CS
#if defined(BOARD_CYD)
#define SD_CS    5
#elif defined(BOARD_ESP32_DIV_V1)
#define SD_CS    5
#else
#define SD_CS    10
#endif
#endif
#ifndef SD_MOSI
#if defined(BOARD_CYD)
#define SD_MOSI  23
#elif defined(BOARD_ESP32_DIV_V1)
#define SD_MOSI  23
#else
#define SD_MOSI  11
#endif
#endif
#ifndef SD_MISO
#if defined(BOARD_CYD)
#define SD_MISO  19
#elif defined(BOARD_ESP32_DIV_V1)
#define SD_MISO  19
#else
#define SD_MISO  13
#endif
#endif
#ifndef SD_SCLK
#if defined(BOARD_CYD)
#define SD_SCLK  18
#elif defined(BOARD_ESP32_DIV_V1)
#define SD_SCLK  18
#else
#define SD_SCLK  12
#endif
#endif
/* DIV V2 has a card-detect switch on GPIO 38. EVRAS3 has none, and GPIO 38 is
 * its touch SCK, so no SD_CD must be defined for that board. */
#if !defined(BOARD_CYD) && !defined(BOARD_ESP32_DIV_V1) && !defined(BOARD_EVRAS3) && !defined(SD_CD)
#define SD_CD    38
#endif
#ifndef SD_CS_PIN
#define SD_CS_PIN 5
#endif

/* PN532 RFID/NFC (SPI).
 * CYD has few spare GPIOs; these are suggested external wiring defaults.
 * Override any pin below if your wiring differs. Requires Adafruit PN532 library. */
#ifndef PN532_SCK
#if defined(BOARD_CYD)
#define PN532_SCK  18
#else
#define PN532_SCK  12
#endif
#endif
#ifndef PN532_MISO
#if defined(BOARD_CYD)
#define PN532_MISO 19
#else
#define PN532_MISO 11
#endif
#endif
#ifndef PN532_MOSI
#if defined(BOARD_CYD)
#define PN532_MOSI 23
#else
#define PN532_MOSI 13
#endif
#endif
#ifndef PN532_SS
#if defined(BOARD_CYD)
#define PN532_SS   25
#else
#define PN532_SS   5
#endif
#endif

/* UART (if you use hardware serial on external pins) */
#ifndef RX_PIN
#if defined(BOARD_CYD)
#define RX_PIN 35
#elif defined(BOARD_ESP32_DIV_V1)
#define RX_PIN 16
#else
#define RX_PIN 6
#endif
#endif
#ifndef TX_PIN
#if defined(BOARD_CYD)
#define TX_PIN 22
#elif defined(BOARD_ESP32_DIV_V1)
#define TX_PIN 26
#else
#define TX_PIN 3
#endif
#endif

/* Neo-6M GPS — GPS module TX → ESP RX, GPS RX → ESP TX (optional). Uses UART2 by default. */
#ifndef GPS_UART_RX
#if defined(BOARD_CYD)
#define GPS_UART_RX 35
#elif defined(BOARD_ESP32_DIV_V1)
#define GPS_UART_RX 3
#else
#define GPS_UART_RX 5
#endif
#endif
#ifndef GPS_UART_TX
#if defined(BOARD_CYD)
#define GPS_UART_TX 22
#elif defined(BOARD_ESP32_DIV_V1)
#define GPS_UART_TX 1
#else
#define GPS_UART_TX 6
#endif
#endif

/* CC1101 (Sub-GHz) */
#ifndef CC1101_SCK
#if defined(BOARD_CYD)
#define CC1101_SCK  18
#elif defined(BOARD_ESP32_DIV_V1)
#define CC1101_SCK  18
#else
#define CC1101_SCK  12
#endif
#endif
#ifndef CC1101_MISO
#if defined(BOARD_CYD)
#define CC1101_MISO 19
#elif defined(BOARD_ESP32_DIV_V1)
#define CC1101_MISO 19
#else
#define CC1101_MISO 13
#endif
#endif
#ifndef CC1101_MOSI
#if defined(BOARD_CYD)
#define CC1101_MOSI 23
#elif defined(BOARD_ESP32_DIV_V1)
#define CC1101_MOSI 23
#else
#define CC1101_MOSI 11
#endif
#endif
#ifndef CC1101_CS
#if defined(BOARD_CYD)
#define CC1101_CS   27
#elif defined(BOARD_ESP32_DIV_V1)
#define CC1101_CS   27
#else
#define CC1101_CS   5
#endif
#endif

/* SubGHz (RCSwitch/Replay) data pins (wired to CC1101 GDO pins) */
// Override these in your board config if your wiring differs.
#ifndef SUBGHZ_RX_PIN
#if defined(BOARD_CYD)
#define SUBGHZ_RX_PIN 35
#elif defined(BOARD_ESP32_DIV_V1)
#define SUBGHZ_RX_PIN 16
#else
#define SUBGHZ_RX_PIN 3
#endif
#endif
#ifndef SUBGHZ_TX_PIN
#if defined(BOARD_CYD)
#define SUBGHZ_TX_PIN 22
#elif defined(BOARD_ESP32_DIV_V1)
#define SUBGHZ_TX_PIN 26
#else
#define SUBGHZ_TX_PIN 6
#endif
#endif

// CC1101 GDO mapping (used by ELECHOUSE_cc1101.setGDO).
// Default follows the legacy wiring used in the SubGHz code: setGDO(TX, RX).
#ifndef CC1101_GDO0
#define CC1101_GDO0 SUBGHZ_TX_PIN
#endif
#ifndef CC1101_GDO2
#define CC1101_GDO2 SUBGHZ_RX_PIN
#endif

/* SubGHz debug (0=off, 1=on) */
#ifndef SUBGHZ_DEBUG
#define SUBGHZ_DEBUG 0
#endif

/* NRF24 */
#ifndef CE_PIN_1
#if defined(BOARD_CYD)
#define CE_PIN_1  16
#elif defined(BOARD_ESP32_DIV_V1)
#define CE_PIN_1  4
#else
#define CE_PIN_1  15
#endif
#endif
#ifndef CSN_PIN_1
#if defined(BOARD_CYD)
#define CSN_PIN_1 17
#elif defined(BOARD_ESP32_DIV_V1)
#define CSN_PIN_1 5
#else
#define CSN_PIN_1 4
#endif
#endif
#ifndef CE_PIN_2
#if defined(BOARD_CYD)
#define CE_PIN_2  22
#elif defined(BOARD_ESP32_DIV_V1)
#define CE_PIN_2  26
#else
#define CE_PIN_2  47
#endif
#endif
#ifndef CSN_PIN_2
#if defined(BOARD_CYD)
#define CSN_PIN_2 27
#elif defined(BOARD_ESP32_DIV_V1)
#define CSN_PIN_2 27
#else
#define CSN_PIN_2 48
#endif
#endif
#ifndef CE_PIN_3
#if defined(BOARD_CYD)
#define CE_PIN_3  4
#elif defined(BOARD_ESP32_DIV_V1)
#define CE_PIN_3  16
#else
#define CE_PIN_3  14
#endif
#endif
#ifndef CSN_PIN_3
#if defined(BOARD_CYD)
#define CSN_PIN_3 25
#elif defined(BOARD_ESP32_DIV_V1)
#define CSN_PIN_3 17
#else
#define CSN_PIN_3 21
#endif
#endif

/* nRF24 bit-bang CE/CSN pair used by Scanner / ESB / MouseJack helpers. */
#ifndef NRF24_SCAN_CE
#define NRF24_SCAN_CE  CE_PIN_3
#endif
#ifndef NRF24_SCAN_CSN
#define NRF24_SCAN_CSN CSN_PIN_3
#endif
/* Shared SPI pinout for those nRF features (must match board SD/CC1101 bus on v1). */
#if BOARD_HAS_ESP32S3
#ifndef NRF24_SPI_SCK
#define NRF24_SPI_SCK  12
#endif
#ifndef NRF24_SPI_MISO
#define NRF24_SPI_MISO 13
#endif
#ifndef NRF24_SPI_MOSI
#define NRF24_SPI_MOSI 11
#endif
#ifndef NRF24_SPI_SS
#define NRF24_SPI_SS   4
#endif
#else
#ifndef NRF24_SPI_SCK
#define NRF24_SPI_SCK  SD_SCLK
#endif
#ifndef NRF24_SPI_MISO
#define NRF24_SPI_MISO SD_MISO
#endif
#ifndef NRF24_SPI_MOSI
#define NRF24_SPI_MOSI SD_MOSI
#endif
#ifndef NRF24_SPI_SS
#define NRF24_SPI_SS   CSN_PIN_1
#endif
#endif

/* IR Remote (Record/Replay)
 * NOTE: Default pins overlap with the optional NRF24 #3 wiring above.
 * If you use NRF24 on CE_PIN_3/CSN_PIN_3, override these in your board config.
 */
#ifndef IR_RX_PIN
#if defined(BOARD_CYD)
#define IR_RX_PIN 35
#elif defined(BOARD_ESP32_DIV_V1)
#define IR_RX_PIN 4
#else
#define IR_RX_PIN 21
#endif
#endif
#ifndef IR_TX_PIN
#if defined(BOARD_CYD)
#define IR_TX_PIN 4
#elif defined(BOARD_ESP32_DIV_V1)
#define IR_TX_PIN 5
#else
#define IR_TX_PIN 14
#endif
#endif
#ifndef IR_DEFAULT_KHZ
#define IR_DEFAULT_KHZ 38
#endif

/*──────────────────── Display & Touch ────────────────────*/
#ifndef TFT_WIDTH
#define TFT_WIDTH 240
#endif
#ifndef TFT_HEIGHT
#define TFT_HEIGHT 320
#endif
#ifndef STATUS_BAR_Y_OFFSET
#define STATUS_BAR_Y_OFFSET 0
#endif

/*──────────────────── Timing ────────────────────*/
#ifndef DEBOUNCE_MS
#define DEBOUNCE_MS 40
#endif
#ifndef LONG_PRESS_MS
#define LONG_PRESS_MS 800
#endif
#ifndef REPEAT_SCROLL_MS
#define REPEAT_SCROLL_MS 120
#endif

/*──────────────────── Backlight Levels ────────────────────*/
#ifndef BKL_LEVEL_MIN
#define BKL_LEVEL_MIN 10
#endif
#ifndef BKL_LEVEL_MED
#define BKL_LEVEL_MED 128
#endif
#ifndef BKL_LEVEL_MAX
#define BKL_LEVEL_MAX 255
#endif

/*──────────────────── Filesystem ────────────────────*/
#ifndef FS_MOUNT_OK_MSG
#define FS_MOUNT_OK_MSG "SD OK"
#endif
#ifndef FS_MOUNT_FAIL_MSG
#define FS_MOUNT_FAIL_MSG "SD Fail"
#endif
#ifndef LOG_DIR
#define LOG_DIR "/logs"
#endif
#ifndef CAPTURE_DIR
#define CAPTURE_DIR "/captures"
#endif

/*──────────────────── Battery ────────────────────*/
#ifndef BATTERY_ADC_PIN
#if defined(BOARD_ESP32_DIV_V1)
#define BATTERY_ADC_PIN 36
#elif defined(BOARD_CYD)
#define BATTERY_ADC_PIN -1
#else
#define BATTERY_ADC_PIN -1
#endif
#endif
#ifndef BATTERY_VDIV_R1
#define BATTERY_VDIV_R1 200000.0f
#endif
#ifndef BATTERY_VDIV_R2
#define BATTERY_VDIV_R2 100000.0f
#endif
#ifndef BATTERY_LOW_VOLT
#define BATTERY_LOW_VOLT 3.40f
#endif

/*──────────────────── Wi-Fi ────────────────────*/
#ifndef WIFI_SCAN_ACTIVE_MS
#define WIFI_SCAN_ACTIVE_MS  500
#endif
#ifndef WIFI_SCAN_PASSIVE_MS
#define WIFI_SCAN_PASSIVE_MS 0
#endif
#ifndef WIFI_SPECTRUM_FFT_N
#define WIFI_SPECTRUM_FFT_N  256
#endif

/*──────────────────── BLE ────────────────────*/
#ifndef BLE_ADV_INTERVAL_MS
#define BLE_ADV_INTERVAL_MS 100
#endif
#ifndef BLE_JAMMER_SWEEP_MS
#define BLE_JAMMER_SWEEP_MS 80
#endif

/*──────────────────── Sub-GHz / RF ────────────────────*/
#ifndef SUBGHZ_SAMPLE_RATE
#define SUBGHZ_SAMPLE_RATE 38000
#endif
#ifndef SUBGHZ_DEFAULT_FREQ
#define SUBGHZ_DEFAULT_FREQ 433920000UL
#endif
#ifndef NRF24_DATA_RATE
#define NRF24_DATA_RATE    2
#endif
#ifndef NRF24_PA_LEVEL
#define NRF24_PA_LEVEL     3
#endif
#ifndef CC1101_DEFAULT_MOD
#define CC1101_DEFAULT_MOD 2
#endif

/*──────────────────── Feature Flags ────────────────────*/
#ifndef FEATURE_WIFI_TOOLS
#define FEATURE_WIFI_TOOLS   1
#endif
#ifndef FEATURE_BLE_TOOLS
#define FEATURE_BLE_TOOLS    1
#endif
#ifndef FEATURE_BLE_DUCKY
#define FEATURE_BLE_DUCKY    BOARD_HAS_ESP32S3
#endif
#ifndef FEATURE_SUBGHZ_TOOLS
#define FEATURE_SUBGHZ_TOOLS 1
#endif

/*──────────────────── Utilities ────────────────────*/
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))
#endif
#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

/*──────────────────── Settings / Themes ────────────────────*/
enum class Theme : uint8_t { Dark = 0, Light = 1 };

#ifndef NEOPIXEL_BRIGHT_MAX
#define NEOPIXEL_BRIGHT_MAX 64
#endif
#ifndef SETTINGS_PATH
#define SETTINGS_PATH "/config/settings.json"
#endif

void displaySubmenu();

/*──────────────────── Runtime UI Palette ────────────────────*/
struct UiPalette { uint16_t bg, fg, icon, text, accent, line, lable, warn, ok; };
extern UiPalette UI;                   
void applyThemeToPalette(Theme t);
uint16_t uiUniversalColor();

/*──────────────────── UI Text Helpers ────────────────────*/
// Dim label text color (used for *text* only).
// Keep UI_LABLE unchanged because it's also used for fills (e.g., status bar bg).
static inline uint16_t uiDimTextColor() {
  // In Dark theme, UI_LABLE (L_Dark) is intentionally very dark; brighten text a bit.
  return (UI.bg == BG_Dark) ? GRAY : UI_LABLE;
}
#ifndef UI_DIM_TEXT
#define UI_DIM_TEXT uiDimTextColor()
#endif

/*──────────────────── Global State Flags ────────────────────*/
extern bool in_sub_menu;
extern bool feature_active;
extern bool submenu_initialized;
extern bool is_main_menu;
extern bool feature_exit_requested;

#endif
