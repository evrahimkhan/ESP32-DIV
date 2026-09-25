// Compile-time cross-check of the Evrahim S3 wiring: the TFT_eSPI User_Setup and
// ESP32-DIV/shared.h must agree about every pin, and nothing may sit on the
// octal-PSRAM pins of the N16R8 module. A wiring mistake fails the build here
// instead of on the bench. Fails to compile == wiring bug.
#include "BoardConfig.h"
#undef BOARD_CYD
#undef BOARD_ESP32_DIV_V1
#undef BOARD_ESP32_DIV_V2
#ifndef BOARD_EVRAS3
#define BOARD_EVRAS3
#endif
#define HIGH 1
#define LOW  0
#include "User_Setup evras3.h"
#include "shared.h"

#ifdef USE_HSPI_PORT
#error "USE_HSPI_PORT must stay off so TFT_eSPI uses FSPI (shared bus), leaving HSPI for touch"
#endif
#ifndef ST7789_DRIVER
#error "ST7789 driver must be selected"
#endif
#if defined(ILI9341_DRIVER) || defined(ILI9341_2_DRIVER) || defined(ST7735_DRIVER)
#error "only one display driver may be defined"
#endif
#ifdef TOUCH_CS
#error "TOUCH_CS must stay undefined: XPT2046 is driven by XPT2046_Touchscreen on its own bus"
#endif
#ifndef TFT_INVERSION_ON
#error "ST7789 240x320 needs TFT_INVERSION_ON"
#endif

static_assert(TFT_WIDTH == 240 && TFT_HEIGHT == 320, "panel must be 240x320");
static_assert(TFT_MISO == SD_MISO && TFT_MOSI == SD_MOSI && TFT_SCLK == SD_SCLK,
              "TFT must sit on the same 11/12/13 data bus as SD/CC1101/nRF24");
static_assert(TFT_MISO == 13 && TFT_MOSI == 11 && TFT_SCLK == 12, "shared bus is SCK 12 / MOSI 11 / MISO 13");
static_assert(TFT_CS == 10 && TFT_DC == 9 && TFT_RST == 14, "TFT control pins are CS 10 / DC 9 / RST 14");
static_assert(TFT_BL == BACKLIGHT_PIN && TFT_BL == 7, "backlight pin mismatch between User_Setup and shared.h");
static_assert(TFT_BACKLIGHT_ON == HIGH, "backlight is HIGH-on");
static_assert(SPI_FREQUENCY <= 10000000, "shared bus clock must stay within the CC1101 10 MHz SPI limit");
static_assert(XPT2046_CLK == 38 && XPT2046_MOSI == 39 && XPT2046_MISO == 40 &&
              XPT2046_CS == 41 && XPT2046_IRQ == 42, "touch pins changed");
static_assert(SD_CS == 5 && CC1101_CS == 21 && CSN_PIN_1 == 48, "chip selects changed");
static_assert(CC1101_GDO0 == 4 && CC1101_GDO2 == 6, "CC1101 GDO0=4 (TX) / GDO2=6 (RX)");

// subghz.cpp still drives TX_PIN directly in the jammer. GDO0/GDO2 are outputs
// of the CC1101, so TX_PIN must never land on one of those nets.
static_assert(TX_PIN != CC1101_GDO0 && TX_PIN != CC1101_GDO2 && RX_PIN != CC1101_GDO0,
              "TX_PIN/RX_PIN must not sit on a CC1101 GDO net (output driving output)");
static_assert(TX_PIN == 8 && RX_PIN == 8, "legacy 433 MHz lines stay on unconnected GPIO 8");
static_assert(CE_PIN_1 == 47 && CSN_PIN_1 == 48, "nRF24 CE=47 / CSN=48");

// No pin may sit on the octal-PSRAM pins (33-37) of the N16R8 module.
#define IN_PSRAM(p) ((p) >= 33 && (p) <= 37)
static_assert(!IN_PSRAM(TFT_CS) && !IN_PSRAM(TFT_DC) && !IN_PSRAM(TFT_RST) && !IN_PSRAM(TFT_BL) &&
              !IN_PSRAM(SD_CS) && !IN_PSRAM(CC1101_CS) && !IN_PSRAM(CC1101_GDO0) && !IN_PSRAM(CC1101_GDO2) &&
              !IN_PSRAM(CE_PIN_1) && !IN_PSRAM(CSN_PIN_1) && !IN_PSRAM(XPT2046_CS) && !IN_PSRAM(XPT2046_IRQ) &&
              !IN_PSRAM(TFT_SCLK) && !IN_PSRAM(TFT_MOSI) && !IN_PSRAM(TFT_MISO) &&
              !IN_PSRAM(XPT2046_CLK) && !IN_PSRAM(XPT2046_MOSI) && !IN_PSRAM(XPT2046_MISO),
              "GPIO 33-37 belong to the octal PSRAM on ESP32-S3 N16R8");

// Every chip select on the shared bus must be distinct.
static_assert(SD_CS != CC1101_CS && SD_CS != CSN_PIN_1 && SD_CS != TFT_CS &&
              CC1101_CS != CSN_PIN_1 && CC1101_CS != TFT_CS && CSN_PIN_1 != TFT_CS,
              "chip selects on the shared bus collide");

int main() { return 0; }
